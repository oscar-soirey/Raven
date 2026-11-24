/**
 * Raven - widgets/video_view.c
 * VideoView: the picture of a media player (media/media.c), fitted as an
 * Image is, with controls over its bottom made of ordinary widgets (buttons
 * with the icons of Raven, sliders, a label) in the dark twin of the theme,
 * as they sit on a picture.
 *
 * The picture is taken from the player when the widget is painted, on the UI
 * thread: hidden or scrolled away, no picture is decoded into the texture.
 * While it plays it asks for the next frame (the pictures come at the pace
 * of the display, the player says which one is due); paused, it asks for
 * nothing. The controls follow the player from rvn_video_view_player_changed,
 * which the player calls between frames, never while painting.
 */

#include "common.h"
#include "../media/media.h"
#include "../xml/xml.h"
#include "../core/fs.h"
#include "../render/renderer.h"

/* Seconds the controls stay after the pointer stopped moving, while it plays. */
#define RVN_VIDEO_CONTROLS_DELAY 3.0
/* Seconds Left / Right move. */
#define RVN_VIDEO_SEEK_STEP 5.0

typedef struct rvn_video_view_t {
	rvn_widget_t base;
	Rid player;
	RStretch stretch;
	int autoplay;
	/** The controls are asked for; they are shown now (rvn_video_sync_bar). */
	int controls;
	int bar_shown;
	/** The pointer moved, or a key was pressed, this little while ago: the controls stay. */
	int active;
	Rid hide_timer;
	int fullscreen;
	RVideoViewFullscreenCallback fullscreen_callback;
	void* fullscreen_user;
	/** The controls (children of the widget). */
	rvn_widget_t* bar;
	rvn_widget_t* play;
	rvn_widget_t* play_icon;
	rvn_widget_t* time;
	rvn_widget_t* seek;
	rvn_widget_t* mute;
	rvn_widget_t* mute_icon;
	rvn_widget_t* volume;
	rvn_widget_t* full;
	rvn_widget_t* full_icon;
	/** What they show, so that only what changed is told to them. */
	char time_text[48];
	int shown_playing, shown_silent, shown_full;
	/** The state last seen, for the action of the widget. */
	RMediaState state;
} rvn_video_view_t;

static rvn_video_view_t* rvn_video_from_user(void* user) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user);
	if (!widget || widget->cls->type != R_WIDGET_VIDEO_VIEW) return NULL;
	return (rvn_video_view_t*)widget;
}

/* The view a control belongs to. */
static rvn_video_view_t* rvn_video_of_control(Rid control) {
	rvn_widget_t* widget = rvn_widget_get(control);
	while (widget && widget->cls->type != R_WIDGET_VIDEO_VIEW) widget = widget->parent;
	return (rvn_video_view_t*)widget;
}

Rid rvn_video_view_player_of(rvn_widget_t* widget) {
	return ((rvn_video_view_t*)widget)->player;
}

static const char* rvn_video_text(const char* key, const char* english) {
	return Rtr_exists(key) ? Rtr(key) : english;
}

/* "1:05", "1:02:05". */
static void rvn_video_format_time(char* out, size_t size, double seconds) {
	long total = seconds > 0.0 ? (long)floor(seconds + 1e-6) : 0;
	if (total >= 3600) snprintf(out, size, "%ld:%02ld:%02ld", total / 3600, (total / 60) % 60, total % 60);
	else snprintf(out, size, "%ld:%02ld", total / 60, total % 60);
}

/* ---------------------------------------------------------------------------
 *  The controls
 * ------------------------------------------------------------------------- */

static void rvn_video_stop_timer(rvn_video_view_t* view) {
	if (view->hide_timer != R_INVALID_ID && Rtimer_is_valid(view->hide_timer)) Rtimer_destroy(view->hide_timer);
	view->hide_timer = R_INVALID_ID;
}

/* Inside the controls: the pointer over them, or the keyboard in them. */
static int rvn_video_bar_in_use(rvn_video_view_t* view) {
	rvn_widget_t* focus = view->base.window ? view->base.window->focus : NULL;
	if (!view->bar) return R_FALSE;
	if (view->bar->flags & RVN_WF_HOVERED) return R_TRUE;
	if (view->seek && (view->seek->flags & RVN_WF_PRESSED)) return R_TRUE;
	if (view->volume && (view->volume->flags & RVN_WF_PRESSED)) return R_TRUE;
	return focus && focus != &view->base && rvn_widget_is_inside(focus, view->bar) && view->base.window->focus_visible;
}

/* Shows or hides the controls as the state of the player and of the pointer want. */
static void rvn_video_sync_bar(rvn_video_view_t* view) {
	int shown;
	if (!view->bar) return;
	shown = view->controls && (Rmedia_player_get_state(view->player) != R_MEDIA_PLAYING || view->active || rvn_video_bar_in_use(view));
	if (shown == view->bar_shown) return;
	view->bar_shown = shown;
	Rwidget_set_visibility(view->bar->id, shown ? R_VISIBLE : R_HIDDEN);
	rvn_widget_invalidate_paint(&view->base);
}

static void rvn_video_hide_later(Rid timer, void* user_data) {
	rvn_video_view_t* view = rvn_video_from_user(user_data);
	RVN_UNUSED(timer);
	if (!view) return;
	view->hide_timer = R_INVALID_ID;
	if (rvn_video_bar_in_use(view)) {
		/* (still in use: looked at again later) */
		view->hide_timer = Rtimer_create(RVN_VIDEO_CONTROLS_DELAY, R_FALSE, rvn_video_hide_later, user_data);
		return;
	}
	view->active = R_FALSE;
	rvn_video_sync_bar(view);
}

/* The user did something: the controls show, and hide again a while after. */
static void rvn_video_wake_controls(rvn_video_view_t* view) {
	if (!view->controls) return;
	view->active = R_TRUE;
	rvn_video_stop_timer(view);
	view->hide_timer = Rtimer_create(RVN_VIDEO_CONTROLS_DELAY, R_FALSE, rvn_video_hide_later, (void*)(uintptr_t)view->base.id);
	rvn_video_sync_bar(view);
}

static void rvn_video_set_icon(rvn_widget_t* image, const char* icon) {
	Rimage_set_texture(image->id, Ricon_get(icon));
}

static void rvn_video_set_tip(rvn_widget_t* button, const char* key, const char* english) {
	const char* text = rvn_video_text(key, english);
	rvn_str_set(&button->tooltip, text);
	Rwidget_set_accessible_name(button->id, text);
}

/* The controls say what the player is. */
static void rvn_video_sync(rvn_video_view_t* view) {
	Rid player = view->player;
	RMediaState state = Rmedia_player_get_state(player);
	double duration = Rmedia_player_get_duration(player), position = Rmedia_player_get_position(player);
	int playing = state == R_MEDIA_PLAYING || state == R_MEDIA_LOADING;
	int silent = Rmedia_player_is_muted(player) || Rmedia_player_get_volume(player) <= 0.0f;
	char text[48], total[24];

	if (!view->bar) return;
	if (playing != view->shown_playing) {
		view->shown_playing = playing;
		rvn_video_set_icon(view->play_icon, playing ? "pause" : "play");
		if (playing) rvn_video_set_tip(view->play, "raven.video.pause", "Pause");
		else rvn_video_set_tip(view->play, "raven.video.play", "Play");
	}
	rvn_video_format_time(text, sizeof(text), position);
	if (duration > 0.0) {
		rvn_video_format_time(total, sizeof(total), duration);
		snprintf(text + strlen(text), sizeof(text) - strlen(text), " / %s", total);
	}
	if (strcmp(text, view->time_text) != 0) {
		snprintf(view->time_text, sizeof(view->time_text), "%s", text);
		Rlabel_set_text(view->time->id, text);
	}
	if (!(view->seek->flags & RVN_WF_PRESSED)) {
		Rslider_set_range(view->seek->id, 0.0f, duration > 0.0 ? (float)duration : 1.0f);
		Rslider_set_value(view->seek->id, (float)position);
	}
	Rwidget_set_enabled(view->seek->id, duration > 0.0 && (state == R_MEDIA_PLAYING || state == R_MEDIA_PAUSED || state == R_MEDIA_ENDED));
	Rwidget_set_enabled(view->play->id, state != R_MEDIA_EMPTY && state != R_MEDIA_ERROR);
	if (silent != view->shown_silent) {
		view->shown_silent = silent;
		rvn_video_set_icon(view->mute_icon, silent ? "volume-off" : "volume");
		if (silent) rvn_video_set_tip(view->mute, "raven.video.unmute", "Sound on");
		else rvn_video_set_tip(view->mute, "raven.video.mute", "Mute");
	}
	if (!(view->volume->flags & RVN_WF_PRESSED)) Rslider_set_value(view->volume->id, Rmedia_player_is_muted(player) ? 0.0f : Rmedia_player_get_volume(player));
	if (view->fullscreen != view->shown_full) {
		view->shown_full = view->fullscreen;
		rvn_video_set_icon(view->full_icon, view->fullscreen ? "minimize" : "maximize");
		if (view->fullscreen) rvn_video_set_tip(view->full, "raven.video.exit_fullscreen", "Exit full screen");
		else rvn_video_set_tip(view->full, "raven.video.fullscreen", "Full screen");
	}
	Rwidget_set_visibility(view->full->id, view->fullscreen_callback ? R_VISIBLE : R_COLLAPSED);
	rvn_video_sync_bar(view);
}

static void rvn_video_toggle(rvn_video_view_t* view) {
	RMediaState state = Rmedia_player_get_state(view->player);
	if (state == R_MEDIA_PLAYING || (state == R_MEDIA_LOADING && Rvideo_view_get_autoplay(view->base.id))) Rmedia_player_pause(view->player);
	else Rmedia_player_play(view->player);
}

static void rvn_video_toggle_fullscreen(rvn_video_view_t* view) {
	Rid id = view->base.id;
	if (!view->fullscreen_callback) return;
	view->fullscreen = !view->fullscreen;
	rvn_video_sync(view);
	view->fullscreen_callback(id, view->fullscreen, view->fullscreen_user);
}

static void rvn_video_play_clicked(Rid button, void* user_data) {
	rvn_video_view_t* view = rvn_video_of_control(button);
	RVN_UNUSED(user_data);
	if (view) rvn_video_toggle(view);
}

static void rvn_video_mute_clicked(Rid button, void* user_data) {
	rvn_video_view_t* view = rvn_video_of_control(button);
	RVN_UNUSED(user_data);
	if (!view) return;
	if (Rmedia_player_is_muted(view->player) || Rmedia_player_get_volume(view->player) <= 0.0f) {
		Rmedia_player_set_muted(view->player, R_FALSE);
		/* (silent because the loudness is 0: it comes back audible) */
		if (Rmedia_player_get_volume(view->player) <= 0.0f) Rmedia_player_set_volume(view->player, 0.5f);
	} else {
		Rmedia_player_set_muted(view->player, R_TRUE);
	}
}

static void rvn_video_full_clicked(Rid button, void* user_data) {
	rvn_video_view_t* view = rvn_video_of_control(button);
	RVN_UNUSED(user_data);
	if (view) rvn_video_toggle_fullscreen(view);
}

static void rvn_video_seek_changed(Rid slider, float value, void* user_data) {
	rvn_video_view_t* view = rvn_video_of_control(slider);
	RVN_UNUSED(user_data);
	if (view) Rmedia_player_seek(view->player, (double)value);
}

static void rvn_video_volume_changed(Rid slider, float value, void* user_data) {
	rvn_video_view_t* view = rvn_video_of_control(slider);
	RVN_UNUSED(user_data);
	if (!view) return;
	Rmedia_player_set_volume(view->player, value);
	if (value > 0.0f) Rmedia_player_set_muted(view->player, R_FALSE);
}

static rvn_widget_t* rvn_video_button(rvn_video_view_t* view, const char* name, const char* icon, rvn_widget_t** image, RButtonClickedCallback clicked) {
	rvn_widget_t* button = rvn_widget_create(R_WIDGET_BUTTON);
	rvn_widget_t* picture = rvn_widget_create(R_WIDGET_IMAGE);
	rvn_widget_add_child(view->bar, button, -1);
	rvn_widget_add_child(button, picture, -1);
	Rwidget_set_name(button->id, name);
	Rbutton_set_style(button->id, R_BUTTON_FLAT);
	Rbutton_set_clicked_callback(button->id, clicked, NULL);
	button->slot.valign = R_VALIGN_CENTER;
	rvn_widget_set_attribute(button, "Padding", "6");
	picture->fixed_size = rvn_vec2(18.0f, 18.0f);
	Rimage_set_tint_text(picture->id, R_TRUE);
	rvn_video_set_icon(picture, icon);
	*image = picture;
	return button;
}

static void rvn_video_make_controls(rvn_video_view_t* view) {
	rvn_widget_t* widget = &view->base;
	if (view->bar) return;
	view->bar = rvn_widget_create(R_WIDGET_HBOX);
	rvn_widget_add_child(widget, view->bar, -1);
	Rwidget_set_name(view->bar->id, "controls");
	/* on a picture: the dark look of the theme, whatever the theme */
	rvn_widget_set_theme(view->bar, rvn_theme_dark_twin(), R_TRUE);
	rvn_widget_set_attribute(view->bar, "Padding", "8,4,10,6");
	rvn_widget_set_attribute(view->bar, "Spacing", "4");

	view->play = rvn_video_button(view, "play", "play", &view->play_icon, rvn_video_play_clicked);
	view->time = rvn_widget_create(R_WIDGET_LABEL);
	rvn_widget_add_child(view->bar, view->time, -1);
	Rwidget_set_name(view->time->id, "time");
	view->time->slot.valign = R_VALIGN_CENTER;
	rvn_widget_set_attribute(view->time, "Margin", "4,0,6,0");

	view->seek = rvn_widget_create(R_WIDGET_SLIDER);
	rvn_widget_add_child(view->bar, view->seek, -1);
	Rwidget_set_name(view->seek->id, "seek");
	view->seek->slot.is_fill = R_TRUE;
	view->seek->slot.fill = 1.0f;
	view->seek->slot.valign = R_VALIGN_CENTER;
	Rslider_set_changed_callback(view->seek->id, rvn_video_seek_changed, NULL);
	Rwidget_set_accessible_name(view->seek->id, rvn_video_text("raven.video.position", "Position"));

	view->mute = rvn_video_button(view, "mute", "volume", &view->mute_icon, rvn_video_mute_clicked);
	view->volume = rvn_widget_create(R_WIDGET_SLIDER);
	rvn_widget_add_child(view->bar, view->volume, -1);
	Rwidget_set_name(view->volume->id, "volume");
	view->volume->fixed_size.x = 72.0f;
	view->volume->slot.valign = R_VALIGN_CENTER;
	Rslider_set_range(view->volume->id, 0.0f, 1.0f);
	Rslider_set_step(view->volume->id, 0.05f);
	Rslider_set_changed_callback(view->volume->id, rvn_video_volume_changed, NULL);
	Rwidget_set_accessible_name(view->volume->id, rvn_video_text("raven.video.volume", "Volume"));

	view->full = rvn_video_button(view, "fullscreen", "maximize", &view->full_icon, rvn_video_full_clicked);
	view->shown_playing = view->shown_silent = view->shown_full = -1;
	view->bar_shown = R_TRUE;
	rvn_video_sync(view);
}

static void rvn_video_remove_controls(rvn_video_view_t* view) {
	if (!view->bar) return;
	rvn_widget_destroy(view->bar);
	view->bar = view->play = view->play_icon = view->time = view->seek = NULL;
	view->mute = view->mute_icon = view->volume = view->full = view->full_icon = NULL;
	view->time_text[0] = '\0';
}

/* ---------------------------------------------------------------------------
 *  The player tells
 * ------------------------------------------------------------------------- */

void rvn_video_view_player_changed(rvn_widget_t* widget, RMediaEvent event) {
	rvn_video_view_t* view = (rvn_video_view_t*)widget;
	RMediaState state = Rmedia_player_get_state(view->player);
	rvn_video_sync(view);
	rvn_widget_invalidate_paint(widget);
	if (event == R_MEDIA_EVENT_LOADED) rvn_widget_invalidate_layout(widget);
	if (event == R_MEDIA_EVENT_STATE && state != view->state) {
		view->state = state;
		/* playing again: the controls go away after a while */
		if (state == R_MEDIA_PLAYING && view->controls && !view->active) rvn_video_wake_controls(view);
		rvn_widget_fire_action(widget);
	}
}

/* ---------------------------------------------------------------------------
 *  Class
 * ------------------------------------------------------------------------- */

static void rvn_video_view_init(rvn_widget_t* widget) {
	rvn_video_view_t* view = (rvn_video_view_t*)widget;
	view->player = rvn_media_player_create_internal();
	rvn_media_player_set_view(view->player, widget->id);
	view->stretch = R_STRETCH_FIT;
	view->hide_timer = R_INVALID_ID;
	view->state = R_MEDIA_EMPTY;
}

static void rvn_video_view_destroy(rvn_widget_t* widget) {
	rvn_video_view_t* view = (rvn_video_view_t*)widget;
	/* (the controls are children: they go with the widget) */
	if (g_rvn.initialized) rvn_video_stop_timer(view);
	rvn_media_player_release(view->player);
	view->player = R_INVALID_ID;
}

static rvn_vec2_t rvn_video_view_measure(rvn_widget_t* widget) {
	rvn_video_view_t* view = (rvn_video_view_t*)widget;
	int width = 0, height = 0;
	rvn_vec2_t size = rvn_vec2(320.0f, 180.0f);
	if (Rmedia_player_get_video_size(view->player, &width, &height)) size = rvn_vec2((float)width, (float)height);
	if (view->bar) {
		rvn_vec2_t bar = rvn_widget_measure(view->bar);
		size.x = rvn_maxf(size.x, bar.x);
		size.y = rvn_maxf(size.y, bar.y);
	}
	return size;
}

static void rvn_video_view_arrange(rvn_widget_t* widget) {
	rvn_video_view_t* view = (rvn_video_view_t*)widget;
	if (view->bar) {
		float height = rvn_widget_measure(view->bar).y;
		rvn_widget_arrange(view->bar, 0.0f, widget->rect.h - height, widget->rect.w, height);
	}
}

/* A message in the middle: why nothing plays, the name of a sound. */
static void rvn_video_paint_message(rvn_widget_t* widget, RPainter* painter, const char* icon, const char* message, rvn_color_t color) {
	float size = rvn_widget_font_size(widget, 0.0f), gap = 12.0f;
	float icon_size = rvn_minf(48.0f, widget->rect.h * 0.3f);
	float middle = widget->rect.h * 0.5f;
	if (icon && icon_size >= 12.0f) {
		rvn_paint_image(painter, rvn_rect((widget->rect.w - icon_size) * 0.5f, middle - icon_size - 4.0f, icon_size, icon_size),
			Ricon_get(icon), NULL, color, 0.0f);
		middle += size;
	}
	rvn_paint_text_in(painter, R_INVALID_ID, size, rvn_rect(gap, middle - size, rvn_maxf(0.0f, widget->rect.w - gap * 2.0f), size * 2.0f),
		0.5f, message, -1, color);
}

static void rvn_video_view_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_video_view_t* view = (rvn_video_view_t*)widget;
	rvn_rect_t area = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	RMediaState state = Rmedia_player_get_state(view->player);
	rvn_color_t dim = rvn_rgba(1.0f, 1.0f, 1.0f, 0.7f);
	int playing = R_FALSE;
	Rid texture = rvn_media_player_frame(view->player, &playing);

	rvn_paint_rect(painter, area, rvn_rgba(0.0f, 0.0f, 0.0f, 1.0f));
	if (texture != R_INVALID_ID && state != R_MEDIA_ERROR) {
		static const float whole[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
		rvn_texture_t* frame = rvn_texture_get(texture);
		rvn_rect_t rect;
		float uv[4];
		rvn_vec2_t size = frame ? rvn_vec2((float)frame->width, (float)frame->height) : rvn_vec2(0.0f, 0.0f);
		int width = 0, height = 0;
		/* (the size to show it at: a picture with pixels that are not square says it) */
		if (Rmedia_player_get_video_size(view->player, &width, &height) && width > 0 && height > 0) size = rvn_vec2((float)width, (float)height);
		rvn_image_fit(view->stretch, size, widget->rect.w, widget->rect.h, whole, &rect, uv);
		rvn_paint_image(painter, rect, texture, uv, rvn_rgba(1.0f, 1.0f, 1.0f, 1.0f), 0.0f);
	} else if (!Rmedia_is_supported() && state == R_MEDIA_ERROR) {
		rvn_video_paint_message(widget, painter, "warning",
			rvn_video_text("raven.video.unsupported", "Video cannot be played here: this system has no media framework Raven can use."), dim);
	} else if (state == R_MEDIA_ERROR) {
		rvn_video_paint_message(widget, painter, "error", Rmedia_player_get_error(view->player), dim);
	} else if (state != R_MEDIA_EMPTY && state != R_MEDIA_LOADING && !Rmedia_player_get_video_size(view->player, NULL, NULL)) {
		/* a sound: its name */
		rvn_video_paint_message(widget, painter, "volume", rvn_path_name(Rmedia_player_get_source(view->player)), dim);
	}
	if (view->bar && view->bar_shown) {
		/* the controls stand out of the picture: it darkens toward the bottom */
		float height = view->bar->rect.h + 24.0f, top = widget->rect.h - height;
		int i, strips = 12;
		for (i = 0; i < strips; i++) {
			float t = (float)(i + 1) / (float)strips;
			rvn_paint_rect(painter, rvn_rect(0.0f, top + height * (float)i / (float)strips, widget->rect.w, height / (float)strips + 0.5f),
				rvn_rgba(0.0f, 0.0f, 0.0f, 0.62f * t * t));
		}
	}
	/* the next picture is due at the next frame: the display paces it */
	if (playing) rvn_paint_keep_animating(painter);
	rvn_paint_focus(widget, painter, 0.0f);
}

static int rvn_video_key(rvn_video_view_t* view, const REvent* event) {
	Rid player = view->player;
	double position = Rmedia_player_get_position(player), duration = Rmedia_player_get_duration(player);
	if (event->modifiers & (R_MOD_CTRL | R_MOD_ALT | R_MOD_SUPER)) return R_FALSE;
	switch (event->key) {
		case R_KEY_SPACE:
		case R_KEY_K:
			rvn_video_toggle(view);
			break;
		case R_KEY_LEFT:
			Rmedia_player_seek(player, position - RVN_VIDEO_SEEK_STEP);
			break;
		case R_KEY_RIGHT:
			Rmedia_player_seek(player, duration > 0.0 ? (position + RVN_VIDEO_SEEK_STEP < duration ? position + RVN_VIDEO_SEEK_STEP : duration) : position + RVN_VIDEO_SEEK_STEP);
			break;
		case R_KEY_HOME:
			Rmedia_player_seek(player, 0.0);
			break;
		case R_KEY_END:
			if (duration > 0.0) Rmedia_player_seek(player, duration);
			break;
		case R_KEY_UP:
			Rmedia_player_set_muted(player, R_FALSE);
			Rmedia_player_set_volume(player, Rmedia_player_get_volume(player) + 0.1f);
			break;
		case R_KEY_DOWN:
			Rmedia_player_set_volume(player, Rmedia_player_get_volume(player) - 0.1f);
			break;
		case R_KEY_M:
			Rmedia_player_set_muted(player, !Rmedia_player_is_muted(player));
			break;
		case R_KEY_F:
			if (!view->fullscreen_callback) return R_FALSE;
			rvn_video_toggle_fullscreen(view);
			break;
		case R_KEY_ESCAPE:
			if (!view->fullscreen || !view->fullscreen_callback) return R_FALSE;
			rvn_video_toggle_fullscreen(view);
			break;
		default:
			return R_FALSE;
	}
	if (rvn_widget_alive(&view->base)) rvn_video_wake_controls(view);
	return R_TRUE;
}

static int rvn_video_view_event(rvn_widget_t* widget, const REvent* event) {
	rvn_video_view_t* view = (rvn_video_view_t*)widget;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
		case R_EVENT_MOUSE_ENTER:
			rvn_video_wake_controls(view);
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			/* the pointer went away: the controls go with it, while it plays */
			view->active = R_FALSE;
			rvn_video_stop_timer(view);
			rvn_video_sync_bar(view);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN: {
			rvn_widget_t* target = rvn_widget_get(event->target);
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			/* (a click between the controls is not one on the picture) */
			if (target && view->bar && rvn_widget_is_inside(target, view->bar)) return R_TRUE;
			rvn_widget_focus(widget);
			if (event->clicks == 2 && view->fullscreen_callback) {
				/* the first click of the two played or paused: undone */
				rvn_video_toggle(view);
				rvn_video_toggle_fullscreen(view);
			} else {
				rvn_video_toggle(view);
			}
			if (rvn_widget_alive(widget)) rvn_video_wake_controls(view);
			return R_TRUE;
		}
		case R_EVENT_MOUSE_UP:
			return event->button == R_MOUSE_LEFT;
		case R_EVENT_KEY_DOWN:
			return rvn_video_key(view, event);
		case R_EVENT_FOCUS_GAINED:
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		default:
			return R_FALSE;
	}
}

static int rvn_video_view_child_active(rvn_widget_t* widget, rvn_widget_t* child) {
	rvn_video_view_t* view = (rvn_video_view_t*)widget;
	/* hidden controls take no click: it goes to the picture */
	return child != view->bar || view->bar_shown;
}

static int rvn_video_view_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_video_view_t* view = (rvn_video_view_t*)widget;
	int flag;
	float number;
	if (RVN_ATTR("Source")) {
		char* path;
		if (!value || !value[0]) {
			Rmedia_player_close(view->player);
			return R_TRUE;
		}
		path = rvn_xml_resolve_path(value);
		Rvideo_view_load_file(widget->id, path ? path : value);
		rvn_free(path);
	} else if (RVN_ATTR("Url")) {
		if (!value || !value[0]) Rmedia_player_close(view->player);
		else Rvideo_view_load_url(widget->id, value);
	} else if (RVN_ATTR("AutoPlay")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		Rvideo_view_set_autoplay(widget->id, flag);
	} else if (RVN_ATTR("Loop")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		Rmedia_player_set_loop(view->player, flag);
	} else if (RVN_ATTR("Muted")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		Rmedia_player_set_muted(view->player, flag);
	} else if (RVN_ATTR("Volume")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		Rmedia_player_set_volume(view->player, number);
	} else if (RVN_ATTR("Rate")) {
		if (!rvn_parse_float(value, &number) || number <= 0.0f) return R_FALSE;
		Rmedia_player_set_rate(view->player, number);
	} else if (RVN_ATTR("Controls")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		Rvideo_view_set_controls(widget->id, flag);
	} else if (RVN_ATTR("Stretch")) {
		RStretch stretch;
		if (!rvn_parse_stretch(value, &stretch)) return R_FALSE;
		Rvideo_view_set_stretch(widget->id, stretch);
	} else {
		return R_FALSE;
	}
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_video_view = {
	.type = R_WIDGET_VIDEO_VIEW,
	.name = "VideoView",
	.size = sizeof(rvn_video_view_t),
	/* painting takes the picture of the player into its texture: UI thread only */
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_UI_PAINT | RVN_CF_CLIP | RVN_CF_NO_MIRROR,
	.init = rvn_video_view_init,
	.destroy = rvn_video_view_destroy,
	.measure = rvn_video_view_measure,
	.arrange = rvn_video_view_arrange,
	.paint = rvn_video_view_paint,
	.event = rvn_video_view_event,
	.child_active = rvn_video_view_child_active,
	.set_attribute = rvn_video_view_set_attribute
};

/* ---------------------------------------------------------------------------
 *  API
 * ------------------------------------------------------------------------- */

Rid Rvideo_view_create(void) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_VIDEO_VIEW);
	return widget ? widget->id : R_INVALID_ID;
}

Rid Rvideo_view_get_player(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_VIDEO_VIEW, R_INVALID_ID);
	return ((rvn_video_view_t*)widget)->player;
}

static int rvn_video_view_loaded(rvn_widget_t* widget, int loaded) {
	rvn_video_view_t* view = (rvn_video_view_t*)widget;
	if (loaded && view->autoplay) Rmedia_player_play(view->player);
	rvn_video_sync(view);
	rvn_widget_invalidate_layout(widget);
	return loaded;
}

int Rvideo_view_load_file(Rid id, const char* path) {
	RVN_WIDGET(widget, id, R_WIDGET_VIDEO_VIEW, R_FALSE);
	return rvn_video_view_loaded(widget, Rmedia_player_load_file(((rvn_video_view_t*)widget)->player, path));
}

int Rvideo_view_load_url(Rid id, const char* url) {
	RVN_WIDGET(widget, id, R_WIDGET_VIDEO_VIEW, R_FALSE);
	return rvn_video_view_loaded(widget, Rmedia_player_load_url(((rvn_video_view_t*)widget)->player, url));
}

void Rvideo_view_set_autoplay(Rid id, int autoplay) {
	rvn_video_view_t* view;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_VIDEO_VIEW);
	view = (rvn_video_view_t*)widget;
	view->autoplay = autoplay ? R_TRUE : R_FALSE;
	/* (XML: AutoPlay after Source, while it loads) */
	if (view->autoplay && Rmedia_player_get_state(view->player) == R_MEDIA_LOADING) Rmedia_player_play(view->player);
}

int Rvideo_view_get_autoplay(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_VIDEO_VIEW, R_FALSE);
	return ((rvn_video_view_t*)widget)->autoplay;
}

void Rvideo_view_set_stretch(Rid id, RStretch stretch) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_VIDEO_VIEW);
	if (stretch < R_STRETCH_FILL || stretch > R_STRETCH_NONE) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rvideo_view_set_stretch: invalid stretch mode");
		return;
	}
	((rvn_video_view_t*)widget)->stretch = stretch;
	rvn_widget_invalidate_paint(widget);
}

RStretch Rvideo_view_get_stretch(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_VIDEO_VIEW, R_STRETCH_FIT);
	return ((rvn_video_view_t*)widget)->stretch;
}

void Rvideo_view_set_controls(Rid id, int shown) {
	rvn_video_view_t* view;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_VIDEO_VIEW);
	view = (rvn_video_view_t*)widget;
	view->controls = shown ? R_TRUE : R_FALSE;
	if (view->controls) {
		rvn_video_make_controls(view);
	} else {
		rvn_video_stop_timer(view);
		rvn_video_remove_controls(view);
		view->active = R_FALSE;
	}
	rvn_widget_invalidate_layout(widget);
}

int Rvideo_view_get_controls(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_VIDEO_VIEW, R_FALSE);
	return ((rvn_video_view_t*)widget)->controls;
}

int Rvideo_view_are_controls_visible(Rid id) {
	rvn_video_view_t* view;
	RVN_WIDGET(widget, id, R_WIDGET_VIDEO_VIEW, R_FALSE);
	view = (rvn_video_view_t*)widget;
	return view->bar != NULL && view->bar_shown;
}

void Rvideo_view_set_fullscreen_callback(Rid id, RVideoViewFullscreenCallback callback, void* user_data) {
	rvn_video_view_t* view;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_VIDEO_VIEW);
	view = (rvn_video_view_t*)widget;
	view->fullscreen_callback = callback;
	view->fullscreen_user = user_data;
	rvn_video_sync(view);
}

void Rvideo_view_set_fullscreen(Rid id, int fullscreen) {
	rvn_video_view_t* view;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_VIDEO_VIEW);
	view = (rvn_video_view_t*)widget;
	view->fullscreen = fullscreen ? R_TRUE : R_FALSE;
	rvn_video_sync(view);
}

int Rvideo_view_is_fullscreen(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_VIDEO_VIEW, R_FALSE);
	return ((rvn_video_view_t*)widget)->fullscreen;
}
