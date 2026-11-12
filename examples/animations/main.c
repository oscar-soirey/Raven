/**
 * Raven example - animations
 * Everything that moves, and how to ask for it.
 *
 *   transitions   Appear="FlyUp" in XML (animations.xml), or from C:
 *                 Rwidget_set_transition (each time the widget appears) and
 *                 Rwidget_play_transition (once, now)
 *   springs       RSpring + Rdraw_spring in a custom widget: a value that is
 *                 pulled to its target and may run past it; Rspring_kick
 *                 pushes it
 *   island        a pill at the top of the window that unfolds
 *                 (Risland_expand_for to tell something, then fold back)
 *   tuning        Rset_animation_speed, Rset_animations_enabled
 *
 * The controls animate by themselves (press a button, flip the switch): that
 * needs no code at all.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

#define MAX_CARDS 40

static struct {
	Rid window, card, card_title, cards, island, island_count, springs;
	Rid card_ids[MAX_CARDS];
	int card_count;
	int next_card;
	/* the springs playground: one spring per kind of motion, all pulled to the pointer */
	RSpring spring[4];
	RSpring glow[4];
	float target;
	int inside;
} app;

static const struct { const char* name; RTransition transition; } g_transitions[] = {
	{ "Fade", R_TRANSITION_FADE }, { "Pop", R_TRANSITION_POP }, { "Zoom", R_TRANSITION_ZOOM },
	{ "FlyUp", R_TRANSITION_FLY_UP }, { "FlyDown", R_TRANSITION_FLY_DOWN }, { "FlyLeft", R_TRANSITION_FLY_LEFT },
	{ "FlyRight", R_TRANSITION_FLY_RIGHT }, { "Drop", R_TRANSITION_DROP }, { "SlideLeft", R_TRANSITION_SLIDE_LEFT }
};
#define TRANSITION_COUNT ((int)(sizeof(g_transitions) / sizeof(g_transitions[0])))

/* ---------------------------------------------------------------------------
 *  Transitions
 * ------------------------------------------------------------------------- */

static void action_play(Rid sender, const char* action, void* user_data) {
	const char* name = action + 5;
	int i;
	(void)sender;
	(void)user_data;
	if (strcmp(name, "all") == 0) {
		/* every card of the list arrives again, one after the other */
		for (i = 0; i < app.card_count; i++) {
			Rwidget_play_transition(app.card_ids[i], g_transitions[(i + 1) % TRANSITION_COUNT].transition, 0.06f * (float)i);
		}
		Rwidget_play_transition(app.card, R_TRANSITION_DROP, 0.0f);
		Rlabel_set_text(app.card_title, "Drop");
		return;
	}
	for (i = 0; i < TRANSITION_COUNT; i++) {
		if (strcmp(g_transitions[i].name, name) != 0) continue;
		Rlabel_set_text(app.card_title, name);
		Rwidget_play_transition(app.card, g_transitions[i].transition, 0.0f);
	}
}

/* ---------------------------------------------------------------------------
 *  Cards that come and go
 * ------------------------------------------------------------------------- */

static void update_count(void) {
	char text[32];
	snprintf(text, sizeof(text), "%d card%s", app.card_count, app.card_count == 1 ? "" : "s");
	Rlabel_set_text(app.island_count, text);
}

static void add_card(float delay) {
	static const char* const colors[] = { "#F76B15", "#46A758", "#D6409F", "#05A2C2", "#8E4EC6", "#E5484D" };
	char title[32];
	Rid card, label;
	if (app.card_count >= MAX_CARDS) return;
	card = Rwidget_create(R_WIDGET_BORDER);
	Rwidget_set_attribute(card, "Padding", "12");
	Rwidget_set_attribute(card, "CornerRadius", "14");
	Rwidget_set_attribute(card, "Background", colors[app.next_card % 6]);
	snprintf(title, sizeof(title), "Card %d", ++app.next_card);
	label = Rwidget_add_child(card, Rlabel_create(title));
	Rlabel_set_color(label, 1.0f, 1.0f, 1.0f, 1.0f);
	/* it pops in each time it appears: now, and again if its panel is hidden and shown */
	Rwidget_set_transition(card, R_TRANSITION_POP, delay);
	Rwidget_add_child(app.cards, card);
	app.card_ids[app.card_count++] = card;
	update_count();
}

static void action_cards(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	if (strcmp(action, "cards.add") == 0) {
		add_card(0.0f);
	} else if (app.card_count > 0) {
		Rwidget_destroy(app.card_ids[--app.card_count]);
		update_count();
	}
	/* the island says it, then folds back by itself */
	Risland_expand_for(app.island, 1.6);
}

/* ---------------------------------------------------------------------------
 *  Springs: a custom widget
 * ------------------------------------------------------------------------- */

static void paint_springs(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	static const char* const names[4] = { "Snappy", "Smooth", "Bouncy", "Wobbly" };
	static const RMotion motions[4] = { R_MOTION_SNAPPY, R_MOTION_SMOOTH, R_MOTION_BOUNCY, R_MOTION_WOBBLY };
	static const float colors[4][3] = { { 0.23f, 0.51f, 0.96f }, { 0.27f, 0.65f, 0.35f }, { 0.97f, 0.42f, 0.08f }, { 0.84f, 0.25f, 0.62f } };
	float label = 70.0f, row = height / 4.0f, r, g, b, a;
	int i;
	(void)widget;
	(void)user_data;
	Rtheme_get_color(R_COLOR_TEXT, &r, &g, &b, &a);
	for (i = 0; i < 4; i++) {
		float y = row * ((float)i + 0.5f);
		float track = width - label - 16.0f;
		/* the same target for the four: only the way they get there differs */
		float x = label + 8.0f + Rdraw_spring(painter, &app.spring[i], app.target * track, motions[i]);
		float glow = Rdraw_spring(painter, &app.glow[i], app.inside ? 1.0f : 0.0f, R_MOTION_BOUNCY);
		Rdraw_text(painter, R_INVALID_ID, 0.0f, 0.0f, y - 9.0f, names[i], r, g, b, 0.8f);
		Rdraw_rect(painter, label, y - 2.0f, track + 16.0f, 4.0f, 2.0f, r, g, b, 0.12f);
		Rdraw_circle(painter, x, y, 9.0f + 5.0f * glow, colors[i][0], colors[i][1], colors[i][2], 0.25f);
		Rdraw_circle(painter, x, y, 8.0f, colors[i][0], colors[i][1], colors[i][2], 1.0f);
	}
}

static int springs_event(Rid widget, const REvent* event, void* user_data) {
	float x, y, width, height;
	int i;
	(void)user_data;
	Rwidget_get_rect(widget, &x, &y, &width, &height);
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			app.inside = R_TRUE;
			app.target = (event->local_x - 78.0f) / (width - 86.0f);
			if (app.target < 0.0f) app.target = 0.0f;
			if (app.target > 1.0f) app.target = 1.0f;
			Rwidget_invalidate(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_LEAVE:
			app.inside = R_FALSE;
			Rwidget_invalidate(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			/* a push to the right: each spring comes back in its own way */
			for (i = 0; i < 4; i++) Rspring_kick(&app.spring[i], 1400.0f);
			Rwidget_invalidate(widget);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

/* ---------------------------------------------------------------------------
 *  Tuning
 * ------------------------------------------------------------------------- */

static void action_motion(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "motion.speed") == 0) {
		char text[16];
		float speed = Rslider_get_value(sender);
		Rset_animation_speed(speed);
		snprintf(text, sizeof(text), "%.1fx", (double)speed);
		Rlabel_set_text(Rwidget_find(app.window, "speed.value"), text);
	} else {
		/* for the users who ask their system for less motion */
		Rset_animations_enabled(!Rcheckbox_is_checked(sender));
	}
}

int main(void) {
	int i;
	char action[32];

	if (!example_init("animations")) return 1;

	for (i = 0; i < TRANSITION_COUNT; i++) {
		snprintf(action, sizeof(action), "play.%s", g_transitions[i].name);
		Raction_bind(action, action_play, NULL);
	}
	Raction_bind("play.all", action_play, NULL);
	Raction_bind("cards.add", action_cards, NULL);
	Raction_bind("cards.remove", action_cards, NULL);
	Raction_bind("motion.speed", action_motion, NULL);
	Raction_bind("motion.reduce", action_motion, NULL);

	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/animations.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.card = Rwidget_find(app.window, "card");
	app.card_title = Rwidget_find(app.window, "card.title");
	app.cards = Rwidget_find(app.window, "cards");
	app.island = Rwidget_find(app.window, "island");
	app.island_count = Rwidget_find(app.window, "island.count");
	app.springs = Rwidget_find(app.window, "springs");
	app.target = 0.5f;

	Rcustom_set_paint_callback(app.springs, paint_springs, NULL);
	Rwidget_set_event_callback(app.springs, springs_event, NULL);
	/* the first cards arrive one after the other */
	for (i = 0; i < 3; i++) add_card(0.35f + 0.08f * (float)i);
	Rwidget_set_transition(app.card, R_TRANSITION_POP, 0.3f);

	example_run(app.window, NULL, NULL);
	return 0;
}
