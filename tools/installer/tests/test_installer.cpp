// Raven SDK Installer - tests of the engine: compression, JSON, payload,
// installation (fresh, idempotent, repair, update, rollback), uninstallation,
// project templates. Nothing touches the real home of the user: HOME and the
// XDG folders point into the working folder given on the command line.
#include "compilers/compilers.hpp"
#include "core/deflate.hpp"
#include "core/engine.hpp"
#include "core/json.hpp"
#include "core/payload.hpp"
#include "platform/platform.hpp"
#include "templates/raven_templates.h"
#include "templates/templates.hpp"

#include <cstdio>
#include <cstring>
#include <random>

using namespace rinst;

#if defined(__APPLE__)
static const std::string kFamily = "apple";
static const char* const kSharedName = "libraven.dylib";
#else
static const std::string kFamily = "gnu";
static const char* const kSharedName = "libraven.so";
#endif
static const std::string kStatic = kFamily + "-static";
static const std::string kShared = kFamily + "-shared";

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(condition) \
	do { \
		g_checks++; \
		if (!(condition)) { \
			std::printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #condition); \
			g_failures++; \
		} \
	} while (0)

static std::vector<std::uint8_t> bytes(const std::string& text) {
	return std::vector<std::uint8_t>(text.begin(), text.end());
}

static std::string read(const fs::path& p) {
	std::string text;
	read_text_file(p, text);
	return text;
}

static void test_deflate() {
	std::mt19937 rng(42);
	std::vector<std::vector<std::uint8_t>> inputs;
	inputs.push_back({});
	inputs.push_back({'a'});
	inputs.push_back(bytes(std::string(100000, 'x')));
	std::vector<std::uint8_t> random(300000);
	for (auto& b : random) b = static_cast<std::uint8_t>(rng());
	inputs.push_back(random);
	std::string text;
	for (int i = 0; i < 20000; i++) text += "line " + std::to_string(i % 97) + " of a header file with Rwidget_find();\n";
	inputs.push_back(bytes(text));
	// a mix: compressible and not, long matches across block limits
	std::vector<std::uint8_t> mix;
	for (int k = 0; k < 40; k++) {
		mix.insert(mix.end(), random.begin() + k * 1000, random.begin() + k * 1000 + 3000);
		mix.insert(mix.end(), text.begin() + k * 50, text.begin() + k * 50 + 9000);
	}
	inputs.push_back(mix);
	for (int level : {1, 6, 9}) {
		for (const auto& in : inputs) {
			const auto packed = deflate_compress(in.data(), in.size(), level);
			std::vector<std::uint8_t> out(in.size());
			std::string why;
			const bool ok = inflate_exact(packed.data(), packed.size(), out.data(), out.size(), &why);
			if (!ok) std::printf("  inflate (level %d, %zu bytes): %s\n", level, in.size(), why.c_str());
			CHECK(ok);
			CHECK(out == in);
		}
	}
	const auto packed = deflate_compress(reinterpret_cast<const std::uint8_t*>(text.data()), text.size(), 9);
	CHECK(packed.size() < text.size() / 10);
	// a damaged stream is refused, never read past
	auto damaged = packed;
	damaged[damaged.size() / 2] ^= 0x55;
	std::vector<std::uint8_t> out(text.size());
	const bool ok = inflate_exact(damaged.data(), damaged.size(), out.data(), out.size());
	CHECK(!ok || std::memcmp(out.data(), text.data(), text.size()) != 0);
	std::vector<std::uint8_t> small(10);
	CHECK(!inflate_exact(packed.data(), packed.size(), small.data(), small.size()));
	std::printf("deflate: %zu -> %zu bytes (text)\n", text.size(), packed.size());
}

static void test_json() {
	Json j = Json::object();
	j.set("name", "Raven \"SDK\"\n\t\\ é");
	j.set("size", 1234567890123ull);
	j.set("crc", 0xFFFFFFFFu);
	Json list = Json::array();
	list.push(true);
	list.push(nullptr);
	list.push(-2.5);
	j.set("list", list);
	Json back;
	std::string why;
	CHECK(Json::parse(j.dump(2), back, &why));
	CHECK(back["name"].as_string() == "Raven \"SDK\"\n\t\\ é");
	CHECK(back["size"].as_uint() == 1234567890123ull);
	CHECK(back["crc"].as_uint() == 0xFFFFFFFFu);
	CHECK(back["list"][2].as_number() == -2.5);
	CHECK(back["list"][1].is_null());
	CHECK(back["missing"]["deeper"].is_null());
	Json unicode;
	CHECK(Json::parse("\"\\u00e9\\ud83d\\ude00\"", unicode));
	CHECK(unicode.as_string() == "\xC3\xA9\xF0\x9F\x98\x80");
	Json bad;
	CHECK(!Json::parse("{\"a\": }", bad));
	CHECK(!Json::parse("[1, 2", bad));
}

/** A fake package of Raven, as `cmake --install` writes it. */
static void make_package(const fs::path& prefix, const std::string& version, bool shared) {
	write_text_file(prefix / "include" / "Raven.h", "/* Raven " + version + " */\n");
	write_text_file(prefix / "include" / "raven" / "raven_core.h", "/* core */\n");
	write_text_file(prefix / "lib" / "cmake" / "raven" / "ravenConfig.cmake", "# config " + version + "\n");
	write_text_file(prefix / "lib" / (shared ? kSharedName : "libraven.a"), std::string(50000, shared ? 's' : 'a') + version);
	write_text_file(prefix / "share" / "raven" / "README.md", "readme\n");
	for (const char* b : {"python", "rust", "go", "csharp", "java", "zig", "odin", "luajit", "bun", "cpp"})
		write_text_file(prefix / "share" / "raven" / "bindings" / b / "binding.txt", b);
	write_text_file(prefix / "share" / "raven" / "bindings" / "zig" / "raven.zig", "// zig\n");
	write_text_file(prefix / "share" / "raven" / "bindings" / "cpp" / "raven.hpp", "// hpp\n");
	if (version == "0.1.0") write_text_file(prefix / "share" / "raven" / "old_only.txt", "removed in 0.2\n");
#if !defined(_WIN32)
	if (shared) {
		std::error_code ec;
		fs::create_symlink(kSharedName, prefix / "lib" / "libraven.so.0", ec);
	}
#endif
}

static fs::path make_installer(const fs::path& work, const std::string& version) {
	const fs::path pkg = work / ("pkg-" + version);
	make_package(pkg / kStatic, version, false);
	make_package(pkg / kShared, version, true);
	PayloadWriter w(6);
	std::string why;
	CHECK(w.add_tree("sdk/" + kStatic, pkg / kStatic, &why));
	CHECK(w.add_tree("sdk/" + kShared, pkg / kShared, &why));
	w.add("designer/raven_designer", bytes("#!/bin/sh\necho designer " + version + "\n"), true);
	w.add("sdk-bin/raven-new", bytes("raven-new"), true);
	CHECK(w.add_tree("templates", path_from_utf8(RAVEN_INSTALLER_TEMPLATES), &why));
	Json& m = w.meta();
	m.set("product", "RavenGUI SDK");
	m.set("version", version);
	m.set("platform", platform::host_info().platform_id());
	Json targets = Json::array();
	for (const std::string& id : {kStatic, kShared}) {
		Json t = Json::object();
		t.set("id", id);
		t.set("family", kFamily);
		t.set("kind", id.substr(id.find('-') + 1));
		targets.push(t);
	}
	m.set("targets", targets);
	Json designer = Json::object();
	designer.set("launch", "raven_designer");
	m.set("designer", designer);
	Json tools = Json::object();
	tools.set("new_project", "bin/raven-new");
	m.set("tools", tools);
	// a stub program before the payload, as in the real installer
	const fs::path stub = work / "stub.bin";
	write_text_file(stub, "STUB PROGRAM " + version);
	const fs::path out = work / ("installer-" + version);
	CHECK(w.write(out, stub, &why));
	// identical files are stored once (Raven.h of the two packages)
	Payload p;
	CHECK(p.open(out, &why));
	CHECK(p.blob_count() < p.files().size());
	CHECK(p.stub_size() == std::string("STUB PROGRAM " + version).size());
	return out;
}

static void test_payload(const fs::path& work) {
	const fs::path file = make_installer(work, "9.9.9");
	Payload p;
	std::string why;
	CHECK(p.open(file, &why));
	CHECK(p.meta()["version"].as_string() == "9.9.9");
	bool found = false;
	for (const auto& f : p.files()) {
		if (f.path != "sdk/" + kStatic + "/include/Raven.h") continue;
		std::vector<std::uint8_t> data;
		CHECK(p.read_blob(f.blob, data, &why));
		CHECK(std::string(data.begin(), data.end()) == "/* Raven 9.9.9 */\n");
		found = true;
	}
	CHECK(found);
	// a truncated installer is refused
	std::vector<std::uint8_t> all;
	read_file(file, all);
	all.resize(all.size() - 100);
	const fs::path truncated = work / "truncated";
	write_file_atomic(truncated, all.data(), all.size());
	Payload t;
	CHECK(!t.open(truncated, &why));
	// paths that leave the folder are refused
	PayloadWriter evil;
	evil.add("sdk/../../etc/passwd", bytes("x"), false);
	CHECK(evil.write(work / "evil", {}, &why));
	Payload e;
	CHECK(!e.open(work / "evil", &why));
}

static int count_files(const fs::path& dir) {
	int n = 0;
	std::error_code ec;
	for (fs::recursive_directory_iterator it(dir, ec), end; it != end; it.increment(ec))
		if (it->is_regular_file()) n++;
	return n;
}

#if !defined(_WIN32)
static void test_install(const fs::path& work) {
	const fs::path v1 = make_installer(work, "0.1.0");
	const fs::path v2 = make_installer(work, "0.2.0");
	const fs::path sdk = work / "home" / "sdk";
	const fs::path designer = work / "home" / "designer";
	Logger log;
	std::string why;
	std::atomic<bool> cancel{false};

	InstallOptions o;
	o.sdk_dir = sdk;
	o.designer_dir = designer;
	o.targets = {kStatic, kShared};
	o.integrations = {"menu_shortcut"};

	// 1. fresh install of 0.1.0
	Payload p1;
	CHECK(p1.open(v1, &why));
	{
		Installer installer(p1, log, v1);
		CHECK(installer.validate(o).empty());
		int calls = 0;
		InstallReport r = installer.install(o, [&](const InstallProgress&) { calls++; }, cancel);
		if (!r.ok) std::printf("  install: %s\n", r.error.c_str());
		CHECK(r.ok);
		CHECK(calls > 0);
	}
	CHECK(read(sdk / kStatic / "include" / "Raven.h") == "/* Raven 0.1.0 */\n");
	CHECK(fs::exists(designer / "raven_designer"));
	CHECK(fs::exists(sdk / "templates" / "python" / "template.json"));
	CHECK(fs::exists(sdk / ".raven-install" / "manifest.json"));
	// the maintenance program is the program without its payload
	CHECK(read(sdk / ".raven-install" / "RavenSDKMaintenance") == "STUB PROGRAM 0.1.0");
#if !defined(__APPLE__)
	CHECK(fs::exists(work / "home" / ".local" / "share" / "applications" / "raven-designer.desktop"));
#endif
	ExistingInstall e = find_existing_install();
	CHECK(e.found && e.manifest.version == "0.1.0");

	// 2. the same again: nothing changes; a removed file is put back (repair)
	fs::remove(sdk / kShared / "include" / "Raven.h");
	const fs::path user_file = sdk / kStatic / "include" / "my_notes.txt";
	write_text_file(user_file, "mine");
	write_text_file(sdk / kStatic / "include" / "raven" / "raven_core.h", "/* changed by the user */\n");
	{
		Installer installer(p1, log, v1);
		InstallReport r = installer.install(o, nullptr, cancel);
		CHECK(r.ok);
		CHECK(r.preserved.size() == 1); // raven_core.h, changed: kept beside
	}
	CHECK(fs::exists(sdk / kShared / "include" / "Raven.h"));
	CHECK(read(user_file) == "mine");
	CHECK(read(sdk / kStatic / "include" / "raven" / "raven_core.h") == "/* core */\n");

	// 3. an update that fails half way is undone: a folder where a file must go
	Payload p2;
	CHECK(p2.open(v2, &why));
	const fs::path blocker = sdk / kStatic / "lib" / "libraven.a";
	const std::string before = read(blocker);
	fs::create_directories(designer / "raven_designer.raven-tmp" / "x"); // the temporary file cannot be made
	{
		Installer installer(p2, log, v2);
		InstallReport r = installer.install(o, nullptr, cancel);
		CHECK(!r.ok);
	}
	CHECK(read(blocker) == before);
	CHECK(read(sdk / kStatic / "include" / "Raven.h") == "/* Raven 0.1.0 */\n");
	CHECK(!fs::exists(sdk / ".raven-txn"));
	fs::remove_all(designer / "raven_designer.raven-tmp");

	// 4. a cancelled update is undone too
	{
		std::atomic<bool> stop{false};
		Installer installer(p2, log, v2);
		int n = 0;
		InstallReport r = installer.install(o, [&](const InstallProgress&) {
			if (++n == 5) stop = true;
		}, stop);
		CHECK(!r.ok && r.cancelled);
	}
	CHECK(read(sdk / kStatic / "include" / "Raven.h") == "/* Raven 0.1.0 */\n");

	// 5. the update: new files, the file 0.2.0 no longer has is removed, the files of the user stay
	o.targets = {kShared};
	{
		Installer installer(p2, log, v2);
		InstallReport r = installer.install(o, nullptr, cancel);
		if (!r.ok) std::printf("  update: %s\n", r.error.c_str());
		CHECK(r.ok);
	}
	CHECK(read(sdk / kShared / "include" / "Raven.h") == "/* Raven 0.2.0 */\n");
	CHECK(fs::is_symlink(sdk / kShared / "lib" / "libraven.so.0"));
	CHECK(read(sdk / kShared / "lib" / "libraven.so.0") == read(sdk / kShared / "lib" / kSharedName));
	CHECK(!fs::exists(sdk / kShared / "share" / "raven" / "old_only.txt"));
	CHECK(!fs::exists(sdk / kStatic / "lib" / "libraven.a")); // no longer selected
	CHECK(read(user_file) == "mine");
	e = find_existing_install();
	CHECK(e.found && e.manifest.version == "0.2.0");
	CHECK(e.manifest.targets.size() == 1);

	// 6. projects from the templates of the installed SDK
	SdkLayout layout;
	CHECK(describe_sdk(sdk, layout, &why));
	CHECK(layout.shared_target == kShared);
	TemplateCatalog catalog;
	CHECK(catalog.load(layout.templates_dir, &why));
	CHECK(catalog.templates().size() >= 11);
	const fs::path projects = work / "projects";
	fs::create_directories(projects / "occupied");
	write_text_file(projects / "occupied" / "something.txt", "x");
	for (const auto& t : catalog.templates()) {
		ProjectResult result;
		const bool ok = create_project(catalog, t.id, projects / "occupied", "", layout, result, &why);
		if (!ok) std::printf("  template %s: %s\n", t.id.c_str(), why.c_str());
		CHECK(ok);
		CHECK(fs::exists(result.project_dir / "main.xml"));
		CHECK(fs::exists(result.project_dir / "strings.csv"));
		for (const auto& f : result.files) {
			const std::string content = read(f);
			if (content.find("{{") != std::string::npos) std::printf("  %s: a variable is left\n", path_to_utf8(f).c_str());
			CHECK(content.find("{{") == std::string::npos);
		}
	}
	// an empty folder is the project itself; the C interface makes the same projects
	fs::create_directories(projects / "empty");
	CHECK(raven_templates_load(path_to_utf8(sdk).c_str()) >= 11);
	char out[1024], error[512];
	CHECK(raven_project_create("c", path_to_utf8(projects / "empty").c_str(), nullptr, out, sizeof(out), error, sizeof(error)) == 1);
	CHECK(path_from_utf8(out) == projects / "empty");
	CHECK(read(projects / "empty" / "main.xml").find("Title=\"empty\"") != std::string::npos);
	CHECK(raven_project_create("c", path_to_utf8(projects / "empty").c_str(), "empty", out, sizeof(out), error, sizeof(error)) == 1);
	CHECK(path_from_utf8(out) == projects / "empty" / "empty");

	// 7. uninstall: everything of the installer goes, the files of the user stay
	const int user_files_before = 1;
	{
		InstallReport r = uninstall(e.manifest, log, nullptr);
		CHECK(r.ok);
	}
	CHECK(!fs::exists(designer));
	CHECK(!fs::exists(sdk / "templates"));
	CHECK(!fs::exists(sdk / ".raven-install" / "manifest.json"));
	CHECK(read(user_file) == "mine");
	CHECK(count_files(sdk) == user_files_before + 1); // my_notes.txt and the raven_core.h kept aside in step 2
	CHECK(!fs::exists(work / "home" / ".local" / "share" / "applications" / "raven-designer.desktop"));
	CHECK(!find_existing_install().found);
}

#endif

static void test_compilers() {
	CompilerDetection d = detect_compilers(true);
	for (const auto& c : d.compilers)
		std::printf("compiler: %s [%s] %s%s\n", c.name.c_str(), c.machine.c_str(), c.verified ? "tested" : c.usable ? "usable" : "not usable",
			c.note.empty() ? "" : (" - " + c.note).c_str());
	for (const auto& c : d.compilers) {
		CHECK(!c.family.empty());
		if (c.verified) CHECK(c.usable);
	}
}

int main(int argc, char** argv) {
	const fs::path work = fs::absolute(argc > 1 ? path_from_utf8(argv[1]) : fs::path("test_installer_work"));
	std::error_code ec;
	fs::remove_all(work, ec);
	fs::create_directories(work / "home");
	// never the folders of the user running the tests
	platform::set_env("HOME", path_to_utf8(work / "home"));
	platform::set_env("XDG_CONFIG_HOME", path_to_utf8(work / "home" / ".config"));
	platform::set_env("XDG_DATA_HOME", path_to_utf8(work / "home" / ".local" / "share"));
	platform::set_env("PATH", platform::get_env("PATH")); // (update-desktop-database may run: harmless in the fake home)

	test_deflate();
	test_json();
	test_payload(work);
#if !defined(_WIN32)
	test_install(work);
#endif
	test_compilers();
	std::printf("%d checks, %d failed\n", g_checks, g_failures);
	return g_failures ? 1 : 0;
}
