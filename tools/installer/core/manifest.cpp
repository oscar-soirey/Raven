// Raven SDK Installer - installation manifest (see manifest.hpp).
#include "manifest.hpp"

namespace rinst {

fs::path install_state_dir(const fs::path& sdk_dir) {
	return sdk_dir / ".raven-install";
}

fs::path manifest_path(const fs::path& sdk_dir) {
	return install_state_dir(sdk_dir) / "manifest.json";
}

std::string path_key(const fs::path& path) {
	std::string key = path_to_generic_utf8(path.lexically_normal());
#if defined(_WIN32) || defined(__APPLE__)
	key = to_lower(key);
#endif
	return key;
}

namespace {
Json strings(const std::vector<std::string>& list) {
	Json array = Json::array();
	for (const auto& s : list) array.push(s);
	return array;
}
std::vector<std::string> strings(const Json& array) {
	std::vector<std::string> list;
	for (const auto& item : array.elements())
		if (item.is_string()) list.push_back(item.as_string());
	return list;
}
} // namespace

Json Manifest::to_json() const {
	Json j = Json::object();
	j.set("format", 1);
	j.set("product", product);
	j.set("version", version);
	j.set("platform", platform);
	j.set("installed_at", installed_at);
	j.set("updated_at", updated_at);
	j.set("sdk_dir", sdk_dir);
	j.set("designer_dir", designer_dir);
	j.set("designer", designer);
	j.set("maintenance", maintenance);
	j.set("components", strings(components));
	j.set("targets", strings(targets));
	j.set("directories", strings(directories));
	Json actions_json = Json::array();
	for (const auto& a : actions) {
		Json item = Json::object();
		item.set("kind", a.kind);
		item.set("undo", a.undo);
		actions_json.push(std::move(item));
	}
	j.set("actions", std::move(actions_json));
	Json files_json = Json::array();
	for (const auto& f : files) {
		Json item = Json::object();
		item.set("path", f.path);
		item.set("size", f.size);
		item.set("crc", f.crc);
		item.set("component", f.component);
		if (!f.link.empty()) item.set("link", f.link);
		files_json.push(std::move(item));
	}
	j.set("files", std::move(files_json));
	return j;
}

bool Manifest::from_json(const Json& j, Manifest& m, std::string* error) {
	if (!j.is_object() || j["format"].as_int() != 1) {
		if (error) *error = "not a manifest of the Raven SDK Installer";
		return false;
	}
	m = Manifest();
	m.product = j["product"].as_string();
	m.version = j["version"].as_string();
	m.platform = j["platform"].as_string();
	m.installed_at = j["installed_at"].as_string();
	m.updated_at = j["updated_at"].as_string();
	m.sdk_dir = j["sdk_dir"].as_string();
	m.designer_dir = j["designer_dir"].as_string();
	m.designer = j["designer"].as_string();
	m.maintenance = j["maintenance"].as_string();
	m.components = strings(j["components"]);
	m.targets = strings(j["targets"]);
	m.directories = strings(j["directories"]);
	for (const auto& a : j["actions"].elements()) m.actions.push_back({a["kind"].as_string(), a["undo"]});
	for (const auto& f : j["files"].elements()) {
		InstalledFile file;
		file.path = f["path"].as_string();
		file.size = f["size"].as_uint();
		file.crc = static_cast<std::uint32_t>(f["crc"].as_uint());
		file.component = f["component"].as_string();
		file.link = f["link"].as_string();
		if (!file.path.empty()) m.files.push_back(std::move(file));
	}
	return true;
}

bool Manifest::save(const fs::path& path, std::string* error) const {
	return write_text_file(path, to_json().dump(1) + "\n", error);
}

bool Manifest::load(const fs::path& path, Manifest& out, std::string* error) {
	std::string text;
	if (!read_text_file(path, text, error)) return false;
	Json j;
	std::string why;
	if (!Json::parse(text, j, &why)) {
		if (error) *error = path_to_utf8(path) + ": " + why;
		return false;
	}
	return from_json(j, out, error);
}

bool Manifest::has_component(const std::string& id) const {
	for (const auto& c : components)
		if (c == id) return true;
	return false;
}

const InstalledFile* Manifest::find_file(const std::string& path) const {
	const std::string key = path_key(path_from_utf8(path));
	for (const auto& f : files)
		if (path_key(path_from_utf8(f.path)) == key) return &f;
	return nullptr;
}

} // namespace rinst
