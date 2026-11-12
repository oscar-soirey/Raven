// Raven SDK Installer - the session (see session.hpp).
#include "session.hpp"

#include "../shell/integration.hpp"

#include <algorithm>
#include <map>

namespace rinst {

std::string Session::family_label(const std::string& family) const {
	if (family == "msvc") return "MSVC (Visual C++)";
	if (family == "mingw") return "MinGW-w64 (GCC)";
	if (family == "gnu") return "GCC and Clang (Linux x64, glibc)";
	if (family == "apple") return "Apple Clang (universal: arm64 + x86_64)";
	return family;
}

void Session::load() {
	self = platform::executable_path();
	host = platform::host_info();
	has_payload = payload.open_self(self, &payload_error);
	targets.clear();
	if (has_payload) {
		std::map<std::string, std::uint64_t> sizes;
		for (const auto& f : payload.files()) {
			if (!starts_with(f.path, "sdk/")) continue;
			const std::string rest = f.path.substr(4);
			sizes[rest.substr(0, rest.find('/'))] += payload.blob(f.blob).size;
		}
		for (const auto& t : payload.meta()["targets"].elements()) {
			TargetInfo info;
			info.id = t["id"].as_string();
			info.family = t["family"].as_string();
			info.kind = t["kind"].as_string();
			info.label = family_label(info.family) + (info.kind == "shared" ? " - shared library" : " - static library");
			info.detail = t["detail"].as_string();
			info.size = sizes[info.id];
			targets.push_back(info);
		}
	}
	existing = find_existing_install(platform::default_sdk_dir());

	auto add = [&](const std::string& kind, const std::string& label, bool designer, bool sdk) {
		IntegrationInfo i;
		i.kind = kind;
		i.label = label;
		i.availability = integration_availability(kind);
		i.needs_designer = designer;
		i.needs_sdk = sdk;
		integrations.push_back(i);
	};
	integrations.clear();
	add("desktop_shortcut", "Desktop shortcut", true, false);
#if defined(_WIN32)
	add("menu_shortcut", "Start menu shortcuts", true, false);
#else
	add("menu_shortcut", "Applications menu entry", true, false);
#endif
	add("app_registration", "Register Raven Designer with the system", true, false);
#if defined(_WIN32)
	add("shell_open", "\"Open with Raven Designer\" in the context menu of folders (File Explorer)", true, false);
	add("shell_new_project", "\"New Raven Project\" in the context menu of the inside of folders (File Explorer)", false, true);
#else
	add("shell_open", "\"Open with Raven Designer\" in the context menu of folders", true, false);
	add("shell_new_project", "\"New Raven Project\" in the context menu of folders", false, true);
#endif
}

std::string Session::product() const {
	return has_payload ? payload.meta()["product"].string_or("RavenGUI SDK") : "RavenGUI SDK";
}

std::string Session::version() const {
	return has_payload ? payload.meta()["version"].as_string() : existing.manifest.version;
}

bool Session::platform_matches(std::string* why) const {
	if (!has_payload) {
		if (why) *why = payload_error;
		return false;
	}
	const std::string platform = payload.meta()["platform"].as_string();
	// a macOS payload is universal: it is for both kinds of Mac
	const bool ok = platform == host.platform_id() || (platform == "macos-universal" && host.os == "macos");
	if (!ok && why) *why = "This installer is for " + platform + "; this machine is " + host.platform_id() + ".";
	return ok;
}

InstallOptions Session::default_options(const CompilerDetection* detection) const {
	InstallOptions o;
	o.sdk_dir = platform::default_sdk_dir();
	o.designer_dir = platform::default_designer_dir();
	if (existing.found) {
		o.sdk_dir = existing.sdk_dir;
		if (!existing.manifest.designer_dir.empty()) o.designer_dir = path_from_utf8(existing.manifest.designer_dir);
		o.install_sdk = existing.manifest.has_component("sdk");
		o.install_designer = existing.manifest.has_component("designer") || !existing.manifest.has_component("sdk");
	}
	// packages: the previous selection; else those of the compilers found; else all of them
	std::vector<std::string> families = detection ? detection->families() : std::vector<std::string>();
	for (const auto& t : targets) {
		bool on;
		if (existing.found && !existing.manifest.targets.empty())
			on = std::find(existing.manifest.targets.begin(), existing.manifest.targets.end(), t.id) != existing.manifest.targets.end();
		else if (detection && !families.empty())
			on = std::find(families.begin(), families.end(), t.family) != families.end();
		else
			on = true;
		if (on) o.targets.push_back(t.id);
	}
	// the bindings of the other languages load a shared library: keep one
	bool shared = false;
	for (const auto& id : o.targets)
		if (ends_with(id, "-shared")) shared = true;
	if (!shared)
		for (const auto& t : targets)
			if (t.kind == "shared") {
				o.targets.push_back(t.id);
				break;
			}
	if (existing.found) {
		for (const auto& a : existing.manifest.actions)
			if (a.kind != "uninstall_entry") o.integrations.push_back(a.kind);
	} else {
		for (const auto& i : integrations)
			if (i.availability.available) o.integrations.push_back(i.kind);
	}
	return o;
}

const TargetInfo* Session::target(const std::string& id) const {
	for (const auto& t : targets)
		if (t.id == id) return &t;
	return nullptr;
}

fs::path Session::log_path(const fs::path& sdk_dir) const {
	return install_state_dir(sdk_dir) / "install.log";
}

fs::path Session::self_package() const {
	if (self.parent_path().filename() == "MacOS") return self.parent_path().parent_path().parent_path();
	return self;
}

} // namespace rinst
