/**
 * Raven example - charts
 * A small dashboard made of Chart widgets: KPI tiles with sparklines, a pie,
 * a donut with its total in the middle, two gauges with bands, and a heatmap
 * of the orders by weekday and hour.
 *
 *   charts.xml   the tiles, the slices, the bands (all of it but the heatmap)
 *   main.c       fills the heatmap, pushes live values every second, shuffles
 *                the shares (they glide to their new place), switches the theme
 *
 * The pointer over a slice lifts it and tells its value and share; over a
 * cell of the heatmap or a point of a sparkline, its value. A click on a
 * slice selects it; a click on a cell writes it under the title.
 *
 * For the pictures of the documentation: RAVEN_EXAMPLE_THEME=light starts in
 * the light theme, RAVEN_EXAMPLE_POINT=x,y puts the pointer there (window
 * coordinates) once the window is shown.
 */

#include "../common/example.h"

#include <math.h>

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

#define DAYS 7
#define HOURS 24
#define SPARK_DAYS 30

static struct {
	Rid window;
	Rid traffic, browsers, cpu, memory, activity;
	Rid spark_revenue, spark_users, spark_orders, spark_latency;
	Rid timer;
	int live;
	uint32_t noise;
	float point_x, point_y;
	int point;
	int frame;
} app;

/* A small random number generator: the same data at every run. */
static double noise(void) {
	app.noise = app.noise * 1664525u + 1013904223u;
	return (double)(app.noise >> 8) / 16777216.0 - 0.5;
}

/* Orders in an hour of a weekday: a morning and an evening peak, quieter weekends, a little noise. */
static float orders_at(int day, int hour) {
	double weekend = day >= 5 ? 0.65 : 1.0;
	double morning = exp(-pow((hour - 11.0) / 2.6, 2.0));
	double evening = exp(-pow((hour - 20.0) / 2.2, 2.0)) * (day >= 4 ? 1.35 : 1.0);
	double night = hour < 6 ? 0.15 : 0.35;
	return (float)floor((night + morning * 1.6 + evening * 1.2) * 38.0 * weekend * (1.0 + noise() * 0.35));
}

static void fill_activity(void) {
	float cells[DAYS * HOURS];
	int d, h;
	for (d = 0; d < DAYS; d++) {
		for (h = 0; h < HOURS; h++) cells[d * HOURS + h] = orders_at(d, h);
	}
	/* (the same size as before: the cells glide to their new values) */
	Rchart_set_grid(app.activity, DAYS, HOURS, cells);
}

/* A new value at the end of a sparkline, near the last one; the oldest goes. */
static void push_near(Rid spark, double spread) {
	int count = Rchart_get_value_count(spark);
	double last = count ? Rchart_get_value_at(spark, count - 1) : 0.0;
	Rchart_push_value(spark, (float)(last + noise() * spread), SPARK_DAYS);
}

static void tick(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	if (!app.live) return;
	push_near(app.spark_revenue, 2.0);
	push_near(app.spark_users, 60.0);
	push_near(app.spark_orders, 30.0);
	push_near(app.spark_latency, 14.0);
	Rchart_set_value(app.cpu, fmin(100.0, fmax(0.0, Rchart_get_value(app.cpu) + noise() * 30.0)));
	Rchart_set_value(app.memory, fmin(32.0, fmax(8.0, Rchart_get_value(app.memory) + noise() * 4.0)));
}

/* New shares: every slice moves, the slices glide to their place. */
static void shuffle(void) {
	int i;
	for (i = 0; i < Rchart_get_slice_count(app.traffic); i++) {
		double value = Rchart_get_slice_value(app.traffic, i);
		Rchart_set_slice_value(app.traffic, i, floor(value * (1.0 + noise() * 0.8)));
	}
	for (i = 0; i < Rchart_get_slice_count(app.browsers); i++) {
		double value = Rchart_get_slice_value(app.browsers, i);
		Rchart_set_slice_value(app.browsers, i, floor(value * (1.0 + noise() * 0.8)));
	}
	fill_activity();
}

static void on_slice(Rid chart, int slice, int selected, void* user_data) {
	(void)user_data;
	printf("%s: %s %s\n", Rchart_get_title(chart), Rchart_get_slice_label(chart, slice), selected ? "selected" : "unselected");
}

static void on_cell(Rid chart, int row, int column, float value, void* user_data) {
	char title[128];
	(void)user_data;
	snprintf(title, sizeof(title), "Orders by weekday and hour  -  %s %s: %.0f", Rchart_get_row_label(chart, row), Rchart_get_column_label(chart, column), value);
	Rchart_set_title(chart, title);
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "charts.theme") == 0) {
		Rtheme_apply(Rcheckbox_is_checked(sender) ? R_THEME_DARK : R_THEME_LIGHT);
	} else if (strcmp(action, "charts.live") == 0) {
		app.live = Rcheckbox_is_checked(sender);
	} else if (strcmp(action, "charts.shuffle") == 0) {
		shuffle();
	}
}

/* (for the pictures: the pointer put where RAVEN_EXAMPLE_POINT says, once the layout is done) */
static void on_frame(double delta, void* user_data) {
	(void)delta;
	(void)user_data;
	if (app.point && ++app.frame == 3) Rwindow_inject_mouse_move(app.window, app.point_x, app.point_y);
}

static void example_shutdown(void) {
	Rtimer_destroy(app.timer);
	Rshutdown();
}

int main(void) {
	const char* theme = getenv("RAVEN_EXAMPLE_THEME");
	const char* point = getenv("RAVEN_EXAMPLE_POINT");
	int light = theme && strcmp(theme, "light") == 0;
	if (!example_init("Raven charts")) return 1;
	Rtheme_apply(light ? R_THEME_LIGHT : R_THEME_DARK);
	app.noise = 2026u;
	app.live = R_TRUE;
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/charts.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.traffic = Rwidget_find(app.window, "traffic");
	app.browsers = Rwidget_find(app.window, "browsers");
	app.cpu = Rwidget_find(app.window, "cpu");
	app.memory = Rwidget_find(app.window, "memory");
	app.activity = Rwidget_find(app.window, "activity");
	app.spark_revenue = Rwidget_find(app.window, "spark.revenue");
	app.spark_users = Rwidget_find(app.window, "spark.users");
	app.spark_orders = Rwidget_find(app.window, "spark.orders");
	app.spark_latency = Rwidget_find(app.window, "spark.latency");
	if (light) Rcheckbox_set_checked(Rwidget_find(app.window, "dark"), R_FALSE);

	/* the total in the middle of the donut, written short */
	Rchart_set_center_text(app.browsers, "10.8k", "sessions");
	Rchart_set_slice_clicked_callback(app.traffic, on_slice, NULL);
	Rchart_set_slice_clicked_callback(app.browsers, on_slice, NULL);
	Rchart_set_cell_clicked_callback(app.activity, on_cell, NULL);
	fill_activity();

	Raction_bind("charts.theme", on_action, NULL);
	Raction_bind("charts.live", on_action, NULL);
	Raction_bind("charts.shuffle", on_action, NULL);
	app.timer = Rtimer_create(1.0, R_TRUE, tick, NULL);
	if (point && sscanf(point, "%f,%f", &app.point_x, &app.point_y) == 2) {
		app.point = R_TRUE;
		Rset_frame_callback(on_frame, NULL);
	}

	example_run(app.window, NULL, example_shutdown);
	return 0;
}
