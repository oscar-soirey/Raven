// Raven SDK Installer - system integration on Windows (see integration.hpp).
//
// Everything is written for the current user (HKEY_CURRENT_USER, the user's
// Start menu) unless the installer runs elevated, where the uninstall entry
// goes to HKEY_LOCAL_MACHINE and the shortcuts to the common Start menu.
//
// The context menu entries are classic registry verbs. On Windows 11 they are
// in "Show more options" (Shift+F10): the first-level menu of Windows 11 only
// takes IExplorerCommand handlers of packaged applications.
#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <shobjidl.h>

#include "../platform/platform.hpp"
#include "../platform/win32_util.hpp"
#include "integration.hpp"

#include <ctime>

namespace rinst {

namespace {

const wchar_t* kUninstallKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\RavenSDK";
const wchar_t* kOpenKey = L"Software\\Classes\\Directory\\shell\\RavenDesigner.Open";
const wchar_t* kNewProjectKey = L"Software\\Classes\\Directory\\Background\\shell\\RavenDesigner.NewProject";
const wchar_t* kAppPathsKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\raven_designer.exe";
const wchar_t* kApplicationKey = L"Software\\Classes\\Applications\\raven_designer.exe";

std::wstring quoted(const fs::path& p) {
	return L"\"" + p.wstring() + L"\"";
}

fs::path known_folder(REFKNOWNFOLDERID id) {
	PWSTR path = nullptr;
	fs::path result;
	if (SUCCEEDED(SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &path)) && path) result = fs::path(path);
	CoTaskMemFree(path);
	return result;
}

HKEY root_for_machine_wide() {
	return platform::is_elevated() ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
}

const char* root_name(HKEY root) {
	return root == HKEY_LOCAL_MACHINE ? "HKLM" : "HKCU";
}

HKEY root_from_name(const std::string& name) {
	return name == "HKLM" ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
}

void record_key(Json& undo, HKEY root, const std::wstring& key) {
	Json keys = undo["registry"].is_array() ? undo["registry"] : Json::array();
	Json k = Json::object();
	k.set("root", root_name(root));
	k.set("key", win32::narrow(key));
	keys.push(std::move(k));
	undo.set("registry", std::move(keys));
}

/** COM for the time of a call, whatever the thread did before. */
struct ComScope {
	HRESULT hr;
	ComScope() : hr(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)) {}
	~ComScope() {
		if (SUCCEEDED(hr)) CoUninitialize();
	}
};

bool make_shortcut(Json& undo, const fs::path& link, const fs::path& target, const std::wstring& arguments, const std::wstring& description,
	const fs::path& icon, std::string* error) {
	ComScope com;
	IShellLinkW* shell_link = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&shell_link));
	if (FAILED(hr)) {
		if (error) *error = "cannot create a shortcut (COM error " + std::to_string(static_cast<long>(hr)) + ")";
		return false;
	}
	shell_link->SetPath(target.wstring().c_str());
	shell_link->SetArguments(arguments.c_str());
	shell_link->SetWorkingDirectory(target.parent_path().wstring().c_str());
	shell_link->SetDescription(description.c_str());
	shell_link->SetIconLocation((icon.empty() ? target : icon).wstring().c_str(), 0);
	IPersistFile* file = nullptr;
	hr = shell_link->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&file));
	std::vector<fs::path> made;
	create_directories_tracked(link.parent_path(), made);
	if (SUCCEEDED(hr)) {
		hr = file->Save(link.wstring().c_str(), TRUE);
		file->Release();
	}
	shell_link->Release();
	if (FAILED(hr)) {
		if (error) *error = "cannot save " + path_to_utf8(link);
		return false;
	}
	// a .lnk is binary: it is recorded without CRC (removed even if Windows updated it)
	Json files = undo["files"].is_array() ? undo["files"] : Json::array();
	Json f = Json::object();
	f.set("path", path_to_utf8(link));
	files.push(std::move(f));
	undo.set("files", std::move(files));
	if (!made.empty()) {
		Json dirs = undo["dirs"].is_array() ? undo["dirs"] : Json::array();
		for (const auto& d : made) dirs.push(path_to_utf8(d));
		undo.set("dirs", std::move(dirs));
	}
	return true;
}

void notify_shell() {
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}

bool set(HKEY root, const std::wstring& key, const std::wstring& name, const std::wstring& value, std::string* error) {
	if (win32::reg_set_string(root, key, name, value)) return true;
	if (error) *error = "cannot write the registry key " + win32::narrow(key);
	return false;
}

} // namespace

IntegrationAvailability integration_availability(const std::string& kind) {
	IntegrationAvailability a;
	a.available = true;
	if (kind == "app_registration") a.detail = "Raven Designer in \"Open with\" and the Run box (App Paths)";
	else if (kind == "desktop_shortcut") a.detail = "A shortcut on the desktop";
	else if (kind == "menu_shortcut") a.detail = "Raven \\ Raven Designer in the Start menu";
	else if (kind == "shell_open") a.detail = "\"Open with Raven Designer\" on folders (Windows 11: in \"Show more options\")";
	else if (kind == "shell_new_project") a.detail = "\"New Raven Project\" in the background of folders (Windows 11: in \"Show more options\")";
	else if (kind == "uninstall_entry") a.detail = "Raven SDK in Settings > Apps, to repair or uninstall";
	else a.available = false;
	return a;
}

bool integration_apply(const std::string& kind, const IntegrationContext& c, Json& undo, std::string* error) {
	undo = Json::object();
	const std::wstring designer = c.designer.wstring();
	const std::wstring icon = (c.icon.empty() ? c.designer : c.icon).wstring();
	if (kind == "desktop_shortcut") {
		const fs::path desktop = platform::is_elevated() ? known_folder(FOLDERID_PublicDesktop) : known_folder(FOLDERID_Desktop);
		return make_shortcut(undo, desktop / "Raven Designer.lnk", c.designer, L"", L"Make RavenGUI interfaces by looking at them", c.icon, error);
	}
	if (kind == "menu_shortcut") {
		const fs::path programs = platform::is_elevated() ? known_folder(FOLDERID_CommonPrograms) : known_folder(FOLDERID_Programs);
		if (!make_shortcut(undo, programs / "Raven" / "Raven Designer.lnk", c.designer, L"", L"Make RavenGUI interfaces by looking at them", c.icon, error))
			return false;
		return make_shortcut(undo, programs / "Raven" / "Raven SDK folder.lnk", c.sdk_dir, L"", L"The headers, libraries and bindings of RavenGUI", {}, error);
	}
	if (kind == "app_registration") {
		const HKEY root = HKEY_CURRENT_USER;
		record_key(undo, root, kAppPathsKey);
		if (!set(root, kAppPathsKey, L"", designer, error)) return false;
		set(root, kAppPathsKey, L"Path", c.designer.parent_path().wstring(), error);
		record_key(undo, root, kApplicationKey);
		set(root, kApplicationKey, L"FriendlyAppName", L"Raven Designer", error);
		set(root, std::wstring(kApplicationKey) + L"\\DefaultIcon", L"", icon, error);
		set(root, std::wstring(kApplicationKey) + L"\\shell\\open\\command", L"", quoted(c.designer) + L" \"%1\"", error);
		set(root, std::wstring(kApplicationKey) + L"\\SupportedTypes", L".xml", L"", error);
		notify_shell();
		return true;
	}
	if (kind == "shell_open") {
		const HKEY root = HKEY_CURRENT_USER;
		record_key(undo, root, kOpenKey);
		if (!set(root, kOpenKey, L"", L"Open with Raven Designer", error)) return false;
		set(root, kOpenKey, L"Icon", icon, error);
		set(root, std::wstring(kOpenKey) + L"\\command", L"", quoted(c.designer) + L" \"%V\"", error);
		notify_shell();
		return true;
	}
	if (kind == "shell_new_project") {
		const HKEY root = HKEY_CURRENT_USER;
		record_key(undo, root, kNewProjectKey);
		if (!set(root, kNewProjectKey, L"MUIVerb", L"New Raven Project", error)) return false;
		set(root, kNewProjectKey, L"Icon", icon, error);
		// an empty SubCommands and a shell key below: a cascading menu
		set(root, kNewProjectKey, L"SubCommands", L"", error);
		int index = 1;
		for (const auto& t : c.templates) {
			wchar_t prefix[8];
			swprintf(prefix, 8, L"%02d", index++);
			const std::wstring verb = std::wstring(kNewProjectKey) + L"\\shell\\" + prefix + L"_" + win32::widen(t.first);
			set(root, verb, L"MUIVerb", win32::widen(t.second), error);
			set(root, verb + L"\\command", L"", quoted(c.new_project_tool) + L" --template " + win32::widen(t.first) + L" --dir \"%V\" --open", error);
		}
		notify_shell();
		return true;
	}
	if (kind == "uninstall_entry") {
		const HKEY root = root_for_machine_wide();
		record_key(undo, root, kUninstallKey);
		if (!set(root, kUninstallKey, L"DisplayName", L"Raven SDK " + win32::widen(c.version), error)) return false;
		set(root, kUninstallKey, L"DisplayVersion", win32::widen(c.version), error);
		set(root, kUninstallKey, L"Publisher", L"Raven", error);
		set(root, kUninstallKey, L"InstallLocation", c.sdk_dir.wstring(), error);
		set(root, kUninstallKey, L"DisplayIcon", icon, error);
		set(root, kUninstallKey, L"URLInfoAbout", L"https://github.com/oscar-soirey/Raven", error);
		set(root, kUninstallKey, L"UninstallString", quoted(c.maintenance) + L" --uninstall", error);
		set(root, kUninstallKey, L"QuietUninstallString", quoted(c.maintenance) + L" --uninstall --quiet", error);
		set(root, kUninstallKey, L"ModifyPath", quoted(c.maintenance) + L" --maintenance", error);
		std::time_t now = std::time(nullptr);
		std::tm local{};
		localtime_s(&local, &now);
		wchar_t date[16];
		wcsftime(date, 16, L"%Y%m%d", &local);
		set(root, kUninstallKey, L"InstallDate", date, error);
		win32::reg_set_dword(root, kUninstallKey, L"EstimatedSize", static_cast<unsigned long>(c.installed_size / 1024));
		win32::reg_set_dword(root, kUninstallKey, L"NoRepair", 1);
		return true;
	}
	if (error) *error = "unknown integration " + kind;
	return false;
}

bool integration_undo(const std::string& kind, const Json& undo, std::string* error) {
	bool ok = true;
	for (const auto& k : undo["registry"].elements()) {
		const std::wstring key = win32::widen(k["key"].as_string());
		// only the keys of the installer (a wrong manifest must not delete anything else)
		if (key.find(L"Raven") == std::wstring::npos && key.find(L"raven") == std::wstring::npos) continue;
		if (!win32::reg_delete_tree(root_from_name(k["root"].as_string()), key)) {
			ok = false;
			if (error) *error = "cannot remove the registry key " + k["key"].as_string();
		}
	}
	if (!detail::undo_files(undo, error)) ok = false;
	if (kind == "shell_open" || kind == "shell_new_project" || kind == "app_registration") notify_shell();
	return ok;
}

} // namespace rinst

#endif // _WIN32
