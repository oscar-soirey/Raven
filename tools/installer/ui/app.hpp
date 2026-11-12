// Raven SDK Installer - the wizard, drawn by RavenGUI. Its interface is the
// XML of ui/xml (embedded); this class fills it, reacts to its actions and
// runs the engine on a worker thread, whose progress comes back to the UI
// thread through Rtask_post.
#pragma once

#include "cli.hpp"
#include "session.hpp"

#include <Raven.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace rinst {

class InstallerApp {
public:
	InstallerApp(Session& session, const CommandLine& args);
	~InstallerApp();

	/** Loads the interface; false when it cannot. */
	bool create();
	/** Runs until the window closes; returns the exit code. */
	int run();

private:
	enum Page { Welcome, Location, Environment, Components, Summary, Progress, Done, PageCount };
	enum class Mode { Install, Uninstall };

	static void on_action(Rid sender, const char* action, void* self);
	static void on_frame(double delta, void* self);
	static int on_close(Rid window, void* self);
	static void on_text_changed(Rid input, const char* text, void* self);
	static void on_check_changed(Rid checkbox, int checked, void* self);
	void action(const std::string& name);

	void show_page(int page);
	void next();
	void back();
	void cancel();

	// pages
	void fill_welcome();
	void fill_location();
	void check_locations();
	void start_detection();
	void fill_detection();
	void build_targets();
	void build_options();
	void refresh_components();
	void fill_summary();
	void start_install();
	void start_uninstall();
	void finish(const InstallReport& report);

	InstallOptions current_options() const;
	Rid find(const char* name) const;
	void set_text(const char* name, const std::string& text);
	void set_visible(const char* name, bool visible);
	void set_status_row(int index, const std::string& state, int level);
	void flush_worker_updates();
	void post(std::function<void()> task);
	void test_step();

	Session& s_;
	CommandLine args_;
	Rid window_ = R_INVALID_ID;
	Rid pages_ = R_INVALID_ID;
	int page_ = Welcome;
	Mode mode_ = Mode::Install;
	InstallOptions defaults_;

	// compilers
	CompilerDetection detection_;
	bool detecting_ = false;
	bool detected_ = false;
	bool targets_touched_ = false;
	std::thread detect_thread_;

	struct Row {
		std::string id;
		Rid row;
		Rid check;
	};
	std::vector<Row> target_rows_;
	std::vector<Row> option_rows_;
	std::vector<Rid> status_rows_;

	// installation
	std::thread work_thread_;
	std::atomic<bool> cancel_{false};
	std::atomic<bool> working_{false};
	bool finished_ = false;
	InstallReport report_;
	fs::path log_file_;

	// what the worker hands to the UI thread
	std::mutex updates_mutex_;
	bool update_posted_ = false;
	InstallProgress pending_progress_;
	bool has_pending_progress_ = false;
	std::vector<std::string> pending_log_;
	std::string log_text_;
	int log_lines_ = 0;

	int exit_code_ = 0;
	int test_ticks_ = 0;
	int test_page_ = -1;
	void capture(const std::string& name);
};

} // namespace rinst
