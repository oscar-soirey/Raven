/**
 * Raven example - studio
 * The widgets of creative tools: NodeGraph, Timeline, ColorPicker.
 *
 *   Rnode_graph_set_connect_callback   the application decides which links are allowed
 *                                      (here: none that makes a loop)
 *   Rnode_graph_set_selection_callback what is selected, to show it elsewhere
 *   Rnode_graph_add_node / add_input   nodes built from code, with any widget inside
 *   Rtimeline_set_time_callback        the playhead, scrubbed or playing
 *   Rcolor_picker_...                  a color, given to the selected nodes
 *   Rnode_graph_set_preset             the look of the graph and of the timeline
 *   Rtimeline_set_preset               (Auto follows the theme of the window)
 *
 * studio.xml is watched while the example runs (live reload): edit it, save,
 * and the window follows.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window, graph, timeline, tint, selected, time, play;
	int added, playing;
} app;

/* Widgets are found again after a reload: the file may have replaced them. */
static void find_widgets(void) {
	app.graph = Rwidget_find(app.window, "graph");
	app.timeline = Rwidget_find(app.window, "timeline");
	app.tint = Rwidget_find(app.window, "tint");
	app.selected = Rwidget_find(app.window, "selected");
	app.time = Rwidget_find(app.window, "time");
	app.play = Rwidget_find(app.window, "play");
}

/* ---- the graph ---- */

/* Does a path of links lead from `node` to `target`? Each node is looked at once,
 * however long the path: a node is expanded once and a link pushed once per expansion,
 * so the nodes to look at never outnumber the links plus the first one. */
static int reaches(Rid graph, int node, int target) {
	int links = Rnode_graph_get_link_count(graph);
	int nodes = Rnode_graph_get_node_count(graph);
	int* pending = (int*)malloc(sizeof(int) * (size_t)(links + 1));
	int* seen = (int*)malloc(sizeof(int) * (size_t)(nodes + 1));
	int pending_count = 0, seen_count = 0, found = R_FALSE;
	if (!pending || !seen) {
		free(pending);
		free(seen);
		return R_TRUE; /* (when unsure, the link is refused) */
	}
	pending[pending_count++] = node;
	while (pending_count > 0) {
		int current = pending[--pending_count], known = R_FALSE, i, k;
		if (current == target) {
			found = R_TRUE;
			break;
		}
		for (i = 0; i < seen_count && !known; i++) known = seen[i] == current;
		if (known || seen_count > nodes) continue;
		seen[seen_count++] = current;
		for (i = 0; ; i++) {
			int output = Rnode_graph_get_output(graph, current, i);
			if (!output) break;
			for (k = 0; k < links; k++) {
				int link = Rnode_graph_get_link(graph, k);
				if (Rnode_graph_get_link_output(graph, link) == output) {
					pending[pending_count++] = Rnode_graph_get_pin_node(graph, Rnode_graph_get_link_input(graph, link));
				}
			}
		}
	}
	free(pending);
	free(seen);
	return found;
}

/* A link is allowed unless what it feeds already feeds where it comes from. */
static int on_connect(Rid graph, int output_pin, int input_pin, void* user_data) {
	(void)user_data;
	return !reaches(graph, Rnode_graph_get_pin_node(graph, input_pin), Rnode_graph_get_pin_node(graph, output_pin));
}

static void on_selection(Rid graph, int node, void* user_data) {
	char text[160];
	int count = Rnode_graph_get_selection_count(graph);
	(void)node;
	(void)user_data;
	if (count == 0) snprintf(text, sizeof(text), "No node selected");
	else if (count == 1) snprintf(text, sizeof(text), "%.100s", Rnode_graph_get_node_title(graph, Rnode_graph_get_selected_node(graph, 0)));
	else snprintf(text, sizeof(text), "%d nodes selected", count);
	Rlabel_set_text(app.selected, text);
}

/* Without this callback the Delete key would only remove links. */
static void on_removed(Rid graph, int node, void* user_data) {
	(void)graph;
	(void)node;
	(void)user_data;
}

static void action_tint(Rid sender, const char* action, void* user_data) {
	float r, g, b, a;
	int i;
	(void)action;
	(void)user_data;
	Rcolor_picker_get_color(sender, &r, &g, &b, &a);
	for (i = 0; i < Rnode_graph_get_selection_count(app.graph); i++) {
		Rnode_graph_set_node_color(app.graph, Rnode_graph_get_selected_node(app.graph, i), r, g, b);
	}
}

/* A node made from code: pins, and a widget inside it. */
static void action_add(Rid sender, const char* action, void* user_data) {
	char title[32];
	float x, y, zoom;
	int node;
	Rid knob;
	(void)sender;
	(void)action;
	(void)user_data;
	Rnode_graph_get_view(app.graph, &x, &y, &zoom);
	snprintf(title, sizeof(title), "Remap %d", ++app.added);
	node = Rnode_graph_add_node(app.graph, title, x - 70.0f + (float)(app.added % 5) * 24.0f, y - 50.0f + (float)(app.added % 5) * 24.0f);
	Rnode_graph_add_input(app.graph, node, "Value", "number");
	Rnode_graph_add_output(app.graph, node, "Result", "number");
	Rnode_graph_set_node_color(app.graph, node, 0.39f, 0.82f, 1.0f);
	knob = Rwidget_create(R_WIDGET_SLIDER);
	Rslider_set_value(knob, 0.5f);
	Rwidget_set_size(knob, 150.0f, -1.0f);
	Rnode_graph_set_node_content(app.graph, node, knob);
	Rnode_graph_select_node(app.graph, node, R_FALSE);
	on_selection(app.graph, node, NULL);
}

static void action_frame(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	Rnode_graph_frame_all(app.graph);
}

/* ---- the timeline ---- */

static void on_time(Rid timeline, double seconds, void* user_data) {
	char text[32];
	int whole = (int)seconds, frame = (int)((seconds - (double)whole) * Rtimeline_get_frame_rate(timeline) + 0.5);
	(void)user_data;
	snprintf(text, sizeof(text), "%d:%02d:%02d", whole / 60, whole % 60, frame);
	Rlabel_set_text(app.time, text);
}

static void action_play(Rid sender, const char* action, void* user_data) {
	int playing = !Rtimeline_is_playing(app.timeline);
	(void)sender;
	(void)action;
	(void)user_data;
	if (playing && Rtimeline_get_time(app.timeline) >= Rtimeline_get_duration(app.timeline)) Rtimeline_set_time(app.timeline, 0.0);
	Rtimeline_set_playing(app.timeline, playing);
	app.playing = playing;
	Rbutton_set_text(app.play, playing ? "Pause" : "Play");
}

/* Space in the timeline plays and pauses too, and tells nobody: the button follows it at each frame. */
static void on_frame(double delta_seconds, void* user_data) {
	int playing = Rtimeline_is_playing(app.timeline) ? R_TRUE : R_FALSE;
	(void)delta_seconds;
	(void)user_data;
	if (playing == app.playing) return;
	app.playing = playing;
	Rbutton_set_text(app.play, playing ? "Pause" : "Play");
}

/* ---- the rest ---- */

static void action_theme(Rid sender, const char* action, void* user_data) {
	static const RTheme themes[] = { R_THEME_DARK, R_THEME_LIGHT, R_THEME_GLASS_DARK, R_THEME_GLASS_LIGHT };
	/* the looks of the platforms: with the look Auto, the graph and the timeline follow them */
	static const char* const platforms[] = { "WindowsDark", "MacOSDark", "GnomeDark" };
	int index = Rcombo_box_get_selected(sender);
	(void)action;
	(void)user_data;
	if (index >= 0 && index < 4) Rtheme_apply(themes[index]);
	else if (index >= 4 && index < 7) Rtheme_use(platforms[index - 4]);
}

/* The look of the graph and of the timeline: Auto, Modern, Blueprint, Blender, Neon. */
static void action_look(Rid sender, const char* action, void* user_data) {
	int index = Rcombo_box_get_selected(sender);
	(void)action;
	(void)user_data;
	if (index < 0 || index > 4) return;
	Rnode_graph_set_preset(app.graph, (RVisualPreset)((int)R_PRESET_AUTO + index));
	Rtimeline_set_preset(app.timeline, (RVisualPreset)((int)R_PRESET_AUTO + index));
}

static void connect_widgets(void) {
	find_widgets();
	Rnode_graph_set_connect_callback(app.graph, on_connect, NULL);
	Rnode_graph_set_selection_callback(app.graph, on_selection, NULL);
	Rnode_graph_set_removed_callback(app.graph, on_removed, NULL);
	Rtimeline_set_time_callback(app.timeline, on_time, NULL);
	Rtimeline_set_loop(app.timeline, R_TRUE);
	on_time(app.timeline, Rtimeline_get_time(app.timeline), NULL);
	/* (a graph or a timeline built again by a reload takes the look chosen) */
	action_look(Rwidget_find(app.window, "look"), "studio.look", NULL);
}

/* The file changed and the window followed: widgets that were replaced need their callbacks again. */
static void on_reload(const char* path, int ok, void* user_data) {
	(void)path;
	(void)user_data;
	if (ok) connect_widgets();
}

int main(void) {
	const char* theme = getenv("RAVEN_EXAMPLE_THEME");
	if (!example_init("Raven studio")) return 1;
	Rtheme_apply(R_THEME_DARK);

	Raction_bind("studio.tint", action_tint, NULL);
	Raction_bind("studio.add", action_add, NULL);
	Raction_bind("studio.frame", action_frame, NULL);
	Raction_bind("studio.play", action_play, NULL);
	Rset_frame_callback(on_frame, NULL);
	Raction_bind("studio.theme", action_theme, NULL);
	Raction_bind("studio.look", action_look, NULL);

	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/studio.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	connect_widgets();
	Rxml_set_reload_callback(on_reload, NULL);
	/* (for the pictures of the documentation: RAVEN_EXAMPLE_THEME=2 starts with the third theme, RAVEN_EXAMPLE_LOOK=3 with the fourth look) */
	if (theme) {
		Rid combo = Rwidget_find(app.window, "theme");
		Rcombo_box_set_selected(combo, atoi(theme));
		action_theme(combo, "studio.theme", NULL);
	}
	if (getenv("RAVEN_EXAMPLE_LOOK")) {
		Rid combo = Rwidget_find(app.window, "look");
		Rcombo_box_set_selected(combo, atoi(getenv("RAVEN_EXAMPLE_LOOK")));
		action_look(combo, "studio.look", NULL);
	}
	Rwindow_show(app.window);

	example_run(app.window, NULL, NULL);
	return Rget_exit_code();
}
