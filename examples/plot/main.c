/**
 * Raven example - plot
 * The Plot widget: curves of X / Y data.
 *
 *   readings   written in XML only: series, markers, a scatter, a drag line (plot.xml)
 *   stream     data that streams in (a timer adds points, the oldest go), a second Y axis
 *   bode       a logarithmic X axis; dragging the cutoff computes the curve again
 *   large      a million points, thinned as they are drawn, on a symmetric logarithm
 *
 * The mouse: drag to move the view, right drag to zoom into a rectangle, the
 * wheel to zoom (over an axis: that axis only), a double click to fit the
 * data again, a click on a name in the legend to hide its series.
 */

#include "../common/example.h"

#include <math.h>

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

#define PI 3.14159265358979323846
#define BODE_POINTS 400
#define LARGE_POINTS 1000000

static struct {
	Rid window;
	Rid readings, stream, bode, large;
	Rid large_view;
	Rid timer;
	int paused;
	double t;
	/* the series of the stream */
	int wave, slow, temperature;
	/* the series of the response, and the cutoff the user drags */
	int rc_gain, rc_phase;
	double frequencies[BODE_POINTS];
	uint32_t noise;
} app;

/* A small random number generator: the same data at every run. */
static double noise(void) {
	app.noise = app.noise * 1664525u + 1013904223u;
	return (double)(app.noise >> 8) / 16777216.0 - 0.5;
}

/* ---------------------------------------------------------------------------
 *  2. The stream: 60 points a second, the last 10 seconds kept
 * ------------------------------------------------------------------------- */

static void stream_tick(Rid timer, void* user_data) {
	double t;
	(void)timer;
	(void)user_data;
	if (app.paused) return;
	app.t += 1.0 / 60.0;
	t = app.t;
	Rplot_add_point(app.stream, app.wave, t, sin(2.0 * PI * 0.5 * t) + noise() * 0.25);
	Rplot_add_point(app.stream, app.slow, t, 0.6 * sin(2.0 * PI * 0.13 * t) + 0.3 * sin(2.0 * PI * 1.7 * t));
	Rplot_add_point(app.stream, app.temperature, t, 21.0 + 2.5 * sin(0.21 * t) + noise() * 0.1);
}

static void make_stream(void) {
	app.wave = Rplot_add_series(app.stream, "wave");
	app.slow = Rplot_add_series(app.stream, "slow");
	app.temperature = Rplot_add_series(app.stream, "temperature");
	Rplot_set_series_line(app.stream, app.slow, R_PLOT_LINE_SOLID, 1.25f);
	Rplot_set_series_fill(app.stream, app.slow, 0.18f);
	/* the temperature against the axis on the right */
	Rplot_set_series_axes(app.stream, app.temperature, R_PLOT_X, R_PLOT_Y2);
	Rplot_set_series_line(app.stream, app.temperature, R_PLOT_LINE_DASHED, 2.0f);
	Rplot_set_series_max_points(app.stream, app.wave, 600);
	Rplot_set_series_max_points(app.stream, app.slow, 600);
	Rplot_set_series_max_points(app.stream, app.temperature, 600);
	app.timer = Rtimer_create(1.0 / 60.0, R_TRUE, stream_tick, NULL);
}

/* ---------------------------------------------------------------------------
 *  3. The response of a first order filter (its cutoff dragged) and of a second order one
 * ------------------------------------------------------------------------- */

static void compute_rc(double cutoff) {
	double gain[BODE_POINTS], phase[BODE_POINTS];
	int i;
	for (i = 0; i < BODE_POINTS; i++) {
		double ratio = app.frequencies[i] / cutoff;
		gain[i] = -10.0 * log10(1.0 + ratio * ratio);
		phase[i] = -atan(ratio) * 180.0 / PI;
	}
	Rplot_set_series_data(app.bode, app.rc_gain, app.frequencies, gain, BODE_POINTS);
	Rplot_set_series_data(app.bode, app.rc_phase, app.frequencies, phase, BODE_POINTS);
}

static void on_cutoff(Rid plot, int line, double value, void* user_data) {
	(void)plot;
	(void)line;
	(void)user_data;
	/* (the line may be dragged out of the range of the frequencies) */
	if (value < 1.0) value = 1.0;
	compute_rc(value);
}

static void make_bode(void) {
	double gain[BODE_POINTS], phase[BODE_POINTS];
	const double f0 = 10000.0, q = 0.7071;
	int second, second_phase, i;

	/* 1 Hz to 100 kHz, at even distances on the logarithmic axis */
	for (i = 0; i < BODE_POINTS; i++) app.frequencies[i] = pow(10.0, 5.0 * (double)i / (double)(BODE_POINTS - 1));

	app.rc_gain = Rplot_add_series(app.bode, "RC gain");
	second = Rplot_add_series(app.bode, "2nd order gain");
	app.rc_phase = Rplot_add_series(app.bode, "RC phase");
	second_phase = Rplot_add_series(app.bode, "2nd order phase");
	Rplot_set_series_line(app.bode, app.rc_gain, R_PLOT_LINE_SOLID, 2.25f);
	Rplot_set_series_line(app.bode, second, R_PLOT_LINE_SOLID, 2.25f);
	/* the phases against the axis on the right, dotted, in the colors of their gains */
	Rplot_set_series_axes(app.bode, app.rc_phase, R_PLOT_X, R_PLOT_Y2);
	Rplot_set_series_axes(app.bode, second_phase, R_PLOT_X, R_PLOT_Y2);
	Rplot_set_series_line(app.bode, app.rc_phase, R_PLOT_LINE_DOTTED, 1.75f);
	Rplot_set_series_line(app.bode, second_phase, R_PLOT_LINE_DOTTED, 1.75f);
	Rplot_set_series_color(app.bode, app.rc_phase, 0.12f, 0.47f, 0.71f, 1.0f);
	Rplot_set_series_color(app.bode, second_phase, 1.0f, 0.5f, 0.05f, 1.0f);

	for (i = 0; i < BODE_POINTS; i++) {
		double ratio = app.frequencies[i] / f0;
		double real = 1.0 - ratio * ratio, imaginary = ratio / q;
		gain[i] = -10.0 * log10(real * real + imaginary * imaginary);
		phase[i] = -atan2(imaginary, real) * 180.0 / PI;
	}
	Rplot_set_series_data(app.bode, second, app.frequencies, gain, BODE_POINTS);
	Rplot_set_series_data(app.bode, second_phase, app.frequencies, phase, BODE_POINTS);
	compute_rc(1000.0);

	/* a dB scale that does not jump when the cutoff moves, and the cutoff to drag */
	Rplot_set_axis_range(app.bode, R_PLOT_Y, -80.0, 5.0);
	Rplot_set_axis_range(app.bode, R_PLOT_Y2, -190.0, 10.0);
	Rplot_set_axis_format(app.bode, R_PLOT_Y, "%g dB");
	Rplot_add_drag_line(app.bode, R_PLOT_X, 1000.0, "cutoff");
	Rplot_add_drag_line(app.bode, R_PLOT_Y, -3.0, "-3 dB");
	Rplot_set_drag_line_color(app.bode, 1, 0.55f, 0.55f, 0.6f, 1.0f);
	Rplot_set_drag_line_callback(app.bode, on_cutoff, NULL);
}

/* ---------------------------------------------------------------------------
 *  4. A million points: a random walk that crosses zero, and a running mean drawn in steps
 * ------------------------------------------------------------------------- */

static void show_large_view(Rid plot, void* user_data) {
	double x0, x1, y0, y1;
	char text[160];
	(void)user_data;
	Rplot_get_axis_range(plot, R_PLOT_X, &x0, &x1);
	Rplot_get_axis_range(plot, R_PLOT_Y, &y0, &y1);
	snprintf(text, sizeof(text), "%d points  -  X from %.0f to %.0f, Y from %.1f to %.1f", LARGE_POINTS, x0, x1, y0, y1);
	Rlabel_set_text(app.large_view, text);
}

static void make_large(void) {
	static double y[LARGE_POINTS], mean_x[LARGE_POINTS / 10000], mean_y[LARGE_POINTS / 10000];
	double position = 0.0, sum = 0.0;
	int walk, mean, i;
	for (i = 0; i < LARGE_POINTS; i++) {
		position += noise() * 2.0;
		y[i] = position;
		sum += position;
		/* the mean of each run of 10000 points */
		if ((i + 1) % 10000 == 0) {
			mean_x[i / 10000] = (double)(i - 9999);
			mean_y[i / 10000] = sum / 10000.0;
			sum = 0.0;
		}
	}
	walk = Rplot_add_series(app.large, "random walk");
	/* x NULL: the points are numbered 0, 1, 2... */
	Rplot_set_series_data(app.large, walk, NULL, y, LARGE_POINTS);
	Rplot_set_series_line(app.large, walk, R_PLOT_LINE_SOLID, 1.0f);
	mean = Rplot_add_series(app.large, "mean of 10000");
	Rplot_set_series_data(app.large, mean, mean_x, mean_y, LARGE_POINTS / 10000);
	Rplot_set_series_step(app.large, mean, R_TRUE);
	Rplot_set_series_line(app.large, mean, R_PLOT_LINE_SOLID, 2.5f);
	Rplot_set_series_color(app.large, mean, 0.85f, 0.15f, 0.16f, 1.0f);
	Rplot_set_view_changed_callback(app.large, show_large_view, NULL);
	show_large_view(app.large, NULL);
}

/* ---------------------------------------------------------------------------
 *  The bar above the plots
 * ------------------------------------------------------------------------- */

static void each_plot(void (*apply)(Rid plot, int on), int on) {
	apply(app.readings, on);
	apply(app.stream, on);
	apply(app.bode, on);
	apply(app.large, on);
}

static void set_crosshair(Rid plot, int on) {
	Rplot_set_crosshair(plot, on);
}

static void fit(Rid plot, int on) {
	(void)on;
	Rplot_fit(plot);
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "plot.crosshair") == 0) {
		each_plot(set_crosshair, Rcheckbox_is_checked(sender));
	} else if (strcmp(action, "plot.symlog") == 0) {
		Rplot_set_axis_scale(app.large, R_PLOT_Y, Rcheckbox_is_checked(sender) ? R_PLOT_SCALE_SYMLOG : R_PLOT_SCALE_LINEAR);
		show_large_view(app.large, NULL);
	} else if (strcmp(action, "plot.pause") == 0) {
		app.paused = !app.paused;
		Rbutton_set_text(sender, app.paused ? "Resume" : "Pause");
	} else if (strcmp(action, "plot.fit") == 0) {
		each_plot(fit, 0);
		/* the limits of the response are those it was given, not those of the data */
		Rplot_set_axis_range(app.bode, R_PLOT_Y, -80.0, 5.0);
		Rplot_set_axis_range(app.bode, R_PLOT_Y2, -190.0, 10.0);
		show_large_view(app.large, NULL);
	}
}

int main(void) {
	if (!example_init("Raven plots")) return 1;
	Rtheme_apply(R_THEME_DARK);
	app.noise = 12345u;
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/plot.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.readings = Rwidget_find(app.window, "readings");
	app.stream = Rwidget_find(app.window, "stream");
	app.bode = Rwidget_find(app.window, "bode");
	app.large = Rwidget_find(app.window, "large");
	app.large_view = Rwidget_find(app.window, "large.view");

	make_stream();
	make_bode();
	make_large();
	Raction_bind("plot.crosshair", on_action, NULL);
	Raction_bind("plot.symlog", on_action, NULL);
	Raction_bind("plot.pause", on_action, NULL);
	Raction_bind("plot.fit", on_action, NULL);

	if (!example_automated(app.window)) {
		while (Rwait_frame(-1.0)) {
		}
	}
	Rtimer_destroy(app.timer);
	Rshutdown();
	return 0;
}
