// Raven SDK Installer - installation engine (see engine.hpp).
#include "engine.hpp"

#include "../platform/platform.hpp"
#include "../shell/integration.hpp"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>

namespace rinst {

// ---------------------------------------------------------------------------
//  Versions and the existing installation
// ---------------------------------------------------------------------------

int compare_versions(const std::string& a, const std::string& b) {
	const auto pa = split(a, '.'), pb = split(b, '.');
	for (std::size_t i = 0; i < std::max(pa.size(), pb.size()); i++) {
		const long x = i < pa.size() ? std::strtol(pa[i].c_str(), nullptr, 10) : 0;
		const long y = i < pb.size() ? std::strtol(pb[i].c_str(), nullptr, 10) : 0;
		if (x != y) return x < y ? -1 : 1;
	}
	return 0;
}

fs::path install_pointer_path() {
	return platform::config_dir() / "install.json";
}

ExistingInstall find_existing_install(const fs::path& hint) {
	ExistingInstall e;
	std::vector<fs::path> candidates;
	std::string text;
	Json pointer;
	if (read_text_file(install_pointer_path(), text) && Json::parse(text, pointer) && pointer["sdk_dir"].is_string())
		candidates.push_back(path_from_utf8(pointer["sdk_dir"].as_string()));
	if (!hint.empty()) candidates.push_back(hint);
	std::error_code ec;
	for (const fs::path& dir : candidates) {
		if (!fs::exists(manifest_path(dir), ec)) continue;
		if (Manifest::load(manifest_path(dir), e.manifest, nullptr)) {
			e.found = true;
			e.sdk_dir = dir;
			return e;
		}
	}
	return e;
}

// ---------------------------------------------------------------------------
//  Transaction
// ---------------------------------------------------------------------------

namespace {

struct Journal {
	struct Entry {
		enum Kind { CreatedFile, ReplacedFile, CreatedDir, Action } kind;
		fs::path path;
		fs::path backup;
		std::string action;
		Json undo;
	};
	std::vector<Entry> entries;
	std::set<std::string> txn_roots;
	int backups = 0;

	fs::path backup_path(const fs::path& root) {
		txn_roots.insert(path_to_utf8(root));
		return root / ".raven-txn" / std::to_string(backups++);
	}

	/** Undoes everything, in reverse order. */
	void rollback(Logger& log) {
		std::error_code ec;
		for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
			switch (it->kind) {
				case Entry::CreatedFile:
					fs::remove(it->path, ec);
					break;
				case Entry::ReplacedFile:
					fs::remove(it->path, ec);
					fs::rename(it->backup, it->path, ec);
					if (ec) log.error("rollback: cannot restore " + path_to_utf8(it->path) + " from " + path_to_utf8(it->backup));
					break;
				case Entry::CreatedDir:
					remove_dir_if_empty(it->path);
					break;
				case Entry::Action: {
					std::string why;
					if (!integration_undo(it->action, it->undo, &why)) log.warning("rollback: " + it->action + ": " + why);
					break;
				}
			}
		}
		entries.clear();
		cleanup();
	}

	/** Deletes the backups (after the commit, or after a rollback restored them). */
	void cleanup() {
		std::error_code ec;
		for (const auto& root : txn_roots) {
			const fs::path txn = path_from_utf8(root) / ".raven-txn";
			fs::remove_all(txn, ec);
		}
	}
};

std::string component_of(const std::string& payload_path) {
	if (starts_with(payload_path, "designer/")) return "designer";
	return "sdk";
}

bool contains(const std::vector<std::string>& list, const std::string& item) {
	return std::find(list.begin(), list.end(), item) != list.end();
}

std::string maintenance_name() {
#if defined(_WIN32)
	return "RavenSDKMaintenance.exe";
#elif defined(__APPLE__)
	return "Raven SDK Maintenance.app";
#else
	return "RavenSDKMaintenance";
#endif
}

} // namespace

Installer::Installer(const Payload& payload, Logger& log, fs::path self_executable)
	: payload_(payload), log_(log), self_(std::move(self_executable)) {}

std::vector<Installer::Planned> Installer::plan(const InstallOptions& o) const {
	std::vector<Planned> list;
	for (const PayloadFile& f : payload_.files()) {
		const std::string& p = f.path;
		Planned item{&f, {}, {}, component_of(p), f.link.empty() ? payload_.blob(f.blob).size : 0};
		if (starts_with(p, "sdk/")) {
			if (!o.install_sdk) continue;
			const std::string rest = p.substr(4);
			const std::string target = rest.substr(0, rest.find('/'));
			if (!contains(o.targets, target)) continue;
			item.root = o.sdk_dir;
			item.dest = o.sdk_dir / path_from_utf8(rest);
		} else if (starts_with(p, "templates/") || starts_with(p, "sdk-bin/") || starts_with(p, "sdk-share/")) {
			if (!o.install_sdk) continue;
			std::string rest = p;
			if (starts_with(p, "sdk-bin/")) rest = "bin/" + p.substr(8);
			if (starts_with(p, "sdk-share/")) rest = "share/" + p.substr(10);
			item.root = o.sdk_dir;
			item.dest = o.sdk_dir / path_from_utf8(rest);
		} else if (starts_with(p, "designer/")) {
			if (!o.install_designer) continue;
			item.root = o.designer_dir;
			item.dest = o.designer_dir / path_from_utf8(p.substr(9));
		} else {
			continue; // installer resources (icons of the window...)
		}
		list.push_back(item);
	}
	return list;
}

std::uint64_t Installer::estimate_size(const InstallOptions& options) const {
	std::uint64_t total = 0;
	for (const auto& p : plan(options)) total += p.size;
	return total;
}

std::vector<std::string> Installer::validate(const InstallOptions& o) const {
	std::vector<std::string> problems;
	if (!o.install_sdk && !o.install_designer) problems.push_back("Nothing is selected.");
	if (o.install_sdk && o.targets.empty()) problems.push_back("Select at least one library package of the SDK.");
	if (o.sdk_dir.empty() || !o.sdk_dir.is_absolute()) problems.push_back("The SDK folder must be an absolute path.");
	if (o.install_designer && (o.designer_dir.empty() || !o.designer_dir.is_absolute()))
		problems.push_back("The Raven Designer folder must be an absolute path.");
	const std::vector<std::string> needs_designer = {"desktop_shortcut", "menu_shortcut", "app_registration", "shell_open"};
	for (const auto& i : o.integrations) {
		if (contains(needs_designer, i) && !o.install_designer) problems.push_back("\"" + i + "\" needs Raven Designer.");
		if (i == "shell_new_project" && !o.install_sdk) problems.push_back("New Raven Project needs the SDK (its templates).");
	}
	const Json& targets = payload_.meta()["targets"];
	for (const auto& t : o.targets) {
		bool known = false;
		for (const auto& entry : targets.elements())
			if (entry["id"].as_string() == t) known = true;
		if (!known) problems.push_back("This installer has no package \"" + t + "\".");
	}
	// folders inside each other would mix the files of the two components
	if (o.install_sdk && o.install_designer && !o.sdk_dir.empty() && !o.designer_dir.empty()) {
		const std::string a = path_key(o.sdk_dir) + "/", b = path_key(o.designer_dir) + "/";
		if (a == b) problems.push_back("The SDK and Raven Designer need two different folders.");
	}
	return problems;
}

InstallReport Installer::install(const InstallOptions& o, const ProgressFn& progress, const std::atomic<bool>& cancel) {
	InstallReport report;
	const Json& meta = payload_.meta();
	const std::string version = meta["version"].as_string();
	std::error_code ec;

	auto fail = [&](const std::string& why, Journal& journal) {
		log_.error(why);
		log_.info("undoing the changes");
		journal.rollback(log_);
		report.ok = false;
		report.error = why;
		return report;
	};

	for (const auto& p : validate(o)) {
		report.error = p;
		log_.error(p);
		return report;
	}

	// the previous installation, in this folder
	Manifest previous;
	bool has_previous = false;
	if (fs::exists(manifest_path(o.sdk_dir), ec)) {
		std::string why;
		has_previous = Manifest::load(manifest_path(o.sdk_dir), previous, &why);
		if (!has_previous) log_.warning("the manifest of the previous installation cannot be read: " + why);
	}
	std::map<std::string, const InstalledFile*> previous_files;
	if (has_previous) {
		log_.info("previous installation: " + previous.version + " (" + previous.installed_at + ")");
		for (const auto& f : previous.files) previous_files[path_key(path_from_utf8(f.path))] = &f;
	}

	const std::vector<Planned> planned = plan(o);
	std::uint64_t total = 0;
	for (const auto& p : planned) total += p.size;
	log_.info("installing " + std::to_string(planned.size()) + " files (" + format_size(total) + ") of " +
		meta["product"].as_string() + " " + version);

	Journal journal;
	InstallProgress state;
	state.bytes_total = total;
	state.files_total = static_cast<int>(planned.size());
	auto report_progress = [&](const std::string& component, const std::string& operation) {
		state.component = component;
		state.operation = operation;
		state.fraction = total ? 0.9 * static_cast<double>(state.bytes_done) / static_cast<double>(total) : 0.9;
		if (progress) progress(state);
	};

	// 1. checks
	report_progress("", "Checking the folders");
	std::vector<fs::path> roots;
	roots.push_back(o.sdk_dir); // the manifest always lives in the SDK folder
	if (o.install_designer) roots.push_back(o.designer_dir);
	for (const fs::path& root : roots) {
		std::vector<fs::path> made;
		std::string why;
		if (!create_directories_tracked(root, made, &why)) return fail(why, journal);
		for (const auto& d : made) journal.entries.push_back({Journal::Entry::CreatedDir, d, {}, {}, {}});
		if (!platform::check_writable(root, &why)) return fail(why + ". Choose another folder, or run the installer as an administrator.", journal);
	}
	{
		std::map<std::string, std::uint64_t> need;
		for (const auto& p : planned) need[path_to_utf8(p.root)] += p.size;
		for (const auto& n : need) {
			const std::uint64_t free = platform::free_space(path_from_utf8(n.first));
			if (free && free < n.second + (16u << 20))
				return fail("Not enough room in " + n.first + ": " + format_size(n.second) + " needed, " + format_size(free) + " free.", journal);
		}
	}

	Manifest manifest;
	manifest.product = meta["product"].as_string();
	manifest.version = version;
	manifest.platform = meta["platform"].as_string();
	manifest.installed_at = has_previous && !previous.installed_at.empty() ? previous.installed_at : now_iso8601();
	manifest.updated_at = now_iso8601();
	manifest.sdk_dir = path_to_utf8(o.sdk_dir);
	manifest.designer_dir = o.install_designer ? path_to_utf8(o.designer_dir) : std::string();
	if (o.install_sdk) manifest.components.push_back("sdk");
	if (o.install_designer) manifest.components.push_back("designer");
	manifest.targets = o.install_sdk ? o.targets : std::vector<std::string>();
	if (has_previous)
		for (const auto& d : previous.directories) manifest.directories.push_back(d);
	auto remember_dirs = [&](const std::vector<fs::path>& made) {
		for (const auto& d : made) {
			journal.entries.push_back({Journal::Entry::CreatedDir, d, {}, {}, {}});
			if (!contains(manifest.directories, path_to_utf8(d))) manifest.directories.push_back(path_to_utf8(d));
		}
	};
	for (const fs::path& root : roots)
		if (!contains(manifest.directories, path_to_utf8(root)) && !has_previous) manifest.directories.push_back(path_to_utf8(root));

	// 2. files
	std::set<std::string> written;
	std::vector<std::uint8_t> data;
	for (const Planned& p : planned) {
		if (cancel.load()) {
			report.cancelled = true;
			return fail("The installation was cancelled.", journal);
		}
		report_progress(p.component, path_to_utf8(p.dest.filename()));
		std::string why;
		if (!p.file->link.empty()) {
			// a symbolic link of a package (libraven.so -> libraven.so.0)
			written.insert(path_key(p.dest));
			std::vector<fs::path> made;
			if (!create_directories_tracked(p.dest.parent_path(), made, &why)) return fail(why, journal);
			remember_dirs(made);
			const fs::file_status st = fs::symlink_status(p.dest, ec);
			if (!(fs::is_symlink(st) && path_to_generic_utf8(fs::read_symlink(p.dest, ec)) == p.file->link)) {
				if (fs::exists(st)) {
					const fs::path backup = journal.backup_path(p.root);
					fs::create_directories(backup.parent_path(), ec);
					fs::rename(p.dest, backup, ec);
					if (ec) return fail("cannot replace " + path_to_utf8(p.dest) + ": " + ec.message(), journal);
					journal.entries.push_back({Journal::Entry::ReplacedFile, p.dest, backup, {}, {}});
				} else {
					journal.entries.push_back({Journal::Entry::CreatedFile, p.dest, {}, {}, {}});
				}
				fs::create_symlink(path_from_utf8(p.file->link), p.dest, ec);
				if (ec) return fail("cannot make the link " + path_to_utf8(p.dest) + ": " + ec.message(), journal);
			}
			InstalledFile entry;
			entry.path = path_to_utf8(p.dest);
			entry.component = p.component;
			entry.link = p.file->link;
			manifest.files.push_back(entry);
			state.files_done++;
			continue;
		}
		if (!payload_.read_blob(p.file->blob, data, &why)) return fail(why, journal);
		const std::uint32_t crc = payload_.blob(p.file->blob).crc;
		const std::string key = path_key(p.dest);
		written.insert(key);

		std::vector<fs::path> made;
		if (!create_directories_tracked(p.dest.parent_path(), made, &why)) return fail(why, journal);
		remember_dirs(made);

		bool write = true;
		if (fs::exists(p.dest, ec)) {
			std::uint32_t old_crc = 0;
			std::uint64_t old_size = 0;
			file_crc32(p.dest, old_crc, old_size);
			if (old_crc == crc && old_size == data.size()) {
				write = false; // already the right content (repair, or the same version again)
			} else {
				auto prev = previous_files.find(key);
				const bool ours_unchanged = prev != previous_files.end() && prev->second->crc == old_crc;
				if (ours_unchanged) {
					const fs::path backup = journal.backup_path(p.root);
					fs::create_directories(backup.parent_path(), ec);
					fs::rename(p.dest, backup, ec);
					if (ec) return fail("cannot replace " + path_to_utf8(p.dest) + ": " + ec.message() + " (is the program running?)", journal);
					journal.entries.push_back({Journal::Entry::ReplacedFile, p.dest, backup, {}, {}});
				} else {
					// a file of the user (or one the user changed): kept beside, never lost
					fs::path aside = p.dest;
					aside += ".user-" + now_iso8601().substr(0, 10);
					for (int n = 2; fs::exists(aside, ec); n++) {
						aside = p.dest;
						aside += ".user-" + now_iso8601().substr(0, 10) + "-" + std::to_string(n);
					}
					fs::rename(p.dest, aside, ec);
					if (ec) return fail("cannot move " + path_to_utf8(p.dest) + " aside: " + ec.message(), journal);
					// a rollback puts it back; a commit leaves it beside
					journal.entries.push_back({Journal::Entry::ReplacedFile, p.dest, aside, {}, {}});
					report.preserved.push_back(path_to_utf8(aside));
					log_.warning("kept the file of the user " + path_to_utf8(p.dest) + " as " + path_to_utf8(aside));
				}
			}
		}
		if (write) {
			if (!write_file_atomic(p.dest, data.data(), data.size(), &why)) return fail(why, journal);
			if (journal.entries.empty() || journal.entries.back().path != p.dest)
				journal.entries.push_back({Journal::Entry::CreatedFile, p.dest, {}, {}, {}});
		}
		if (p.file->executable) platform::set_executable(p.dest);
		manifest.files.push_back({path_to_utf8(p.dest), data.size(), crc, p.component, {}});
		state.bytes_done += p.size;
		state.files_done++;
	}

	// 3. files of the previous installation that are not part of this one
	if (has_previous) {
		for (const auto& f : previous.files) {
			const fs::path path = path_from_utf8(f.path);
			if (written.count(path_key(path)) || f.component == "maintenance") continue;
			const bool is_link = fs::is_symlink(fs::symlink_status(path, ec));
			if (!is_link && !fs::exists(path, ec)) continue;
			if (!f.link.empty()) {
				if (!is_link || path_to_generic_utf8(fs::read_symlink(path, ec)) != f.link) continue;
			} else {
				std::uint32_t crc = 0;
				std::uint64_t size = 0;
				if (is_link || !file_crc32(path, crc, size) || crc != f.crc) {
					log_.warning("kept " + f.path + ": changed since it was installed");
					continue;
				}
			}
			const fs::path root = starts_with(path_key(path), path_key(o.sdk_dir)) ? o.sdk_dir : path.parent_path();
			const fs::path backup = journal.backup_path(root);
			fs::create_directories(backup.parent_path(), ec);
			fs::rename(path, backup, ec);
			if (ec) {
				log_.warning("cannot remove " + f.path + ": " + ec.message());
				continue;
			}
			journal.entries.push_back({Journal::Entry::ReplacedFile, path, backup, {}, {}});
		}
	}

	// 4. the maintenance program: this program without its payload
	report_progress("maintenance", "Maintenance program");
	const fs::path state_dir = install_state_dir(o.sdk_dir);
	{
		std::vector<fs::path> made;
		std::string why;
		if (!create_directories_tracked(state_dir, made, &why)) return fail(why, journal);
		remember_dirs(made);
	}
	const fs::path maintenance = state_dir / maintenance_name();
	{
		std::string why;
		std::vector<std::pair<fs::path, fs::path>> copies; // source, destination
		std::uint64_t stub = 0;
		const fs::path bundle = self_.parent_path().parent_path().parent_path();
#if defined(__APPLE__)
		const bool in_bundle = self_.parent_path().filename() == "MacOS";
#else
		const bool in_bundle = false;
#endif
		if (in_bundle) {
			for (fs::recursive_directory_iterator it(bundle, ec), end; it != end; it.increment(ec)) {
				if (!it->is_regular_file() || it->path().filename() == "payload.rvnpk") continue;
				copies.push_back({it->path(), maintenance / it->path().lexically_relative(bundle)});
			}
		} else {
			copies.push_back({self_, maintenance});
			if (path_key(payload_.source()) == path_key(self_)) stub = payload_.stub_size();
		}
		for (const auto& c : copies) {
			std::vector<std::uint8_t> bytes;
			if (!read_file(c.first, bytes, &why)) return fail(why, journal);
			if (stub && c.first == self_) bytes.resize(static_cast<std::size_t>(stub));
			std::vector<fs::path> made;
			create_directories_tracked(c.second.parent_path(), made);
			remember_dirs(made);
			if (fs::exists(c.second, ec)) {
				const fs::path backup = journal.backup_path(o.sdk_dir);
				fs::create_directories(backup.parent_path(), ec);
				fs::rename(c.second, backup, ec);
				if (ec) return fail("cannot replace " + path_to_utf8(c.second) + ": " + ec.message(), journal);
				journal.entries.push_back({Journal::Entry::ReplacedFile, c.second, backup, {}, {}});
			} else {
				journal.entries.push_back({Journal::Entry::CreatedFile, c.second, {}, {}, {}});
			}
			if (!write_file_atomic(c.second, bytes.data(), bytes.size(), &why)) return fail(why, journal);
			platform::set_executable(c.second);
			manifest.files.push_back({path_to_utf8(c.second), bytes.size(), crc32(bytes.data(), bytes.size()), "maintenance", {}});
		}
	}
	manifest.maintenance = path_to_utf8(maintenance);

	// 5. integrations
	IntegrationContext context;
	context.version = version;
	context.sdk_dir = o.sdk_dir;
	const Json& designer = meta["designer"];
	context.designer = o.designer_dir / path_from_utf8(designer["launch"].as_string());
	if (designer["icon"].is_string()) context.icon = o.designer_dir / path_from_utf8(designer["icon"].as_string());
	if (o.install_designer) manifest.designer = path_to_utf8(context.designer);
	context.new_project_tool = o.sdk_dir / path_from_utf8(meta["tools"]["new_project"].as_string());
	context.maintenance = maintenance;
#if defined(__APPLE__)
	context.maintenance = maintenance / "Contents" / "MacOS" / self_.filename();
#endif
	for (const auto& t : meta["templates"].elements()) context.templates.push_back({t["id"].as_string(), t["name"].as_string()});
	context.installed_size = total;
	std::vector<std::string> integrations = o.integrations;
	if (integration_availability("uninstall_entry").available && !contains(integrations, "uninstall_entry"))
		integrations.push_back("uninstall_entry");
	for (const auto& kind : integrations) {
		if (cancel.load()) {
			report.cancelled = true;
			return fail("The installation was cancelled.", journal);
		}
		const IntegrationAvailability a = integration_availability(kind);
		if (!a.available) {
			log_.warning(kind + " is not available here: " + a.detail);
			report.warnings.push_back(kind + ": " + a.detail);
			continue;
		}
		report_progress("integration", kind);
		Json undo;
		std::string why;
		const Json* previous_undo = nullptr;
		if (has_previous)
			for (const auto& old : previous.actions)
				if (old.kind == kind) previous_undo = &old.undo;
		detail::set_previous(previous_undo);
		const bool applied = integration_apply(kind, context, undo, &why);
		detail::set_previous(nullptr);
		if (!applied) {
			// what was half made is undone; the installation goes on
			integration_undo(kind, undo, nullptr);
			log_.warning(kind + " failed: " + why);
			report.warnings.push_back(kind + ": " + why);
			continue;
		}
		log_.info(kind + ": done");
		journal.entries.push_back({Journal::Entry::Action, {}, {}, kind, undo});
		manifest.actions.push_back({kind, undo});
	}

	// 6. the manifest: the commit
	report_progress("", "Saving the manifest");
	{
		std::string why;
		if (!manifest.save(manifest_path(o.sdk_dir), &why)) return fail(why, journal);
	}
	journal.cleanup();
	// integrations of the previous installation that were not made again
	if (has_previous) {
		for (const auto& old : previous.actions) {
			bool again = false;
			for (const auto& a : manifest.actions)
				if (a.kind == old.kind) again = true;
			if (again) continue;
			std::string why;
			if (!integration_undo(old.kind, old.undo, &why)) log_.warning("cannot undo " + old.kind + ": " + why);
		}
	}
	// the pointer that finds this installation again
	Json pointer = Json::object();
	pointer.set("sdk_dir", path_to_utf8(o.sdk_dir));
	pointer.set("version", version);
	write_text_file(install_pointer_path(), pointer.dump(1) + "\n");

	state.fraction = 1.0;
	state.operation = "Done";
	if (progress) progress(state);
	log_.info("installation done");
	report.ok = true;
	report.manifest = manifest;
	return report;
}

// ---------------------------------------------------------------------------
//  Uninstall
// ---------------------------------------------------------------------------

InstallReport uninstall(const Manifest& manifest, Logger& log, const ProgressFn& progress) {
	InstallReport report;
	InstallProgress state;
	state.files_total = static_cast<int>(manifest.files.size());
	std::error_code ec;
	log.info("uninstalling " + manifest.product + " " + manifest.version + " from " + manifest.sdk_dir);

	for (auto it = manifest.actions.rbegin(); it != manifest.actions.rend(); ++it) {
		state.operation = it->kind;
		if (progress) progress(state);
		std::string why;
		if (!integration_undo(it->kind, it->undo, &why)) {
			log.warning("cannot undo " + it->kind + ": " + why);
			report.warnings.push_back(it->kind + ": " + why);
		} else {
			log.info(it->kind + ": removed");
		}
	}
	const fs::path self = platform::executable_path();
	std::set<std::string> parents;
	for (const auto& f : manifest.files) {
		const fs::path path = path_from_utf8(f.path);
		state.files_done++;
		state.fraction = 0.95 * state.files_done / std::max(1, state.files_total);
		state.operation = path_to_utf8(path.filename());
		if (progress) progress(state);
		parents.insert(path_to_utf8(path.parent_path()));
		if (!f.link.empty()) {
			if (fs::is_symlink(fs::symlink_status(path, ec)) && path_to_generic_utf8(fs::read_symlink(path, ec)) == f.link) fs::remove(path, ec);
			continue;
		}
		if (!fs::exists(path, ec)) continue;
		if (path_key(path) == path_key(self)) continue; // the program running: removed after it exits
		std::uint32_t crc = 0;
		std::uint64_t size = 0;
		if (f.component != "maintenance" && (!file_crc32(path, crc, size) || crc != f.crc)) {
			log.warning("kept " + f.path + ": changed since it was installed");
			report.preserved.push_back(f.path);
			continue;
		}
		if (!fs::remove(path, ec) || ec) {
			log.warning("cannot remove " + f.path + ": " + ec.message());
			report.warnings.push_back("cannot remove " + f.path);
		}
	}
	// the state of the installation (the log goes with it)
	const fs::path state_dir = install_state_dir(path_from_utf8(manifest.sdk_dir));
	fs::remove(state_dir / "manifest.json", ec);
	log.close_file();
	fs::remove(state_dir / "install.log", ec);
	// folders: the ones the installer made, then the now empty folders that held its files, deepest first
	std::vector<std::string> dirs(parents.begin(), parents.end());
	for (const auto& d : manifest.directories) dirs.push_back(d);
	std::sort(dirs.begin(), dirs.end(), [](const std::string& a, const std::string& b) { return a.size() > b.size(); });
	const std::string sdk_key = path_key(path_from_utf8(manifest.sdk_dir));
	const std::string designer_key = manifest.designer_dir.empty() ? std::string("\x01") : path_key(path_from_utf8(manifest.designer_dir));
	for (const auto& d : dirs) {
		const fs::path dir = path_from_utf8(d);
		// walk up while empty, but never above the folders of the installation
		for (fs::path p = dir; !p.empty(); p = p.parent_path()) {
			const std::string key = path_key(p);
			const bool inside = key == sdk_key || starts_with(key, sdk_key + "/") || key == designer_key ||
				starts_with(key, designer_key + "/") || contains(manifest.directories, path_to_utf8(p));
			if (!inside || !remove_dir_if_empty(p)) break;
			if (p == p.parent_path()) break;
		}
	}
	std::string text;
	Json pointer;
	if (read_text_file(install_pointer_path(), text) && Json::parse(text, pointer) &&
		path_key(path_from_utf8(pointer["sdk_dir"].as_string())) == sdk_key)
		fs::remove(install_pointer_path(), ec);
	state.fraction = 1.0;
	state.operation = "Done";
	if (progress) progress(state);
	report.ok = true;
	return report;
}

} // namespace rinst
