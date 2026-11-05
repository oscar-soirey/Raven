/**
 * Raven example - glass
 * The glass themes: translucent controls that blur and refract what is
 * behind them, floating panels, animated switches and sliders.
 *
 * Two things are needed to get this look:
 *
 *   Rtheme_apply(R_THEME_GLASS_DARK);      every control becomes glass
 *   <Border Material="FrostedGlass" ...>   a panel of glass: clear (Glass),
 *                                          frosted or tinted (glass.xml)
 *
 * Glass shows what is behind it, so it needs something to show: the theme
 * gives the window a soft colored wallpaper, replaced with
 * Rwindow_set_background, and here a page of colored cards scrolls under
 * the floating bars.
 */

#include "../common/example.h"

#include <math.h>

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

#define CARD_COUNT 9
#define CARD_SIZE 96

static struct {
	Rid window, page, play, position, time, progress, menu;
	int playing;
	double seconds;
} app;

static const char* const g_card_titles[CARD_COUNT] = {
	"Sunrise", "Deep water", "Citrus", "Orchid", "Glacier", "Ember", "Meadow", "Dusk", "Coral"
};

/* A small picture for a card: two colors melted together with a bright spot. */
static Rid card_texture(int index) {
	static const float colors[CARD_COUNT][6] = {
		{ 1.00f, 0.60f, 0.20f, 0.95f, 0.20f, 0.45f }, { 0.05f, 0.30f, 0.75f, 0.10f, 0.80f, 0.85f },
		{ 0.98f, 0.85f, 0.15f, 0.40f, 0.85f, 0.25f }, { 0.65f, 0.25f, 0.90f, 0.98f, 0.45f, 0.75f },
		{ 0.55f, 0.85f, 1.00f, 0.20f, 0.35f, 0.80f }, { 0.90f, 0.20f, 0.10f, 1.00f, 0.70f, 0.15f },
		{ 0.20f, 0.70f, 0.35f, 0.75f, 0.95f, 0.45f }, { 0.25f, 0.15f, 0.55f, 0.95f, 0.45f, 0.35f },
		{ 1.00f, 0.45f, 0.40f, 1.00f, 0.80f, 0.60f }
	};
	static uint8_t pixels[CARD_SIZE * CARD_SIZE * 4];
	const float* c = colors[index];
	int x, y, k;

	for (y = 0; y < CARD_SIZE; y++) {
		for (x = 0; x < CARD_SIZE; x++) {
			float u = (float)x / (CARD_SIZE - 1), v = (float)y / (CARD_SIZE - 1);
			/* a diagonal blend, bent by a wave so that no two cards look alike */
			float t = 0.5f * (u + v) + 0.18f * sinf(u * 5.0f + (float)index) * cosf(v * 4.0f - (float)index * 0.7f);
			float du = u - 0.72f, dv = v - 0.28f;
			float spot = expf(-(du * du + dv * dv) * 9.0f) * 0.45f;
			uint8_t* out = pixels + (y * CARD_SIZE + x) * 4;
			t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
			for (k = 0; k < 3; k++) {
				float value = c[k] + (c[k + 3] - c[k]) * t + spot;
				out[k] = (uint8_t)(255.0f * (value > 1.0f ? 1.0f : value));
			}
			out[3] = 255;
		}
	}
	return Rtexture_create_from_pixels(pixels, CARD_SIZE, CARD_SIZE);
}

/* A card: a picture with rounded corners and its title on a chip of glass. */
static void add_card(Rid grid, int index) {
	Rid card = Rwidget_add_child(grid, Rwidget_create(R_WIDGET_OVERLAY));
	Rid image = Rwidget_add_child(card, Rwidget_create(R_WIDGET_IMAGE));
	Rid chip = Rwidget_add_child(card, Rwidget_create(R_WIDGET_BORDER));
	Rid title = Rwidget_add_child(chip, Rlabel_create(g_card_titles[index]));

	Rimage_set_texture(image, card_texture(index));
	Rimage_set_stretch(image, R_STRETCH_COVER);
	Rimage_set_corner_radius(image, 24.0f);

	Rborder_set_material(chip, R_MATERIAL_GLASS);
	Rborder_set_corner_radius(chip, 100.0f, 100.0f, 100.0f, 100.0f);
	Rborder_set_padding(chip, 14.0f, 6.0f, 14.0f, 6.0f);
	Rslot_set_halign(chip, R_HALIGN_LEFT);
	Rslot_set_valign(chip, R_VALIGN_BOTTOM);
	Rslot_set_padding(chip, 12.0f, 0.0f, 0.0f, 12.0f);
	(void)title;

	/* the cards pop in one after the other */
	Rwidget_set_transition(card, R_TRANSITION_POP, 0.30f + 0.06f * (float)index);
}

/* Shows where the player is: the time, the slider and the progress. */
static void show_time(void) {
	char text[16];
	int total = (int)app.seconds;
	snprintf(text, sizeof(text), "%d:%02d", total / 60, total % 60);
	Rlabel_set_text(app.time, text);
	Rslider_set_value(app.position, (float)(app.seconds / 240.0));
	Rprogress_bar_set_value(app.progress, (float)(app.seconds / 240.0));
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "theme.toggle") == 0) {
		Rtheme_apply(Rcheckbox_is_checked(sender) ? R_THEME_GLASS_DARK : R_THEME_GLASS_LIGHT);
	} else if (strcmp(action, "player.toggle") == 0) {
		app.playing = !app.playing;
		Rbutton_set_text(app.play, app.playing ? "Pause" : "Play");
		/* frames are only needed while the time moves */
		Rwindow_set_continuous_redraw(app.window, app.playing);
	} else if (strcmp(action, "player.seek") == 0) {
		/* the slider was dragged: the time goes where it was left */
		app.seconds = Rslider_get_value(sender) * 240.0;
		show_time();
	} else if (strcmp(action, "menu.open") == 0) {
		/* a menu is a popup of glass: it opens under the button */
		float x, y, width, height;
		Rwidget_get_rect(sender, &x, &y, &width, &height);
		Rmenu_open(app.menu, app.window, x, y + height + 8.0f);
	} else if (strcmp(action, "app.quit") == 0) {
		Rquit(0);
	}
}

/* Called once per frame: the player advances while it plays. */
static void on_frame(double delta_seconds, void* user_data) {
	(void)user_data;
	if (!app.playing || !Rwindow_is_valid(app.window)) return;
	app.seconds += delta_seconds;
	if (app.seconds > 240.0) app.seconds = 0.0;
	show_time();
}

int main(int argc, char** argv) {
	/* "raven_glass light" starts with the light glass */
	int light = argc > 1 && strcmp(argv[1], "light") == 0;
	Rid grid, item;
	int i;

	if (!example_init("glass")) return 1;

	/* before the window is built: sizes depend on the metrics of the theme */
	Rtheme_apply(light ? R_THEME_GLASS_LIGHT : R_THEME_GLASS_DARK);

	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/glass.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.page = Rwidget_find(app.window, "page");
	app.play = Rwidget_find(app.window, "play");
	app.position = Rwidget_find(app.window, "position");
	app.time = Rwidget_find(app.window, "time");
	app.progress = Rwidget_find(app.window, "progress");
	Rcheckbox_set_checked(Rwidget_find(app.window, "dark"), !light);
	app.playing = 1;
	app.seconds = 84.0;

	grid = Rwidget_find(app.window, "cards");
	for (i = 0; i < CARD_COUNT; i++) add_card(grid, i);

	/* the menu of the "More" button */
	app.menu = Rwidget_create(R_WIDGET_MENU);
	Rmenu_add_item(app.menu, "Add to library");
	Rmenu_add_item(app.menu, "Add to a playlist");
	item = Rmenu_add_item(app.menu, "Lossless audio");
	Rmenu_item_set_checkable(item, R_TRUE);
	Rmenu_item_set_checked(item, R_TRUE);
	Rmenu_add_separator(app.menu);
	item = Rmenu_add_item(app.menu, "Quit");
	Rmenu_item_set_shortcut(item, R_KEY_Q, R_MOD_CTRL);
	Rwidget_set_action(item, "app.quit");

	Raction_set_default_handler(on_action, NULL);
	Rset_frame_callback(on_frame, NULL);
	/* the player moves: keep producing frames */
	Rwindow_set_continuous_redraw(app.window, R_TRUE);

	/* the loop belongs to the application: Raven draws one frame per call,
	 * and Rwait_frame sleeps until there is something to draw */
	if (!example_automated(app.window)) {
		while (Rwait_frame(-1.0)) {
		}
	}

	Rshutdown();
	return 0;
}
