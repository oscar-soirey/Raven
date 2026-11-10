/**
 * Raven example - editor
 * A small scene editor. It shows the features an editor needs:
 *
 *   - the whole window is described in editor.xml,
 *   - widgets are found by name and driven through named actions,
 *   - a viewport shows a scene rendered by the application into a texture
 *     (here a software renderer running on the Raven job system),
 *   - a custom widget draws a frame-time graph with the Rdraw_ functions,
 *   - a virtual list shows a log of any length without storing rows in Raven.
 */

#include "../common/example.h"

#include <math.h>
#include <stdarg.h>

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

/* ---------------------------------------------------------------------------
 *  Scene
 * ------------------------------------------------------------------------- */

#define MAX_OBJECTS 64

typedef struct Object {
	char name[48];
	float x, y;
	float radius;
	float speed;
	float phase;
	int color;
	int visible;
	/* handle of the row in the outliner */
	Rid item;
} Object;

static const float g_palette[4][3] = {
	{ 1.00f, 0.55f, 0.15f }, { 0.25f, 0.55f, 1.00f }, { 0.30f, 0.85f, 0.45f }, { 1.00f, 0.40f, 0.70f }
};

static Object g_objects[MAX_OBJECTS];
static int g_object_count;
static int g_selected = -1;
static int g_playing = 1;
static double g_time;
static int g_created;

/* ---------------------------------------------------------------------------
 *  User interface
 * ------------------------------------------------------------------------- */

static struct {
	Rid window, outliner, scene, graph, log, stats, play, render_info;
	Rid name, radius, speed, color, visible;
	Rid texture;
	/* set while the property widgets are filled from code, to ignore their callbacks */
	int updating;
} ui;

/* ---- log: only the strings live here, the list asks for the rows it draws ---- */

#define LOG_CAPACITY 2000
static char g_log[LOG_CAPACITY][96];
static int g_log_count;

static const char* log_row(Rid list, int index, void* user_data) {
	(void)list;
	(void)user_data;
	return g_log[index];
}

static void log_add(const char* format, ...) {
	va_list arguments;
	if (g_log_count == LOG_CAPACITY) return;
	va_start(arguments, format);
	vsnprintf(g_log[g_log_count], sizeof(g_log[0]), format, arguments);
	va_end(arguments);
	g_log_count++;
	Rlist_view_set_virtual_items(ui.log, g_log_count, log_row, NULL);
	Rlist_view_scroll_to_item(ui.log, g_log_count - 1);
}

/* ---- properties ---- */

static void properties_refresh(void) {
	const Object* object = g_selected >= 0 ? &g_objects[g_selected] : NULL;
	int enabled = object != NULL;

	ui.updating = 1;
	Rtext_input_set_text(ui.name, object ? object->name : "");
	Rslider_set_value(ui.radius, object ? object->radius : 0.0f);
	Rspin_box_set_value(ui.speed, object ? object->speed : 0.0);
	Rcombo_box_set_selected(ui.color, object ? object->color : 0);
	Rcheckbox_set_checked(ui.visible, object ? object->visible : 0);
	ui.updating = 0;

	Rwidget_set_enabled(ui.name, enabled);
	Rwidget_set_enabled(ui.radius, enabled);
	Rwidget_set_enabled(ui.speed, enabled);
	Rwidget_set_enabled(ui.color, enabled);
	Rwidget_set_enabled(ui.visible, enabled);
}

static int object_of_item(Rid item) {
	int i;
	for (i = 0; i < g_object_count; i++) {
		if (g_objects[i].item == item) return i;
	}
	return -1;
}

/* ---- scene edition ---- */

static void scene_add(const char* name) {
	Object* object;
	if (g_object_count == MAX_OBJECTS) {
		log_add("The scene is full (%d objects)", MAX_OBJECTS);
		return;
	}
	object = &g_objects[g_object_count];
	memset(object, 0, sizeof(*object));
	g_created++;
	if (name) snprintf(object->name, sizeof(object->name), "%s", name);
	else snprintf(object->name, sizeof(object->name), "Object %d", g_created);
	object->x = 0.2f + 0.6f * (float)((g_created * 37) % 100) / 100.0f;
	object->y = 0.25f + 0.5f * (float)((g_created * 61) % 100) / 100.0f;
	object->radius = 0.08f + 0.02f * (float)(g_created % 4);
	object->speed = 1.0f + 0.25f * (float)(g_created % 5);
	object->phase = (float)g_created * 1.7f;
	object->color = (g_created - 1) % 4;
	object->visible = 1;
	object->item = Rtree_view_add_item(ui.outliner, R_INVALID_ID, object->name);
	g_selected = g_object_count++;
	Rtree_view_set_selected_item(ui.outliner, object->item);
	properties_refresh();
	log_add("Added \"%s\"", object->name);
}

static void scene_delete_selected(void) {
	int i;
	if (g_selected < 0) return;
	log_add("Deleted \"%s\"", g_objects[g_selected].name);
	Rtree_item_destroy(g_objects[g_selected].item);
	for (i = g_selected; i + 1 < g_object_count; i++) g_objects[i] = g_objects[i + 1];
	g_object_count--;
	g_selected = -1;
	properties_refresh();
}

/* ---------------------------------------------------------------------------
 *  Software renderer: fills an RGBA image, one band of rows per job
 * ------------------------------------------------------------------------- */

static struct {
	uint8_t* pixels;
	int width, height;
	/* viewport size in pixels, and the divisor chosen in the combo box */
	int viewport_width, viewport_height;
	double render_ms;
} g_render;

static void render_rows(Ruint begin, Ruint end, void* user_data) {
	const int width = g_render.width, height = g_render.height;
	const float aspect = (float)width / (float)height;
	Ruint row;
	(void)user_data;

	for (row = begin; row < end; row++) {
		uint8_t* out = g_render.pixels + (size_t)row * (size_t)width * 4;
		const float v = ((float)row + 0.5f) / (float)height;
		int column, i;
		for (column = 0; column < width; column++) {
			const float u = ((float)column + 0.5f) / (float)width;
			/* background: a slow gradient with a grid */
			float r = 0.07f + 0.05f * v, g = 0.08f + 0.06f * v, b = 0.11f + 0.10f * v;
			const float grid_u = u * aspect * 10.0f, grid_v = v * 10.0f;
			if (grid_u - floorf(grid_u) < 0.03f || grid_v - floorf(grid_v) < 0.03f) {
				r += 0.04f;
				g += 0.04f;
				b += 0.05f;
			}
			for (i = 0; i < g_object_count; i++) {
				const Object* object = &g_objects[i];
				float dx, dy, distance, shade, edge;
				if (!object->visible) continue;
				dx = (u - object->x) * aspect;
				dy = v - (object->y + 0.06f * sinf((float)g_time * object->speed + object->phase));
				distance = sqrtf(dx * dx + dy * dy);
				if (distance > object->radius + 0.01f) continue;
				/* a shaded disc with a soft edge; the selected one gets a ring */
				shade = 1.0f - 0.55f * (distance / object->radius) * (distance / object->radius);
				edge = (object->radius - distance) * (float)height * 0.5f;
				edge = edge < 0.0f ? 0.0f : (edge > 1.0f ? 1.0f : edge);
				if (i == g_selected && distance > object->radius - 0.012f) shade = 1.6f;
				r += (g_palette[object->color][0] * shade - r) * edge;
				g += (g_palette[object->color][1] * shade - g) * edge;
				b += (g_palette[object->color][2] * shade - b) * edge;
			}
			out[column * 4 + 0] = (uint8_t)(255.0f * (r > 1.0f ? 1.0f : r));
			out[column * 4 + 1] = (uint8_t)(255.0f * (g > 1.0f ? 1.0f : g));
			out[column * 4 + 2] = (uint8_t)(255.0f * (b > 1.0f ? 1.0f : b));
			out[column * 4 + 3] = 255;
		}
	}
}

static void scene_render(void) {
	static const int divisors[3] = { 1, 2, 4 };
	int divisor = divisors[Rcombo_box_get_selected(Rwidget_find(ui.window, "render.scale")) % 3];
	int width = g_render.viewport_width / divisor, height = g_render.viewport_height / divisor;
	int threaded = Rcheckbox_is_checked(Rwidget_find(ui.window, "render.threads"));
	double start;
	char info[128];

	if (width < 2 || height < 2) return;
	if (width != g_render.width || height != g_render.height) {
		free(g_render.pixels);
		g_render.pixels = (uint8_t*)malloc((size_t)width * (size_t)height * 4);
		g_render.width = width;
		g_render.height = height;
	}

	start = Rget_time();
	/* bands of 8 rows are shared between the worker threads and this thread */
	if (threaded) Rparallel_for((Ruint)height, 8, render_rows, NULL);
	else render_rows(0, (Ruint)height, NULL);
	g_render.render_ms = (Rget_time() - start) * 1000.0;

	/* the pixels are copied: the renderer uploads them on its own thread */
	if (ui.texture == R_INVALID_ID) {
		ui.texture = Rtexture_create_from_pixels(g_render.pixels, width, height);
		Rviewport_set_texture(ui.scene, ui.texture);
	} else {
		Rtexture_update(ui.texture, g_render.pixels, width, height);
	}

	snprintf(info, sizeof(info), "%d x %d pixels in %.2f ms on %d thread%s", width, height, g_render.render_ms,
		threaded ? Rget_worker_thread_count() + 1 : 1, threaded && Rget_worker_thread_count() > 0 ? "s" : "");
	Rlabel_set_text(ui.render_info, info);
}

static void on_scene_resized(Rid viewport, int width, int height, void* user_data) {
	(void)viewport;
	(void)user_data;
	g_render.viewport_width = width;
	g_render.viewport_height = height;
}

/* Mouse in the viewport: select the object under the cursor and drag it. */
static int on_scene_event(Rid viewport, const REvent* event, void* user_data) {
	static int dragging;
	float x, y, width, height, u, v;
	int i;
	(void)user_data;

	Rwidget_get_rect(viewport, &x, &y, &width, &height);
	if (width <= 0.0f || height <= 0.0f) return R_FALSE;
	u = event->local_x / width;
	v = event->local_y / height;

	if (event->type == R_EVENT_MOUSE_DOWN && event->button == R_MOUSE_LEFT) {
		for (i = g_object_count - 1; i >= 0; i--) {
			float dx = (u - g_objects[i].x) * width / height, dy = v - g_objects[i].y;
			if (g_objects[i].visible && dx * dx + dy * dy < g_objects[i].radius * g_objects[i].radius * 1.5f) break;
		}
		g_selected = i;
		Rtree_view_set_selected_item(ui.outliner, i >= 0 ? g_objects[i].item : R_INVALID_ID);
		properties_refresh();
		dragging = i >= 0;
		return R_TRUE;
	}
	if (event->type == R_EVENT_MOUSE_MOVE && dragging && g_selected >= 0) {
		g_objects[g_selected].x = u < 0.0f ? 0.0f : (u > 1.0f ? 1.0f : u);
		g_objects[g_selected].y = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
		return R_TRUE;
	}
	if (event->type == R_EVENT_MOUSE_UP) dragging = 0;
	return R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Custom widget: frame-time graph
 * ------------------------------------------------------------------------- */

#define GRAPH_SAMPLES 240
static float g_frame_ms[GRAPH_SAMPLES];
static int g_frame_cursor;

static void graph_paint(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	float text_r, text_g, text_b, text_a, peak = 20.0f, scale;
	char label[32];
	int i;
	(void)widget;
	(void)user_data;

	/* theme colors keep the widget readable in both themes */
	Rtheme_get_color(R_COLOR_TEXT, &text_r, &text_g, &text_b, &text_a);
	Rdraw_rect(painter, 0.0f, 0.0f, width, height, 6.0f, 0.5f, 0.5f, 0.5f, 0.10f);

	for (i = 0; i < GRAPH_SAMPLES; i++) peak = g_frame_ms[i] > peak ? g_frame_ms[i] : peak;
	scale = (height - 24.0f) / peak;

	/* the 16.7 ms line: 60 frames per second */
	Rdraw_line(painter, 0.0f, height - 16.7f * scale, width, height - 16.7f * scale, 1.0f, 0.3f, 0.8f, 0.4f, 0.8f);
	Rdraw_text(painter, R_INVALID_ID, 11.0f, 6.0f, height - 16.7f * scale - 16.0f, "16.7 ms", text_r, text_g, text_b, 0.6f);

	for (i = 1; i < GRAPH_SAMPLES; i++) {
		/* oldest sample on the left */
		float previous = g_frame_ms[(g_frame_cursor + i - 1) % GRAPH_SAMPLES];
		float current = g_frame_ms[(g_frame_cursor + i) % GRAPH_SAMPLES];
		float x0 = width * (float)(i - 1) / (float)(GRAPH_SAMPLES - 1);
		float x1 = width * (float)i / (float)(GRAPH_SAMPLES - 1);
		Rdraw_line(painter, x0, height - previous * scale, x1, height - current * scale, 1.5f, 0.25f, 0.6f, 1.0f, 1.0f);
	}

	snprintf(label, sizeof(label), "peak %.1f ms", peak);
	Rdraw_text(painter, R_INVALID_ID, 12.0f, 8.0f, 6.0f, label, text_r, text_g, text_b, text_a);
}

/* ---------------------------------------------------------------------------
 *  Actions (named in editor.xml)
 * ------------------------------------------------------------------------- */

#define THEME_COUNT 4

/* The entries of the View menu that choose a theme. */
static const struct {
	const char* action;
	const char* menu_item;
	RTheme theme;
} g_themes[THEME_COUNT] = {
	{ "view.dark", "menu.dark", R_THEME_DARK },
	{ "view.light", "menu.light", R_THEME_LIGHT },
	{ "view.glass_dark", "menu.glass_dark", R_THEME_GLASS_DARK },
	{ "view.glass_light", "menu.glass_light", R_THEME_GLASS_LIGHT }
};

static int theme_of_action(const char* action, int* index) {
	int i;
	for (i = 0; i < THEME_COUNT; i++) {
		if (strcmp(action, g_themes[i].action) == 0) {
			*index = i;
			return 1;
		}
	}
	return 0;
}

static void on_action(Rid sender, const char* action, void* user_data) {
	Object* object = g_selected >= 0 ? &g_objects[g_selected] : NULL;
	int theme_index = 0;
	(void)user_data;

	if (strcmp(action, "app.quit") == 0) {
		Rquit(0);
	} else if (strcmp(action, "scene.new") == 0) {
		Rtree_view_clear_items(ui.outliner);
		g_object_count = 0;
		g_selected = -1;
		properties_refresh();
		log_add("New scene");
	} else if (strcmp(action, "scene.add") == 0) {
		scene_add(NULL);
	} else if (strcmp(action, "scene.delete") == 0) {
		scene_delete_selected();
	} else if (strcmp(action, "scene.play") == 0) {
		g_playing = !g_playing;
		Rbutton_set_text(ui.play, g_playing ? "Pause" : "Play");
		/* no animation: the window is only redrawn when something changes */
		Rviewport_set_realtime(ui.scene, g_playing);
		log_add(g_playing ? "Animation started" : "Animation paused");
	} else if (theme_of_action(action, &theme_index)) {
		/* the two glass themes make every control translucent (see examples/glass) */
		int i;
		Rtheme_apply(g_themes[theme_index].theme);
		for (i = 0; i < THEME_COUNT; i++) {
			Rmenu_item_set_checked(Rwidget_find(ui.window, g_themes[i].menu_item), i == theme_index);
		}
	} else if (strcmp(action, "view.compact") == 0) {
		Rtheme_set_metric(R_METRIC_FONT_SIZE, 12.0f);
		Rtheme_set_metric(R_METRIC_CONTROL_HEIGHT, 22.0f);
		Rtheme_set_metric(R_METRIC_ROW_HEIGHT, 20.0f);
	} else if (strcmp(action, "view.comfortable") == 0) {
		Rtheme_set_metric(R_METRIC_FONT_SIZE, 15.0f);
		Rtheme_set_metric(R_METRIC_CONTROL_HEIGHT, 32.0f);
		Rtheme_set_metric(R_METRIC_ROW_HEIGHT, 28.0f);
	} else if (object && !ui.updating) {
		/* the property widgets edit the selected object */
		if (strcmp(action, "prop.radius") == 0) {
			object->radius = Rslider_get_value(sender);
		} else if (strcmp(action, "prop.speed") == 0) {
			object->speed = (float)Rspin_box_get_value(sender);
		} else if (strcmp(action, "prop.color") == 0) {
			object->color = Rcombo_box_get_selected(sender) & 3;
		} else if (strcmp(action, "prop.visible") == 0) {
			object->visible = Rcheckbox_is_checked(sender);
		}
	}
	/* a paused scene is rendered again only when it was edited */
	if (!g_playing) Rwidget_invalidate(ui.scene);
}

/* Every keystroke in the name field renames the selected object. */
static void on_name_changed(Rid input, const char* text, void* user_data) {
	(void)input;
	(void)user_data;
	if (g_selected < 0 || ui.updating) return;
	snprintf(g_objects[g_selected].name, sizeof(g_objects[g_selected].name), "%s", text);
	Rtree_item_set_text(g_objects[g_selected].item, text);
}

/* A row of the outliner was selected, with the mouse or the keyboard. */
static void on_outliner_selection(Rid tree, Rid item, void* user_data) {
	(void)tree;
	(void)user_data;
	g_selected = object_of_item(item);
	properties_refresh();
	if (!g_playing) Rwidget_invalidate(ui.scene);
}

/* ---------------------------------------------------------------------------
 *  Frame
 * ------------------------------------------------------------------------- */

/* Called once per frame, before the layout. */
static void on_frame(double delta_seconds, void* user_data) {
	static double stats_timer;
	(void)user_data;
	if (!Rwindow_is_valid(ui.window)) return;

	if (g_playing) g_time += delta_seconds;
	scene_render();

	g_frame_ms[g_frame_cursor] = (float)(delta_seconds * 1000.0);
	g_frame_cursor = (g_frame_cursor + 1) % GRAPH_SAMPLES;
	Rwidget_invalidate(ui.graph);

	stats_timer += delta_seconds;
	if (stats_timer > 0.25) {
		RFrameStats stats;
		char text[160];
		stats_timer = 0.0;
		Rwindow_get_frame_stats(ui.window, &stats);
		snprintf(text, sizeof(text), "%u widgets   %u primitives in %u draw call%s   layout %.2f ms   paint %.2f ms   gpu %.2f ms",
			stats.widget_count, stats.instance_count, stats.draw_call_count, stats.draw_call_count > 1 ? "s" : "",
			stats.layout_ms, stats.paint_ms, stats.render_ms);
		Rlabel_set_text(ui.stats, text);
	}
}

static void example_shutdown(void) {
	free(g_render.pixels);
	Rshutdown();
}

int main(void) {
	if (!example_init("editor")) return 1;

	ui.texture = R_INVALID_ID;
	ui.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/editor.xml");
	if (ui.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}

	/* widgets are found by the Name given in the XML */
	ui.outliner = Rwidget_find(ui.window, "outliner");
	ui.scene = Rwidget_find(ui.window, "scene");
	ui.graph = Rwidget_find(ui.window, "graph");
	ui.log = Rwidget_find(ui.window, "log");
	ui.stats = Rwidget_find(ui.window, "stats");
	ui.play = Rwidget_find(ui.window, "play");
	ui.render_info = Rwidget_find(ui.window, "render.info");
	ui.name = Rwidget_find(ui.window, "prop.name");
	ui.radius = Rwidget_find(ui.window, "prop.radius");
	ui.speed = Rwidget_find(ui.window, "prop.speed");
	ui.color = Rwidget_find(ui.window, "prop.color");
	ui.visible = Rwidget_find(ui.window, "prop.visible");

	/* one handler receives every action that has no function of its own */
	Raction_set_default_handler(on_action, NULL);

	/* events without an action name are plain callbacks */
	Rtree_view_set_selection_changed_callback(ui.outliner, on_outliner_selection, NULL);
	Rtext_input_set_changed_callback(ui.name, on_name_changed, NULL);
	Rcustom_set_paint_callback(ui.graph, graph_paint, NULL);
	Rviewport_set_resize_callback(ui.scene, on_scene_resized, NULL);
	Rwidget_set_event_callback(ui.scene, on_scene_event, NULL);
	Rset_frame_callback(on_frame, NULL);

	log_add("Raven %s, %d worker thread(s)", Rget_version(), Rget_worker_thread_count());
	scene_add("Sun");
	scene_add("Planet");
	scene_add("Moon");

	/* the loop belongs to the application: Raven draws one frame per call,
	 * and Rwait_frame sleeps until there is something to draw */
	example_run(ui.window, NULL, example_shutdown);
	return 0;
}
