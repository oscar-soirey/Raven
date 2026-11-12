// Raven SDK Installer - project templates, in one place for everything that
// makes a project: the "New Raven Project" menu of the file manager (through
// raven-new), and Raven Designer (through the C interface raven_templates.h).
//
// A template is a folder of <SDK>/templates/<id>/ with a template.json:
//
//   id, name, order, description
//   needs        "any", "shared" (a binding that loads the shared library) or "static"
//   binding      the folder of bindings/ it uses: the template is only offered
//                when the SDK has it
//   extends      folders whose files are added first ("_cmake")
//   vars         variables of its own ({{CMAKE_LANGUAGE}})
//   copy         files copied from the SDK into the project ({"from", "to"})
//   env          environment for the run scripts; library_path: the folder of
//                the library is added to PATH / LD_LIBRARY_PATH / DYLD_LIBRARY_PATH
//   run, run_unix, run_windows   the command of run.sh / run.bat
//   readme       the "Build and run" part of README.md
//
// Every project also gets the files of _common/ (main.xml, strings.csv,
// README.md). A file ending in .in has its {{VARIABLES}} replaced and loses
// the suffix; __PROJECT_IDENT__ in a file name is replaced too.
#pragma once

#include "../core/json.hpp"
#include "../core/util.hpp"

#include <map>
#include <string>
#include <vector>

namespace rinst {

struct TemplateInfo {
	std::string id;
	std::string name;
	std::string description;
	std::string needs;
	std::string binding;
	int order = 100;
	fs::path dir;
	Json spec;
};

/** What a project needs to know about the installed SDK. */
struct SdkLayout {
	fs::path sdk_dir;
	std::string version;
	std::string platform;       // "windows-x64", ...
	std::vector<std::string> targets; // installed packages: "msvc-static", "gnu-shared", ...
	std::string static_target;  // the one projects use by default ("" when none)
	std::string shared_target;  // the one the bindings load ("" when none)
	fs::path templates_dir;

	fs::path prefix(const std::string& target) const { return sdk_dir / target; }
	fs::path bindings_dir() const;
	/** raven.dll / libraven.so / libraven.dylib of shared_target. */
	fs::path shared_library() const;
	/** The folder to link the shared library from (import library on Windows). */
	fs::path link_dir() const;
	/** The folder of the shared library at run time. */
	fs::path runtime_dir() const;
	bool has_binding(const std::string& binding) const;
};

/** Reads <sdk>/.raven-install/manifest.json, or looks at the folders when there is none. */
bool describe_sdk(const fs::path& sdk_dir, SdkLayout& out, std::string* error);

class TemplateCatalog {
public:
	bool load(const fs::path& templates_dir, std::string* error);
	const std::vector<TemplateInfo>& templates() const { return templates_; }
	const TemplateInfo* find(const std::string& id) const;
	/** Why a template cannot be used with this SDK; "" when it can. */
	std::string unavailable_reason(const TemplateInfo& info, const SdkLayout& sdk) const;

private:
	fs::path dir_;
	std::vector<TemplateInfo> templates_;
};

struct ProjectResult {
	fs::path project_dir;
	std::vector<fs::path> files;
	std::string run_hint;
};

/**
 * Makes a project from a template. `parent` is the folder that was
 * right-clicked: when it is empty, the project is made in it; otherwise in a
 * new folder `name` (or name2, name3... when taken). Never overwrites a file.
 */
bool create_project(const TemplateCatalog& catalog, const std::string& template_id, const fs::path& parent,
	const std::string& name, const SdkLayout& sdk, ProjectResult& result, std::string* error);

/** "My App!" -> "my_app" (an identifier for every language). */
std::string project_identifier(const std::string& name);
/** "my app" -> "MyApp" */
std::string project_class_name(const std::string& name);

} // namespace rinst
