// Raven SDK Installer - C interface of the project templates (see raven_templates.h).
#include "raven_templates.h"

#include "templates.hpp"

#include <cstring>

namespace {
rinst::SdkLayout g_sdk;
rinst::TemplateCatalog g_catalog;

void copy_out(const std::string& text, char* out, size_t size) {
	if (!out || size == 0) return;
	const size_t n = text.size() < size - 1 ? text.size() : size - 1;
	std::memcpy(out, text.data(), n);
	out[n] = '\0';
}

const rinst::TemplateInfo* at(int index) {
	const auto& list = g_catalog.templates();
	return index >= 0 && static_cast<size_t>(index) < list.size() ? &list[static_cast<size_t>(index)] : nullptr;
}
} // namespace

extern "C" {

int raven_templates_load(const char* sdk_dir) {
	if (!sdk_dir || !rinst::describe_sdk(rinst::path_from_utf8(sdk_dir), g_sdk, nullptr)) return 0;
	g_catalog.load(g_sdk.templates_dir, nullptr);
	return static_cast<int>(g_catalog.templates().size());
}

const char* raven_template_id(int index) {
	const auto* t = at(index);
	return t ? t->id.c_str() : nullptr;
}

const char* raven_template_name(int index) {
	const auto* t = at(index);
	return t ? t->name.c_str() : nullptr;
}

const char* raven_template_description(int index) {
	const auto* t = at(index);
	return t ? t->description.c_str() : nullptr;
}

int raven_template_available(int index) {
	const auto* t = at(index);
	return t && g_catalog.unavailable_reason(*t, g_sdk).empty() ? 1 : 0;
}

int raven_project_create(const char* template_id, const char* parent_dir, const char* name, char* out_dir, size_t out_size,
	char* error, size_t error_size) {
	rinst::ProjectResult result;
	std::string why;
	if (!template_id || !parent_dir) {
		copy_out("no template or folder", error, error_size);
		return 0;
	}
	if (!rinst::create_project(g_catalog, template_id, rinst::path_from_utf8(parent_dir), name ? name : "", g_sdk, result, &why)) {
		copy_out(why, error, error_size);
		return 0;
	}
	copy_out(rinst::path_to_utf8(result.project_dir), out_dir, out_size);
	return 1;
}

} // extern "C"
