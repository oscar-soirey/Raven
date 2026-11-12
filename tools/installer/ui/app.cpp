// Raven SDK Installer - the wizard (see app.hpp).
#include "app.hpp"

#include "resources.hpp"

#include <algorithm>
#include <cstdio>

namespace rinst {

namespace {

struct Color {
	float r, g, b;
};
const Color kAccent{0.890f, 0.024f, 0.106f};
const Color kWhite{0.93f, 0.94f, 0.96f};
const Color kGray{0.353f, 0.373f, 0.420f};
const Color kAmber{1.0f, 0.706f, 0.329f};
const Color kGreen{0.239f, 0.839f, 0.549f};
const Color kRed{1.0f, 0.353f, 0.373f};

void task_trampoline(void* data) {
	auto* task = static_cast<std::function<void()>*>(data);
	(*task)();
	delete task;
}

Rid create_row(const char* file) {
	return Rwidget_create_from_xml_file(file);
}

void set_badge(Rid row, const Color& c) {
	Rborder_set_background(Rwidget_find(row, "badge"), c.r, c.g, c.b, 1.0f);
}

bool contains(const std::vector<std::string>& list, const std::string& item) {
	return std::find(list.begin(), list.end(), item) != list.end();
}

} // namespace

InstallerApp::InstallerApp(Session& session, const CommandLine& args) : s_(session), args_(args) {}

InstallerApp::~InstallerApp() {
	cancel_ = true;
	if (detect_thread_.joinable()) detect_thread_.join();
	if (work_thread_.joinable()) work_thread_.join();
	s_.log.set_sink(nullptr);
}

void InstallerApp::post(std::function<void()> task) {
	Rtask_post(task_trampoline, new std::function<void()>(std::move(task)));
}

Rid InstallerApp::find(const char* name) const {
	return Rwidget_find(window_, name);
}

void InstallerApp::set_text(const char* name, const std::string& text) {
	const Rid w = find(name);
	if (w == R_INVALID_ID) return;
	if (Rwidget_get_type(w) == R_WIDGET_BUTTON) Rbutton_set_text(w, text.c_str());
	else Rlabel_set_text(w, text.c_str());
}

void InstallerApp::set_visible(const char* name, bool visible) {
	Rwidget_set_visibility(find(name), visible ? R_VISIBLE : R_COLLAPSED);
}

// ---------------------------------------------------------------------------
//  Creation
// ---------------------------------------------------------------------------

bool InstallerApp::create() {
	for (std::size_t i = 0; i < resources::kResourceCount; i++) {
		const auto& r = resources::kResources[i];
		Rfile_register_memory(r.path, r.data, r.size, R_FALSE);
	}
	window_ = Rwindow_create_from_xml_file("ui/main.xml");
	if (window_ == R_INVALID_ID) return false;
	pages_ = find("pages");
	Rwindow_set_close_callback(window_, on_close, this);
	for (const char* a : {"nav.next", "nav.back", "nav.cancel", "location.sdk.browse", "location.designer.browse", "env.rescan",
			 "done.open.sdk", "done.open.log"})
		Raction_bind(a, on_action, this);
	Rtext_input_set_changed_callback(find("location.sdk"), on_text_changed, this);
	Rtext_input_set_changed_callback(find("location.designer"), on_text_changed, this);
	Rcheckbox_set_changed_callback(find("comp.sdk"), on_check_changed, this);
	Rcheckbox_set_changed_callback(find("comp.designer"), on_check_changed, this);
	Rcheckbox_set_changed_callback(find("mode.update"), on_check_changed, this);
	Rcheckbox_set_changed_callback(find("mode.uninstall"), on_check_changed, this);

	defaults_ = s_.default_options(nullptr);
	set_text("sidebar.version", s_.version().empty() ? "" : "Version " + s_.version());
	set_text("sidebar.platform", s_.host.os_name + "\n" + s_.host.platform_id());
	fill_welcome();
	fill_location();
	set_text("env.os", s_.host.os_name);
	set_text("env.arch", s_.host.arch + (s_.host.translated ? " (this program runs translated)" : ""));
	build_targets();
	build_options();
	Rcheckbox_set_checked(find("comp.sdk"), defaults_.install_sdk);
	Rcheckbox_set_checked(find("comp.designer"), defaults_.install_designer);
	refresh_components();
	if (args_.test_gui) Rset_frame_callback(on_frame, this);
	show_page(Welcome);
	return true;
}

void InstallerApp::fill_welcome() {
	std::string why;
	const bool ok = s_.platform_matches(&why);
	if (s_.has_payload) {
		set_text("welcome.subtitle", "Version " + s_.version() + " for " + s_.host.os_name);
		std::uint64_t total = 0;
		for (const auto& f : s_.payload.files()) total += s_.payload.blob(f.blob).size;
		set_text("welcome.payload", std::to_string(s_.payload.files().size()) + " files, " + format_size(total) +
			" once installed. Nothing is downloaded.");
	} else {
		set_text("welcome.subtitle", "Maintenance");
	}
	if (s_.existing.found) {
		const Manifest& m = s_.existing.manifest;
		set_visible("welcome.existing", true);
		const int cmp = s_.has_payload ? compare_versions(s_.version(), m.version) : 0;
		std::string title = "Raven SDK " + m.version + " is installed";
		set_text("welcome.existing.title", title);
		std::string detail = "In " + path_to_utf8(s_.existing.sdk_dir) + ", since " + m.installed_at.substr(0, 10) + ". ";
		if (!s_.has_payload) detail += "To update or repair it, run a newer Raven SDK installer.";
		else if (cmp > 0) detail += "This installer updates it to " + s_.version() + "; the files you made in its folders are kept.";
		else if (cmp == 0) detail += "This installer can repair it: the files that were changed or removed are put back.";
		else detail += "This installer has an older version (" + s_.version() + "): installing it replaces the newer one.";
		set_text("welcome.existing.detail", detail);
		Rcheckbox_set_text(find("mode.update"), (cmp > 0 ? "Update to " + s_.version() : cmp == 0 ? std::string("Repair or modify") : "Install " + s_.version() + " over it").c_str());
		if (!s_.has_payload || !ok) {
			Rwidget_set_enabled(find("mode.update"), R_FALSE);
			Rcheckbox_set_checked(find("mode.uninstall"), R_TRUE);
		}
		if (args_.uninstall || args_.maintenance) Rcheckbox_set_checked(find("mode.uninstall"), R_TRUE);
	}
	if (!ok && !s_.existing.found) set_text("footer.status", why);
}

void InstallerApp::fill_location() {
	Rtext_input_set_text(find("location.sdk"), path_to_utf8(defaults_.sdk_dir).c_str());
	Rtext_input_set_text(find("location.designer"), path_to_utf8(defaults_.designer_dir).c_str());
#if defined(__APPLE__)
	set_text("location.designer.hint", "Raven Designer.app is installed in this folder.");
#elif defined(_WIN32)
	set_text("location.designer.hint", "The program and its icon. The Start menu and the context menus point to it.");
#else
	set_text("location.designer.hint", "The program and its icon. The entry of the applications menu points to it.");
#endif
	const bool locked = s_.existing.found;
	Rtext_input_set_read_only(find("location.sdk"), locked);
	Rwidget_set_enabled(find("location.sdk.browse"), !locked);
	if (locked && !s_.existing.manifest.designer_dir.empty()) {
		Rtext_input_set_read_only(find("location.designer"), R_TRUE);
		Rwidget_set_enabled(find("location.designer.browse"), R_FALSE);
	}
	set_visible("location.locked", locked);
}

void InstallerApp::build_targets() {
	const Rid list = find("env.targets");
	for (const auto& t : s_.targets) {
		const Rid row = create_row("ui/row_target.xml");
		if (row == R_INVALID_ID) continue;
		Rwidget_add_child(list, row);
		const Rid check = Rwidget_find(row, "check");
		Rcheckbox_set_text(check, t.label.c_str());
		Rcheckbox_set_checked(check, contains(defaults_.targets, t.id));
		Rcheckbox_set_changed_callback(check, on_check_changed, this);
		Rlabel_set_text(Rwidget_find(row, "detail"), t.detail.c_str());
		Rlabel_set_text(Rwidget_find(row, "size"), format_size(t.size).c_str());
		target_rows_.push_back({t.id, row, check});
	}
	if (s_.targets.empty()) {
		const Rid row = create_row("ui/row_target.xml");
		Rwidget_add_child(list, row);
		Rwidget_set_visibility(Rwidget_find(row, "check"), R_COLLAPSED);
		Rlabel_set_text(Rwidget_find(row, "detail"), "This installer has no library package.");
	}
}

void InstallerApp::build_options() {
	const Rid list = find("comp.options");
	for (const auto& i : s_.integrations) {
		const Rid row = create_row("ui/row_option.xml");
		if (row == R_INVALID_ID) continue;
		Rwidget_add_child(list, row);
		const Rid check = Rwidget_find(row, "check");
		Rcheckbox_set_text(check, i.label.c_str());
		Rcheckbox_set_checked(check, i.availability.available && contains(defaults_.integrations, i.kind));
		Rlabel_set_text(Rwidget_find(row, "detail"), (i.availability.available ? i.availability.detail : "Not available: " + i.availability.detail).c_str());
		option_rows_.push_back({i.kind, row, check});
	}
}

void InstallerApp::refresh_components() {
	const bool sdk = Rcheckbox_is_checked(find("comp.sdk"));
	const bool designer = Rcheckbox_is_checked(find("comp.designer"));
	for (std::size_t k = 0; k < option_rows_.size(); k++) {
		const IntegrationInfo& info = s_.integrations[k];
		const bool usable = info.availability.available && (!info.needs_designer || designer) && (!info.needs_sdk || sdk);
		Rwidget_set_enabled(option_rows_[k].check, usable);
		if (!usable) Rcheckbox_set_checked(option_rows_[k].check, R_FALSE);
	}
	for (const auto& row : target_rows_) Rwidget_set_enabled(row.row, sdk);
	const InstallOptions o = current_options();
	std::uint64_t size = 0;
	for (const auto& t : o.targets)
		if (const TargetInfo* info = s_.target(t)) size += info->size;
	set_text("comp.sdk.detail", std::to_string(o.targets.size()) + " library package" + (o.targets.size() == 1 ? "" : "s") +
		" (" + format_size(size) + "), the bindings of every language, and the project templates.");
}

InstallOptions InstallerApp::current_options() const {
	InstallOptions o;
	o.sdk_dir = path_from_utf8(trim(Rtext_input_get_text(find("location.sdk")) ? Rtext_input_get_text(find("location.sdk")) : ""));
	o.designer_dir = path_from_utf8(trim(Rtext_input_get_text(find("location.designer")) ? Rtext_input_get_text(find("location.designer")) : ""));
	o.install_sdk = Rcheckbox_is_checked(find("comp.sdk"));
	o.install_designer = Rcheckbox_is_checked(find("comp.designer"));
	if (o.install_sdk)
		for (const auto& row : target_rows_)
			if (Rcheckbox_is_checked(row.check)) o.targets.push_back(row.id);
	for (const auto& row : option_rows_)
		if (Rcheckbox_is_checked(row.check) && Rwidget_is_enabled(row.check)) o.integrations.push_back(row.id);
	return o;
}

// ---------------------------------------------------------------------------
//  Navigation
// ---------------------------------------------------------------------------

void InstallerApp::show_page(int page) {
	page_ = page;
	Rswitcher_set_index(pages_, page);
	for (int i = 0; i < PageCount; i++) {
		char name[16];
		std::snprintf(name, sizeof(name), "step.%d", i);
		const Rid label = find(name);
		const bool current = i == page;
		const Color& c = current ? kAccent : kWhite;
		Rlabel_set_color(label, c.r, c.g, c.b, 1.0f);
		Rwidget_set_opacity(label, current ? 1.0f : (i < page ? 0.75f : 0.4f));
	}
	const Rid next_button = find("nav.next");
	const Rid back_button = find("nav.back");
	const Rid cancel_button = find("nav.cancel");
	Rwidget_set_visibility(back_button, page > Welcome && page < Progress ? R_VISIBLE : R_HIDDEN);
	Rwidget_set_visibility(cancel_button, page == Done ? R_HIDDEN : R_VISIBLE);
	Rwidget_set_enabled(next_button, R_TRUE);
	set_text("footer.status", "");
	std::string why;
	switch (page) {
		case Welcome: {
			const bool uninstall = s_.existing.found && Rcheckbox_is_checked(find("mode.uninstall"));
			Rbutton_set_text(next_button, uninstall ? "Uninstall" : s_.existing.found ? "Next" : "Install");
			const bool can = uninstall || s_.platform_matches(&why);
			Rwidget_set_enabled(next_button, can);
			if (!can) set_text("footer.status", why);
			break;
		}
		case Location:
			Rbutton_set_text(next_button, "Next");
			check_locations();
			break;
		case Environment:
			Rbutton_set_text(next_button, "Next");
			if (!detected_ && !detecting_) start_detection();
			Rwidget_set_enabled(next_button, detected_);
			if (detecting_) set_text("footer.status", "Detecting the compilers...");
			break;
		case Components:
			Rbutton_set_text(next_button, "Next");
			refresh_components();
			break;
		case Summary:
			fill_summary();
			break;
		case Progress:
			Rwidget_set_enabled(next_button, R_FALSE);
			Rbutton_set_text(next_button, "Next");
			break;
		case Done:
			Rbutton_set_text(next_button, "Finish");
			break;
	}
}

void InstallerApp::next() {
	switch (page_) {
		case Welcome:
			if (s_.existing.found && Rcheckbox_is_checked(find("mode.uninstall"))) {
				if (!args_.test_gui && !args_.yes) {
					const std::string question = "Uninstall the Raven SDK " + s_.existing.manifest.version + " from " +
						path_to_utf8(s_.existing.sdk_dir) + "?\n\nThe files you made or changed in its folders are kept.";
					if (Rmessage_box(window_, "Uninstall", question.c_str(), R_MESSAGE_YES_NO) != R_MESSAGE_RESULT_YES) return;
				}
				start_uninstall();
				return;
			}
			show_page(Location);
			break;
		case Location: {
			const InstallOptions o = current_options();
			if (o.sdk_dir.empty() || !o.sdk_dir.is_absolute() || (o.install_designer && (o.designer_dir.empty() || !o.designer_dir.is_absolute()))) {
				set_text("footer.status", "Choose two absolute folders.");
				return;
			}
			show_page(Environment);
			break;
		}
		case Environment:
			if (!detected_) return;
			show_page(Components);
			break;
		case Components:
			show_page(Summary);
			break;
		case Summary:
			start_install();
			break;
		case Done: {
			const Manifest& m = mode_ == Mode::Install ? report_.manifest : s_.existing.manifest;
			if (mode_ == Mode::Install && report_.ok && Rcheckbox_is_checked(find("done.launch")) && !m.designer.empty()) {
#if defined(__APPLE__)
				platform::launch_detached({"/usr/bin/open", m.designer});
#else
				platform::launch_detached({m.designer}, path_from_utf8(m.designer).parent_path());
#endif
			}
			Rquit(exit_code_);
			break;
		}
		default:
			break;
	}
}

void InstallerApp::back() {
	if (page_ > Welcome && page_ < Progress) show_page(page_ - 1);
}

void InstallerApp::cancel() {
	if (working_) {
		if (mode_ == Mode::Uninstall) return; // an uninstallation half done is worse than one finished
		if (args_.test_gui || Rmessage_box(window_, "Cancel", "Stop the installation? What was installed is removed.", R_MESSAGE_YES_NO) == R_MESSAGE_RESULT_YES) {
			cancel_ = true;
			set_text("progress.operation", "Cancelling: undoing what was done...");
		}
		return;
	}
	if (page_ == Done || args_.test_gui ||
		Rmessage_box(window_, "Raven SDK Installer", "Quit the installer? Nothing was installed.", R_MESSAGE_YES_NO) == R_MESSAGE_RESULT_YES)
		Rquit(page_ == Done ? exit_code_ : 1);
}

// ---------------------------------------------------------------------------
//  Callbacks
// ---------------------------------------------------------------------------

void InstallerApp::on_action(Rid, const char* name, void* self) {
	static_cast<InstallerApp*>(self)->action(name ? name : "");
}

void InstallerApp::action(const std::string& name) {
	if (name == "nav.next") next();
	else if (name == "nav.back") back();
	else if (name == "nav.cancel") cancel();
	else if (name == "env.rescan") {
		if (!detecting_) start_detection();
	} else if (name == "location.sdk.browse" || name == "location.designer.browse") {
		const bool sdk = name == "location.sdk.browse";
		const Rid input = find(sdk ? "location.sdk" : "location.designer");
		const std::string current = Rtext_input_get_text(input) ? Rtext_input_get_text(input) : "";
		RFileDialogDesc desc{};
		desc.parent = window_;
		desc.title = sdk ? "Folder of the RavenGUI SDK" : "Folder of Raven Designer";
		fs::path start = path_from_utf8(current);
		std::error_code ec;
		while (!start.empty() && !fs::is_directory(start, ec) && start != start.parent_path()) start = start.parent_path();
		const std::string start_text = path_to_utf8(start);
		desc.directory = start_text.empty() ? nullptr : start_text.c_str();
		if (const char* chosen = Rfile_dialog_select_folder(&desc)) {
			fs::path p = path_from_utf8(chosen);
			// a folder chosen for the SDK gets a folder of its own inside, unless it is empty
			if (sdk && !fs::is_empty(p, ec) && p.filename() != "SDK" && p.filename() != "sdk") p /= "RavenSDK";
			Rtext_input_set_text(input, path_to_utf8(p).c_str());
			check_locations();
		}
	} else if (name == "done.open.sdk") {
		platform::open_with_system(path_to_utf8(current_options().sdk_dir));
	} else if (name == "done.open.log") {
		platform::open_with_system(path_to_utf8(log_file_));
	}
}

void InstallerApp::on_text_changed(Rid, const char*, void* self) {
	static_cast<InstallerApp*>(self)->check_locations();
}

void InstallerApp::on_check_changed(Rid checkbox, int, void* self) {
	auto* app = static_cast<InstallerApp*>(self);
	if (checkbox == app->find("mode.update") || checkbox == app->find("mode.uninstall")) {
		if (app->page_ == Welcome) app->show_page(Welcome);
		return;
	}
	for (const auto& row : app->target_rows_)
		if (row.check == checkbox) app->targets_touched_ = true;
	app->refresh_components();
}

int InstallerApp::on_close(Rid, void* self) {
	auto* app = static_cast<InstallerApp*>(self);
	if (app->working_) {
		app->cancel();
		return R_FALSE;
	}
	Rquit(app->page_ == Done ? app->exit_code_ : 1);
	return R_TRUE;
}

void InstallerApp::on_frame(double, void* self) {
	static_cast<InstallerApp*>(self)->test_step();
}

// ---------------------------------------------------------------------------
//  Location
// ---------------------------------------------------------------------------

void InstallerApp::check_locations() {
	if (page_ != Location) return;
	const InstallOptions o = current_options();
	bool ok = true;
	auto check = [&](const fs::path& dir, const char* status, bool needed) {
		if (!needed) {
			set_text(status, "");
			return;
		}
		if (dir.empty() || !dir.is_absolute()) {
			set_text(status, "Give an absolute path.");
			ok = false;
			return;
		}
		// the nearest folder that exists: the installer makes the others
		std::error_code ec;
		fs::path existing = dir;
		while (!existing.empty() && !fs::exists(existing, ec) && existing != existing.parent_path()) existing = existing.parent_path();
		std::string why;
		if (!fs::is_directory(existing, ec)) {
			set_text(status, path_to_utf8(existing) + " is a file, not a folder.");
			ok = false;
		} else if (!platform::check_writable(existing, &why)) {
			set_text(status, "Cannot be written: " + why);
			ok = false;
		} else {
			const std::uint64_t free = platform::free_space(existing);
			set_text(status, std::string(fs::exists(dir, ec) ? "The folder exists and can be written" : "The folder will be created") +
				(free ? " - " + format_size(free) + " free." : "."));
		}
	};
	check(o.sdk_dir, "location.sdk.status", true);
	check(o.designer_dir, "location.designer.status", o.install_designer);
	if (o.install_designer && !o.sdk_dir.empty() && path_key(o.sdk_dir) == path_key(o.designer_dir)) {
		set_text("location.designer.status", "Choose a folder other than the one of the SDK.");
		ok = false;
	}
	Rwidget_set_enabled(find("nav.next"), ok);
}

// ---------------------------------------------------------------------------
//  Compilers
// ---------------------------------------------------------------------------

void InstallerApp::start_detection() {
	if (detect_thread_.joinable()) detect_thread_.join();
	detecting_ = true;
	set_text("env.detecting", "Looking for compilers and building a test program with each...");
	set_visible("env.detecting", true);
	set_visible("env.detecting.bar", true);
	Rwidget_set_enabled(find("env.rescan"), R_FALSE);
	if (page_ == Environment) Rwidget_set_enabled(find("nav.next"), R_FALSE);
	const bool verify = !args_.no_verify;
	detect_thread_ = std::thread([this, verify]() {
		CompilerDetection d = detect_compilers(verify);
		post([this, d]() {
			detection_ = d;
			detecting_ = false;
			detected_ = true;
			fill_detection();
		});
	});
}

void InstallerApp::fill_detection() {
	for (const auto& line : detection_.log) s_.log.info("compilers: " + line);
	set_visible("env.detecting.bar", false);
	Rwidget_set_enabled(find("env.rescan"), R_TRUE);
	const Rid list = find("env.compilers");
	Rwidget_clear_children(list);
	int usable = 0;
	for (const auto& c : detection_.compilers) {
		const Rid row = create_row("ui/row_compiler.xml");
		if (row == R_INVALID_ID) continue;
		Rwidget_add_child(list, row);
		Rlabel_set_text(Rwidget_find(row, "name"), (c.name.empty() ? c.path : c.name).c_str());
		std::string detail = c.path;
		if (!c.machine.empty()) detail += "  [" + c.machine + "]";
		if (!c.note.empty()) detail += "\n" + c.note;
		Rlabel_set_text(Rwidget_find(row, "detail"), detail.c_str());
		Rlabel_set_text(Rwidget_find(row, "state"), c.verified ? "Tested" : c.usable ? "Found" : "Not usable");
		set_badge(row, c.verified || c.usable ? kGreen : kRed);
		if (c.usable) usable++;
	}
	set_text("env.detecting", detection_.compilers.empty()
		? "No C compiler was found. You can still install the SDK: the libraries of every compiler can be selected below."
		: std::to_string(usable) + " of " + std::to_string(detection_.compilers.size()) + " compilers can build programs with Raven.");
	// the packages of the compilers found, unless the user chose already
	const InstallOptions with = s_.default_options(&detection_);
	for (const auto& row : target_rows_) {
		const TargetInfo* t = s_.target(row.id);
		if (!targets_touched_) Rcheckbox_set_checked(row.check, contains(with.targets, row.id));
		std::string found;
		for (const auto& c : detection_.compilers)
			if (c.usable && target_family_of(c.family) == t->family) found += (found.empty() ? "" : ", ") + c.name;
		const std::string detail = t->detail + (found.empty() ? "\nNo compiler of this kind was found on this machine." : "\nFor " + found + ".");
		Rlabel_set_text(Rwidget_find(row.row, "detail"), detail.c_str());
	}
	targets_touched_ = false;
	refresh_components();
	if (page_ == Environment) {
		Rwidget_set_enabled(find("nav.next"), R_TRUE);
		set_text("footer.status", "");
	}
}

// ---------------------------------------------------------------------------
//  Summary
// ---------------------------------------------------------------------------

void InstallerApp::fill_summary() {
	const InstallOptions o = current_options();
	Installer installer(s_.payload, s_.log, s_.self);
	std::vector<std::string> components;
	if (o.install_sdk) components.push_back("RavenGUI SDK");
	if (o.install_designer) components.push_back("Raven Designer");
	set_text("summary.components", components.empty() ? "Nothing" : join(components, ", "));
	std::vector<std::string> targets;
	for (const auto& id : o.targets)
		if (const TargetInfo* t = s_.target(id)) targets.push_back(t->label);
	set_text("summary.targets", o.install_sdk ? (targets.empty() ? "None" : join(targets, "\n")) : "-");
	std::vector<std::string> compilers;
	for (const auto& c : detection_.compilers)
		if (c.usable) compilers.push_back(c.name);
	set_text("summary.compilers", compilers.empty() ? "None found" : join(compilers, "\n"));
	set_text("summary.sdk", path_to_utf8(o.sdk_dir));
	set_text("summary.designer", o.install_designer ? path_to_utf8(o.designer_dir) : "-");
	std::vector<std::string> integrations;
	for (const auto& i : s_.integrations)
		if (contains(o.integrations, i.kind)) integrations.push_back(i.label);
	set_text("summary.integrations", integrations.empty() ? "None" : join(integrations, "\n"));
	set_text("summary.size", format_size(installer.estimate_size(o)));

	std::string mode;
	if (!s_.existing.found) {
		set_text("summary.title", "Ready to install");
		mode = "A new installation.";
	} else {
		const int cmp = compare_versions(s_.version(), s_.existing.manifest.version);
		set_text("summary.title", cmp > 0 ? "Ready to update" : cmp == 0 ? "Ready to repair" : "Ready to install an older version");
		mode = cmp > 0 ? "Raven SDK " + s_.existing.manifest.version + " is updated to " + s_.version() + "."
		     : cmp == 0 ? "The files of Raven SDK " + s_.version() + " are checked and put back where they were changed or removed."
		                : "Raven SDK " + s_.existing.manifest.version + " is replaced by " + s_.version() + ".";
		mode += " Files you made in these folders are kept.";
	}
	set_text("summary.mode", mode);
	const std::vector<std::string> problems = installer.validate(o);
	set_visible("summary.problems", !problems.empty());
	set_text("summary.problems", join(problems, "\n"));
	const Rid next_button = find("nav.next");
	Rbutton_set_text(next_button, s_.existing.found ? (compare_versions(s_.version(), s_.existing.manifest.version) == 0 ? "Repair" : "Update") : "Install");
	Rwidget_set_enabled(next_button, problems.empty());
}

// ---------------------------------------------------------------------------
//  Installation and uninstallation
// ---------------------------------------------------------------------------

void InstallerApp::set_status_row(int index, const std::string& state, int level) {
	if (index < 0 || index >= static_cast<int>(status_rows_.size())) return;
	const Rid row = status_rows_[static_cast<std::size_t>(index)];
	Rlabel_set_text(Rwidget_find(row, "state"), state.c_str());
	set_badge(row, level == 0 ? kGray : level == 1 ? kAmber : level == 2 ? kGreen : kRed);
}

void InstallerApp::flush_worker_updates() {
	InstallProgress progress;
	bool has_progress;
	std::vector<std::string> lines;
	{
		std::lock_guard<std::mutex> lock(updates_mutex_);
		update_posted_ = false;
		has_progress = has_pending_progress_;
		has_pending_progress_ = false;
		progress = pending_progress_;
		lines.swap(pending_log_);
	}
	if (has_progress) {
		Rprogress_bar_set_value(find("progress.bar"), static_cast<float>(progress.fraction));
		if (!cancel_) set_text("progress.operation", progress.operation);
		if (progress.files_total)
			set_text("progress.detail", std::to_string(progress.files_done) + " / " + std::to_string(progress.files_total) + " files - " +
				format_size(progress.bytes_done) + " / " + format_size(progress.bytes_total));
		// the rows: the component being written is in progress, those before it are done
		const std::vector<std::string> order = {"sdk", "designer", "maintenance", "integration"};
		int current = -1;
		for (std::size_t i = 0; i < order.size(); i++)
			if (order[i] == progress.component) current = static_cast<int>(i);
		if (mode_ == Mode::Install && current >= 0) {
			const InstallOptions o = current_options();
			const bool selected[] = {o.install_sdk, o.install_designer, true, !o.integrations.empty()};
			for (int i = 0; i < 4; i++) {
				if (!selected[i]) set_status_row(i, "Not selected", 0);
				else if (i < current) set_status_row(i, "Done", 2);
				else if (i == current) set_status_row(i, "In progress", 1);
			}
		}
	}
	if (!lines.empty()) {
		for (const auto& l : lines) {
			log_text_ += l + "\n";
			log_lines_++;
		}
		// the last 400 lines (the whole log is in the file)
		while (log_lines_ > 400) {
			const std::size_t nl = log_text_.find('\n');
			if (nl == std::string::npos) break;
			log_text_.erase(0, nl + 1);
			log_lines_--;
		}
		const Rid editor = find("progress.log");
		Rtext_editor_set_text(editor, log_text_.c_str());
		Rtext_editor_scroll_to_line(editor, Rtext_editor_get_line_count(editor));
	}
}

void InstallerApp::start_install() {
	const InstallOptions o = current_options();
	mode_ = Mode::Install;
	cancel_ = false;
	working_ = true;
	log_file_ = s_.log_path(o.sdk_dir);
	set_text("progress.title", s_.existing.found ? "Updating" : "Installing");
	const Rid rows = find("progress.components");
	Rwidget_clear_children(rows);
	status_rows_.clear();
	for (const char* name : {"RavenGUI SDK", "Raven Designer", "Maintenance program", "Shortcuts and integration"}) {
		const Rid row = create_row("ui/row_status.xml");
		Rwidget_add_child(rows, row);
		Rlabel_set_text(Rwidget_find(row, "name"), name);
		status_rows_.push_back(row);
	}
	set_status_row(0, o.install_sdk ? "Waiting" : "Not selected", 0);
	set_status_row(1, o.install_designer ? "Waiting" : "Not selected", 0);
	set_status_row(2, "Waiting", 0);
	set_status_row(3, o.integrations.empty() ? "Not selected" : "Waiting", 0);
	show_page(Progress);

	auto schedule_flush = [this]() {
		// called with updates_mutex_ held: one task in the queue at a time
		if (!update_posted_) {
			update_posted_ = true;
			post([this]() { flush_worker_updates(); });
		}
	};
	s_.log.set_sink([this, schedule_flush](LogLevel level, const std::string& text) {
		std::lock_guard<std::mutex> lock(updates_mutex_);
		pending_log_.push_back((level == LogLevel::Error ? "error: " : level == LogLevel::Warning ? "warning: " : "") + text);
		schedule_flush();
	});
	s_.log.open_file(log_file_);
	for (const auto& line : detection_.log) s_.log.info("compiler detection: " + line);

	work_thread_ = std::thread([this, o, schedule_flush]() {
		Installer installer(s_.payload, s_.log, s_.self);
		InstallReport report = installer.install(o, [this, schedule_flush](const InstallProgress& p) {
			std::lock_guard<std::mutex> lock(updates_mutex_);
			pending_progress_ = p;
			has_pending_progress_ = true;
			schedule_flush();
		}, cancel_);
		post([this, report]() {
			flush_worker_updates();
			finish(report);
		});
	});
}

void InstallerApp::start_uninstall() {
	mode_ = Mode::Uninstall;
	working_ = true;
	log_file_ = s_.log_path(s_.existing.sdk_dir);
	set_text("progress.title", "Uninstalling");
	Rwidget_clear_children(find("progress.components"));
	status_rows_.clear();
	show_page(Progress);
	auto schedule_flush = [this]() {
		if (!update_posted_) {
			update_posted_ = true;
			post([this]() { flush_worker_updates(); });
		}
	};
	s_.log.set_sink([this, schedule_flush](LogLevel, const std::string& text) {
		std::lock_guard<std::mutex> lock(updates_mutex_);
		pending_log_.push_back(text);
		schedule_flush();
	});
	s_.log.open_file(log_file_);
	const Manifest manifest = s_.existing.manifest;
	work_thread_ = std::thread([this, manifest, schedule_flush]() {
		InstallReport report = uninstall(manifest, s_.log, [this, schedule_flush](const InstallProgress& p) {
			std::lock_guard<std::mutex> lock(updates_mutex_);
			pending_progress_ = p;
			has_pending_progress_ = true;
			schedule_flush();
		});
		post([this, report]() {
			flush_worker_updates();
			finish(report);
		});
	});
}

void InstallerApp::finish(const InstallReport& report) {
	if (work_thread_.joinable()) work_thread_.join();
	working_ = false;
	finished_ = true;
	report_ = report;
	exit_code_ = report.ok ? 0 : 1;
	s_.log.set_sink(nullptr);

	if (!report.ok) {
		// the error stays on the progress page, with the log
		set_visible("progress.error", true);
		set_text("progress.error.text", (report.cancelled ? "The installation was cancelled. " : "The installation failed: " + report.error + "\n") +
			"Everything it had done was undone; the log is in " + path_to_utf8(log_file_) + ".");
		set_text("progress.operation", report.cancelled ? "Cancelled" : "Failed");
		for (std::size_t i = 0; i < status_rows_.size(); i++) set_status_row(static_cast<int>(i), "Undone", 3);
		const Rid next_button = find("nav.next");
		Rbutton_set_text(next_button, "Close");
		Rwidget_set_enabled(next_button, R_TRUE);
		page_ = Done; // Close ends the program
		Rwidget_set_visibility(find("nav.cancel"), R_HIDDEN);
		if (args_.test_gui) Rquit(1);
		return;
	}

	const Rid list = find("done.components");
	Rwidget_clear_children(list);
	auto add = [&](const std::string& name, const std::string& detail, const std::string& state) {
		const Rid row = create_row("ui/row_status.xml");
		Rwidget_add_child(list, row);
		Rlabel_set_text(Rwidget_find(row, "name"), name.c_str());
		Rlabel_set_text(Rwidget_find(row, "state"), state.c_str());
		if (!detail.empty()) {
			Rlabel_set_text(Rwidget_find(row, "detail"), detail.c_str());
			Rwidget_set_visibility(Rwidget_find(row, "detail"), R_VISIBLE);
		}
		set_badge(row, kGreen);
	};
	if (mode_ == Mode::Install) {
		const Manifest& m = report.manifest;
		set_text("done.title", s_.existing.found ? "Raven SDK is up to date" : "Raven SDK is installed");
		std::string detail = "Raven SDK " + m.version + " is ready. A CMake project finds it when CMAKE_PREFIX_PATH names one of its packages";
		bool menu = false;
		for (const auto& a : m.actions)
			if (a.kind == "shell_new_project") menu = true;
		detail += menu ? "; \"New Raven Project\" in the context menu of a folder starts a project in any of its languages." : ".";
		set_text("done.detail", detail);
		if (m.has_component("sdk")) add("RavenGUI SDK", m.sdk_dir, "Installed");
		for (const auto& t : m.targets)
			if (const TargetInfo* info = s_.target(t)) add(info->label, path_to_utf8(path_from_utf8(m.sdk_dir) / t), "Installed");
		if (m.has_component("designer")) add("Raven Designer", m.designer, "Installed");
		for (const auto& a : m.actions)
			for (const auto& i : s_.integrations)
				if (i.kind == a.kind) add(i.label, "", "Done");
		Rwidget_set_visibility(find("done.launch"), m.has_component("designer") ? R_VISIBLE : R_COLLAPSED);
		if (!m.has_component("designer")) Rcheckbox_set_checked(find("done.launch"), R_FALSE);
	} else {
		set_text("done.title", "Raven SDK was uninstalled");
		set_text("done.detail", report.preserved.empty() ? "Everything the installer had put on this machine was removed."
		                                                 : "Everything was removed but the files you had changed, listed below.");
		for (const auto& p : report.preserved) add(path_to_utf8(path_from_utf8(p).filename()), p, "Kept");
		set_visible("done.next", false);
	}
	set_visible("done.box", Rwidget_get_child_count(list) > 0);
	std::vector<std::string> notes = report.warnings;
	for (const auto& p : report.preserved)
		if (mode_ == Mode::Install) notes.push_back("A file of yours was in the way and was kept as " + p);
	set_visible("done.warnings", !notes.empty());
	set_text("done.warnings", join(notes, "\n"));
	show_page(Done);
}

// ---------------------------------------------------------------------------
//  Run
// ---------------------------------------------------------------------------

/** --test-gui: walks the wizard by itself, as a user who accepts every default. */
void InstallerApp::capture(const std::string& name) {
	if (args_.screenshots.empty()) return;
	int width = 0, height = 0;
	uint8_t* pixels = Rwindow_capture(window_, &width, &height);
	if (!pixels) return;
	std::string data = "P6\n" + std::to_string(width) + " " + std::to_string(height) + "\n255\n";
	for (int i = 0; i < width * height; i++) data.append(reinterpret_cast<const char*>(pixels + i * 4), 3);
	Rpixels_free(pixels);
	write_text_file(path_from_utf8(args_.screenshots) / (name + ".ppm"), data);
}

void InstallerApp::test_step() {
	if (page_ != test_page_) {
		test_page_ = page_;
		test_ticks_ = 0;
	}
	test_ticks_++;
	// a picture of each page once it is drawn and its transitions are over
	const bool pictures = !args_.screenshots.empty();
	if (pictures && test_ticks_ == 36) Rwindow_request_redraw(window_);
	if (pictures && test_ticks_ == 40 && (page_ != Environment || detected_)) {
		char name[32];
		std::snprintf(name, sizeof(name), "page%d", page_);
		capture(name);
	}
	if (test_ticks_ < (pictures ? 41 : 3) || working_) {
		if (page_ == Environment && !detected_) test_ticks_ = 1;
		return;
	}
	switch (page_) {
		case Welcome:
		case Location:
		case Components:
		case Summary:
			if (Rwidget_is_enabled(find("nav.next"))) Raction_fire("nav.next", R_INVALID_ID);
			else if (test_ticks_ > 600) {
				std::fprintf(stderr, "test: stuck on page %d: %s\n", page_, Rlabel_get_text(find("footer.status")));
				Rquit(2);
			}
			break;
		case Environment:
			if (detected_) Raction_fire("nav.next", R_INVALID_ID);
			break;
		case Done:
			Rcheckbox_set_checked(find("done.launch"), R_FALSE);
			std::fprintf(stderr, "test: %s\n", report_.ok ? "finished" : report_.error.c_str());
			Raction_fire("nav.next", R_INVALID_ID);
			break;
		default:
			break;
	}
}

int InstallerApp::run() {
	while (Rwait_frame(args_.test_gui ? 0.01 : -1.0)) {}
	if (detect_thread_.joinable()) detect_thread_.join();
	if (work_thread_.joinable()) work_thread_.join();
	// the maintenance program removes itself after an uninstallation
	if (mode_ == Mode::Uninstall && report_.ok) {
		const fs::path state = install_state_dir(s_.existing.sdk_dir);
		if (starts_with(path_key(s_.self), path_key(state)))
			platform::delete_after_exit(s_.self_package(), {state, s_.existing.sdk_dir});
	}
	return exit_code_;
}

} // namespace rinst
