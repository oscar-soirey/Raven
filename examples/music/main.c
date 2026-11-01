/**
 * Raven example - music
 * A music player, to show what Raven can do: only the interface, no server
 * and no sound (the songs, made up, "play" by the clock).
 *
 *   ui/main.xml          the window; each part in its own file (<Include>)
 *   ui/widgets.xml       the tags of the player: <TrackRow>, <AlbumCard>...
 *   themes/              the themes of the player; any look of Raven works too
 *   covers/              the covers: 01.jpg, 02.jpg... (drawn ones when missing)
 *
 * The application gives the interface what changes:
 *   - lists of rows and cards, written as XML with the declared tags and
 *     made into widgets (Rwidget_create_from_xml);
 *   - the accent of the theme and the colors of the background, taken from
 *     the cover playing (Rtheme_set_accent, Rwidget_set_attribute);
 *   - tags of its own (tags.c): <Icon>, <Cover>, <Visualizer>, <Heading>.
 * Every button fires an action by name (OnClicked="player.next"): those
 * that carry a number ("track.play.42") go to one handler.
 *
 * The island at the top shows the song, and unfolds into a small player;
 * on a MacBook it lives in the notch.
 *
 *   Space        play / pause         Ctrl+Left / Right   previous / next
 *   Ctrl+L       like the song        Ctrl+F              search
 *   Ctrl+P       settings             Alt+Left            back
 */

#include "../common/example.h"
#include "music.h"

#include <math.h>
#include <stdarg.h>
#include <time.h>

enum { PAGE_HOME, PAGE_COLLECTION, PAGE_SEARCH, PAGE_LIBRARY, PAGE_SETTINGS };
enum { COLLECTION_ALBUM, COLLECTION_PLAYLIST };
enum { REPEAT_OFF, REPEAT_ALL, REPEAT_ONE };

/* the looks the player offers: its three themes, then the looks of Raven */
static const struct { const char* name; const char* theme; RLook look; } g_looks[] = {
	{ "Nocturne", "Nocturne", R_LOOK_RAVEN_DARK },
	{ "Aurora glass", "Aurora glass", R_LOOK_RAVEN_GLASS_DARK },
	{ "Daylight", "Daylight", R_LOOK_RAVEN_LIGHT },
	{ "Raven dark", NULL, R_LOOK_RAVEN_DARK },
	{ "Raven light", NULL, R_LOOK_RAVEN_LIGHT },
	{ "Raven glass dark", NULL, R_LOOK_RAVEN_GLASS_DARK },
	{ "Raven glass light", NULL, R_LOOK_RAVEN_GLASS_LIGHT },
	{ "High contrast", NULL, R_LOOK_RAVEN_HIGH_CONTRAST_DARK },
	{ "Windows 11 dark", NULL, R_LOOK_WINDOWS_DARK },
	{ "Windows 11 light", NULL, R_LOOK_WINDOWS_LIGHT },
	{ "macOS dark", NULL, R_LOOK_MACOS_DARK },
	{ "macOS light", NULL, R_LOOK_MACOS_LIGHT },
	{ "macOS 26 glass dark", NULL, R_LOOK_MACOS_GLASS_DARK },
	{ "GNOME dark", NULL, R_LOOK_GNOME_DARK },
	{ "GNOME light", NULL, R_LOOK_GNOME_LIGHT },
	{ "This system", NULL, R_LOOK_NATIVE_DARK }
};
#define LOOK_COUNT ((int)(sizeof(g_looks) / sizeof(g_looks[0])))

static const char* const g_effects[] = { "mesh", "aurora", "plasma", "spotlight", "shimmer" };

#define MAX_ROWS 64

static struct {
	Rid window, pages, backdrop, island, nowplaying, ticker, notice_timer;

	/* the player */
	int queue[MUSIC_MAX_TRACKS];
	int queue_count, queue_position;
	int track;
	double position, last_tick;
	int playing, shuffle, repeat, muted;
	float volume;
	char context[96];

	/* the page of an album or a playlist */
	int collection_kind, collection_index;
	int collection_tracks[MUSIC_MAX_TRACKS];
	int collection_count;
	int liked_albums[MUSIC_ALBUMS];
	Rid rows[MAX_ROWS];
	int row_tracks[MAX_ROWS];
	int row_count;

	/* the pages */
	int page, history[32], history_count;
	int library_filter;
	int search_top_album;

	/* the look */
	int look;
	int follow_cover, animate, effect;
	float accent[3], picked[3];
} app;

/* ============================================================================
 *  Small things
 * ========================================================================== */

static Rid find(const char* name) {
	return Rwidget_find(app.window, name);
}

static void set_text(const char* name, const char* text) {
	Rid label = find(name);
	if (label != R_INVALID_ID) Rlabel_set_text(label, text);
}

/* Writes a piece of interface with the tags of widgets.xml and adds it to a panel. */
static Rid add_xml(Rid parent, const char* format, ...) {
	char xml[2048];
	va_list args;
	Rid widget;
	va_start(args, format);
	vsnprintf(xml, sizeof(xml), format, args);
	va_end(args);
	widget = Rwidget_create_from_xml(xml, strlen(xml));
	if (widget != R_INVALID_ID && parent != R_INVALID_ID) Rwidget_add_child(parent, widget);
	return widget;
}

static void clear(const char* name) {
	Rid panel = find(name);
	if (panel != R_INVALID_ID) Rwidget_clear_children(panel);
}

static void hex(const float* rgb, float scale, char* out, size_t size) {
	int r = (int)(fminf(fmaxf(rgb[0] * scale, 0.0f), 1.0f) * 255.0f);
	int g = (int)(fminf(fmaxf(rgb[1] * scale, 0.0f), 1.0f) * 255.0f);
	int b = (int)(fminf(fmaxf(rgb[2] * scale, 0.0f), 1.0f) * 255.0f);
	snprintf(out, size, "#%02X%02X%02X", r, g, b);
}

static int album_seconds(int album) {
	int t, total = 0;
	for (t = 0; t < g_albums[album].track_count; t++) total += g_tracks[g_albums[album].first_track + t].seconds;
	return total;
}

/* The liked songs are a playlist made when it is needed. */
static void make_liked_playlist(void) {
	playlist_t* liked = &g_playlists[0];
	int t;
	liked->count = 0;
	for (t = 0; t < g_track_count && liked->count < (int)(sizeof(liked->tracks) / sizeof(liked->tracks[0])); t++) {
		if (g_tracks[t].liked) liked->tracks[liked->count++] = t;
	}
}

static int is_dark_theme(void) {
	float r, g, b, a;
	Rtheme_get_color(R_COLOR_WINDOW, &r, &g, &b, &a);
	return 0.2126f * r + 0.7152f * g + 0.0722f * b < 0.5f;
}

/* A short word in the folded island, for a moment. */
static void on_notice_end(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	app.notice_timer = R_INVALID_ID;
	set_text("island.title", app.track >= 0 ? g_tracks[app.track].title : "Raven Music");
}

static void notice(const char* text) {
	set_text("island.title", text);
	if (app.notice_timer != R_INVALID_ID) Rtimer_destroy(app.notice_timer);
	app.notice_timer = Rtimer_create(2.5, R_FALSE, on_notice_end, NULL);
}

/* ============================================================================
 *  The colors of the cover
 * ========================================================================== */

static void apply_accent(void) {
	float rgb[3];
	int i;
	if (app.follow_cover && app.track >= 0) {
		const album_t* album = &g_albums[g_tracks[app.track].album];
		memcpy(rgb, album->color, sizeof(rgb));
		/* on a light window, a darker shade of it */
		if (!is_dark_theme()) for (i = 0; i < 3; i++) rgb[i] *= 0.62f;
	} else {
		memcpy(rgb, app.picked, sizeof(rgb));
	}
	memcpy(app.accent, rgb, sizeof(rgb));
	Rtheme_set_accent(rgb[0], rgb[1], rgb[2]);
	/* the rows of the song playing are written in it */
	for (i = 0; i < app.row_count; i++) {
		Rid heart;
		if (!Rwidget_is_valid(app.rows[i])) continue;
		if (app.row_tracks[i] == app.track) Rwidget_set_color(app.rows[i], R_COLOR_TEXT, rgb[0], rgb[1], rgb[2], 1.0f);
		else Rwidget_clear_color(app.rows[i], R_COLOR_TEXT);
		/* and their hearts that are on */
		heart = Rwidget_find(app.rows[i], "row.heart");
		if (heart != R_INVALID_ID && g_tracks[app.row_tracks[i]].liked) {
			Rimage_set_tint_text(heart, R_FALSE);
			Rimage_set_tint_color(heart, rgb[0], rgb[1], rgb[2], 1.0f);
		}
	}
}

static void apply_background(void) {
	char a[16], b[16];
	const album_t* album = app.track >= 0 ? &g_albums[g_tracks[app.track].album] : &g_albums[0];
	int dark = is_dark_theme();
	if (app.effect >= (int)(sizeof(g_effects) / sizeof(g_effects[0]))) {
		Rwidget_set_visibility(app.backdrop, R_COLLAPSED);
		return;
	}
	Rwidget_set_visibility(app.backdrop, R_VISIBLE);
	Rwidget_set_attribute(app.backdrop, "Effect", g_effects[app.effect]);
	hex(album->color, dark ? 0.55f : 1.0f, a, sizeof(a));
	hex(album->color2, dark ? 0.35f : 0.9f, b, sizeof(b));
	Rwidget_set_attribute(app.backdrop, "EffectColor", a);
	Rwidget_set_attribute(app.backdrop, "EffectColor2", b);
	Rwidget_set_attribute(app.backdrop, "EffectAnimate", app.animate ? "true" : "false");
	Rwidget_set_opacity(app.backdrop, dark ? 0.9f : 0.45f);
}

/* ============================================================================
 *  The song playing
 * ========================================================================== */

static void update_like_icons(void) {
	const char* glyph = app.track >= 0 && g_tracks[app.track].liked ? "heart-filled" : "heart";
	static const char* const names[] = { "player.like.icon", "np.like.icon", "island.like.icon" };
	int i;
	for (i = 0; i < 3; i++) {
		Rid icon = find(names[i]);
		if (icon == R_INVALID_ID) continue;
		tags_icon_set(icon, glyph);
		/* a heart that is on wears the accent */
		if (glyph[5]) {
			Rimage_set_tint_text(icon, R_FALSE);
			Rimage_set_tint_color(icon, app.accent[0], app.accent[1], app.accent[2], 1.0f);
		} else {
			Rimage_set_tint_text(icon, R_TRUE);
		}
	}
}

static void update_play_icons(void) {
	static const char* const names[] = { "player.play.icon", "island.play.icon", "collection.play.icon" };
	int i;
	for (i = 0; i < 3; i++) {
		Rid icon = find(names[i]);
		int playing_here = app.playing;
		/* the button of a page plays that page: it is a pause only while that page plays */
		if (i == 2) {
			playing_here = app.playing && app.queue_count == app.collection_count && app.queue_count > 0 &&
				memcmp(app.queue, app.collection_tracks, sizeof(int) * (size_t)app.queue_count) == 0;
		}
		if (icon != R_INVALID_ID) tags_icon_set(icon, playing_here ? "pause" : "play");
	}
	tags_set_playing(app.playing);
	Rwindow_set_continuous_redraw(app.window, app.playing || app.animate);
}

static void update_progress(void) {
	char elapsed[16], length[16];
	int seconds = app.track >= 0 ? g_tracks[app.track].seconds : 0;
	float fraction = seconds > 0 ? (float)(app.position / seconds) : 0.0f;
	Rid seek;
	library_format_time((int)app.position, elapsed, sizeof(elapsed));
	library_format_time(seconds, length, sizeof(length));
	set_text("player.elapsed", elapsed);
	set_text("player.length", length);
	set_text("island.elapsed", elapsed);
	set_text("island.length", length);
	if ((seek = find("player.seek")) != R_INVALID_ID) Rslider_set_value(seek, fraction);
	if ((seek = find("island.seek")) != R_INVALID_ID) Rslider_set_value(seek, fraction);
}

static void fill_queue_panel(void) {
	Rid list = find("np.queue");
	int i, shown = 0;
	if (list == R_INVALID_ID) return;
	Rwidget_clear_children(list);
	for (i = app.queue_position + 1; i < app.queue_count && shown < 4; i++, shown++) {
		const track_t* track = &g_tracks[app.queue[i]];
		char title[160], artist[160];
		library_escape(track->title, title, sizeof(title));
		library_escape(g_albums[track->album].artist, artist, sizeof(artist));
		add_xml(list, "<LibraryRow Album=\"%d\" Title=\"%s\" Subtitle=\"%s\" Action=\"queue.jump.%d\"/>", track->album, title, artist, i);
	}
	if (shown == 0) add_xml(list, "<Label Text=\"Nothing after this song.\" Class=\"muted\" Slot.Padding=\"4,6\"/>");
}

static void show_track(void) {
	const track_t* track = &g_tracks[app.track];
	const album_t* album = &g_albums[track->album];
	char text[160];
	int a;
	set_text("player.title", track->title);
	set_text("player.artist", album->artist);
	set_text("np.title", track->title);
	set_text("np.artist", album->artist);
	set_text("np.context", app.context);
	set_text("island.song", track->title);
	set_text("island.artist", album->artist);
	if (app.notice_timer == R_INVALID_ID) set_text("island.title", track->title);
	set_text("np.artistname", album->artist);
	snprintf(text, sizeof(text), "%d,%03d monthly listeners", 1 + (track->album * 37) % 90, (track->album * 211) % 1000);
	set_text("np.listeners", text);
	snprintf(text, sizeof(text), "%s makes %s from %s, %d. Their last album, %s, came out this year.",
		album->artist, album->genre, track->album % 2 ? "a small room" : "a big city", album->year, album->title);
	set_text("np.bio", text);
	{
		char c1[16], c2[16];
		hex(album->color, 0.7f, c1, sizeof(c1));
		hex(album->color2, 0.3f, c2, sizeof(c2));
		Rwidget_set_attribute(find("np.artistcard"), "EffectColor", c1);
		Rwidget_set_attribute(find("np.artistcard"), "EffectColor2", c2);
	}
	snprintf(text, sizeof(text), "%s \xC2\xB7 %s", track->title, album->artist);
	Rwindow_set_title(app.window, text);
	tags_set_current_album(track->album);
	/* the visualizer of the tile of the album playing, on Home */
	for (a = 0; a < MUSIC_ALBUMS; a++) {
		char name[32];
		Rid vis;
		snprintf(name, sizeof(name), "quick.vis.%d", a);
		if ((vis = find(name)) != R_INVALID_ID) Rwidget_set_visibility(vis, a == track->album ? R_VISIBLE : R_COLLAPSED);
	}
	apply_accent();
	apply_background();
	update_like_icons();
	update_play_icons();
	update_progress();
	fill_queue_panel();
}

static void play_position(int position) {
	if (app.queue_count == 0) return;
	if (position < 0) position = 0;
	if (position >= app.queue_count) position = 0;
	app.queue_position = position;
	app.track = app.queue[position];
	app.position = 0.0;
	app.playing = R_TRUE;
	g_tracks[app.track].plays++;
	show_track();
}

/* Plays a list of songs from one of them; with shuffle, the others in random order after it. */
static void play_list(const int* tracks, int count, int start, const char* context) {
	int i;
	if (count <= 0) return;
	memcpy(app.queue, tracks, sizeof(int) * (size_t)count);
	app.queue_count = count;
	snprintf(app.context, sizeof(app.context), "%s", context);
	if (app.shuffle) {
		int first = app.queue[start];
		app.queue[start] = app.queue[0];
		app.queue[0] = first;
		for (i = count - 1; i > 1; i--) {
			int j = 1 + rand() % i, swap = app.queue[i];
			app.queue[i] = app.queue[j];
			app.queue[j] = swap;
		}
		start = 0;
	}
	play_position(start);
}

static void on_tick(Rid timer, void* user_data) {
	double now = Rget_time();
	(void)timer;
	(void)user_data;
	if (app.playing && app.track >= 0) {
		app.position += now - app.last_tick;
		if (app.position >= g_tracks[app.track].seconds) {
			if (app.repeat == REPEAT_ONE) app.position = 0.0;
			else if (app.queue_position + 1 < app.queue_count) play_position(app.queue_position + 1);
			else if (app.repeat == REPEAT_ALL) play_position(0);
			else {
				app.playing = R_FALSE;
				app.position = 0.0;
				update_play_icons();
			}
		}
		update_progress();
	}
	app.last_tick = now;
}

/* ============================================================================
 *  The pages
 * ========================================================================== */

static void set_nav(void) {
	static const char* const names[] = { "nav.home", "", "nav.search", "nav.library", "nav.settings" };
	int i;
	for (i = 0; i < 5; i++) {
		Rid item = names[i][0] ? find(names[i]) : R_INVALID_ID;
		if (item == R_INVALID_ID) continue;
		if (i == app.page) Rwidget_add_class(item, "active");
		else Rwidget_remove_class(item, "active");
	}
}

static void show_page(int page, int remember) {
	if (remember && page != app.page && app.history_count < (int)(sizeof(app.history) / sizeof(app.history[0]))) {
		app.history[app.history_count++] = app.page;
	}
	app.page = page;
	Rswitcher_set_index(app.pages, page);
	set_nav();
}

static void add_track_row(Rid list, int track_index, int number, int show_album) {
	const track_t* track = &g_tracks[track_index];
	const album_t* album = &g_albums[track->album];
	char title[160], artist[160], album_title[160], length[16];
	Rid row;
	library_escape(track->title, title, sizeof(title));
	library_escape(album->artist, artist, sizeof(artist));
	library_escape(album->title, album_title, sizeof(album_title));
	library_format_time(track->seconds, length, sizeof(length));
	row = add_xml(list, "<TrackRow Track=\"%d\" Number=\"%d\" Title=\"%s\" Artist=\"%s\" Album=\"%s\" AlbumIndex=\"%d\" "
		"Time=\"%s\" Liked=\"%s\" ShowCover=\"%s\" ShowAlbum=\"%s\"/>",
		track_index, number, title, artist, album_title, track->album, length,
		track->liked ? "true" : "false", show_album ? "true" : "false", show_album ? "true" : "false");
	if (row != R_INVALID_ID && app.row_count < MAX_ROWS) {
		app.rows[app.row_count] = row;
		app.row_tracks[app.row_count++] = track_index;
	}
}

static void fill_collection_rows(void) {
	Rid list = find("collection.tracks");
	int i, is_album = app.collection_kind == COLLECTION_ALBUM;
	app.row_count = 0;
	Rwidget_clear_children(list);
	for (i = 0; i < app.collection_count; i++) add_track_row(list, app.collection_tracks[i], i + 1, !is_album);
	if (app.collection_count == 0) {
		add_xml(list, "<VBox Spacing=\"8\" Slot.Padding=\"12,24\"><Heading Text=\"Songs you like will appear here\" TextSize=\"20\"/>"
			"<Label Class=\"muted\" Text=\"Save songs by tapping the heart icon.\"/></VBox>");
	}
	apply_accent();
}

static void show_collection_header(int cover, const char* kind, const char* title, const char* description, const char* artist, const char* meta) {
	const album_t* album = &g_albums[cover];
	char c1[16], c2[16];
	tags_cover_set_album(find("collection.cover"), cover);
	tags_cover_set_album(find("collection.avatar"), cover);
	set_text("collection.kind", kind);
	set_text("collection.title", title);
	/* a long title in smaller letters */
	Rlabel_set_text_size(find("collection.title"), strlen(title) <= 12 ? 60.0f : strlen(title) <= 18 ? 46.0f : 38.0f);
	set_text("collection.description", description);
	Rwidget_set_visibility(find("collection.description"), description[0] ? R_VISIBLE : R_COLLAPSED);
	set_text("collection.artist", artist);
	set_text("collection.meta", meta);
	/* the top of the page in dark shades of the colors of the cover: its texts are white, whatever the theme */
	hex(album->color, 0.55f, c1, sizeof(c1));
	hex(album->color2, 0.22f, c2, sizeof(c2));
	Rwidget_set_attribute(find("collection.hero"), "EffectColor", c1);
	Rwidget_set_attribute(find("collection.hero"), "EffectColor2", c2);
	Rwidget_set_attribute(find("collection.hero"), "EffectAnimate", app.animate ? "true" : "false");
	Rscroll_box_set_offset(find("page.collection"), 0.0f, 0.0f);
}

static void update_collection_like(void) {
	Rid icon = find("collection.like.icon");
	int liked = app.collection_kind == COLLECTION_ALBUM && app.liked_albums[app.collection_index];
	if (icon == R_INVALID_ID) return;
	tags_icon_set(icon, liked ? "heart-filled" : "heart");
	if (liked) {
		Rimage_set_tint_text(icon, R_FALSE);
		Rimage_set_tint_color(icon, app.accent[0], app.accent[1], app.accent[2], 1.0f);
	} else {
		Rimage_set_tint_text(icon, R_TRUE);
	}
}

static void open_album(int index) {
	const album_t* album = &g_albums[index];
	char meta[96];
	int t, seconds = album_seconds(index);
	app.collection_kind = COLLECTION_ALBUM;
	app.collection_index = index;
	app.collection_count = album->track_count;
	for (t = 0; t < album->track_count; t++) app.collection_tracks[t] = album->first_track + t;
	snprintf(meta, sizeof(meta), "\xC2\xB7  %d  \xC2\xB7  %d songs, %d min %d s", album->year, album->track_count, seconds / 60, seconds % 60);
	show_collection_header(index, "ALBUM", album->title, "", album->artist, meta);
	set_text("collection.column", "");
	snprintf(meta, sizeof(meta), "%d. %s. Released by Raven Records under a license that does not exist.", album->year, album->genre);
	set_text("collection.footer", meta);
	fill_collection_rows();
	update_collection_like();
	update_play_icons();
	show_page(PAGE_COLLECTION, R_TRUE);
}

static void open_playlist(int index) {
	playlist_t* list = &g_playlists[index];
	char meta[96];
	int t, seconds = 0;
	if (index == 0) make_liked_playlist();
	app.collection_kind = COLLECTION_PLAYLIST;
	app.collection_index = index;
	app.collection_count = list->count;
	for (t = 0; t < list->count; t++) {
		app.collection_tracks[t] = list->tracks[t];
		seconds += g_tracks[list->tracks[t]].seconds;
	}
	snprintf(meta, sizeof(meta), "\xC2\xB7  %d songs, about %d h %02d min", list->count, seconds / 3600, (seconds / 60) % 60);
	show_collection_header(list->cover, "PLAYLIST", list->title, list->description, "Raven Music", meta);
	set_text("collection.column", "Album");
	set_text("collection.footer", "");
	fill_collection_rows();
	update_collection_like();
	update_play_icons();
	show_page(PAGE_COLLECTION, R_TRUE);
}

static void fill_sidebar(void) {
	Rid list = find("sidebar.list");
	static const char* const chips[] = { "filter.all", "filter.playlists", "filter.albums" };
	int i;
	if (list == R_INVALID_ID) return;
	Rwidget_clear_children(list);
	for (i = 0; i < 3; i++) {
		Rid chip = find(chips[i]);
		if (chip == R_INVALID_ID) continue;
		if (i == app.library_filter) Rwidget_add_class(chip, "on");
		else Rwidget_remove_class(chip, "on");
	}
	if (app.library_filter != 2) {
		for (i = 0; i < MUSIC_PLAYLISTS; i++) {
			char title[96], subtitle[96];
			if (i == 0) make_liked_playlist();
			library_escape(g_playlists[i].title, title, sizeof(title));
			snprintf(subtitle, sizeof(subtitle), "Playlist \xC2\xB7 %d songs", g_playlists[i].count);
			add_xml(list, "<LibraryRow Album=\"%d\" Title=\"%s\" Subtitle=\"%s\" Action=\"playlist.open.%d\"/>", g_playlists[i].cover, title, subtitle, i);
		}
	}
	if (app.library_filter != 1) {
		for (i = 0; i < MUSIC_ALBUMS; i++) {
			char title[160], subtitle[160], artist[128];
			library_escape(g_albums[i].title, title, sizeof(title));
			library_escape(g_albums[i].artist, artist, sizeof(artist));
			snprintf(subtitle, sizeof(subtitle), "Album \xC2\xB7 %s", artist);
			add_xml(list, "<LibraryRow Album=\"%d\" Title=\"%s\" Subtitle=\"%s\" Action=\"album.open.%d\"/>", i, title, subtitle, i);
		}
	}
}

static void add_album_card(Rid row, int album, int round) {
	char title[160], subtitle[160];
	if (round) {
		library_escape(g_albums[album].artist, title, sizeof(title));
		snprintf(subtitle, sizeof(subtitle), "Artist");
	} else {
		library_escape(g_albums[album].title, title, sizeof(title));
		char artist[128];
		library_escape(g_albums[album].artist, artist, sizeof(artist));
		snprintf(subtitle, sizeof(subtitle), "%d \xC2\xB7 %s", g_albums[album].year, artist);
	}
	add_xml(row, "<AlbumCard Album=\"%d\" Title=\"%s\" Subtitle=\"%s\" Action=\"album.open.%d\" Round=\"%s\" Appear=\"Fade\"/>",
		album, title, subtitle, album, round ? "true" : "false");
}

static void fill_home(void) {
	Rid quick = find("home.quick"), row;
	time_t now = time(NULL);
	struct tm* local = localtime(&now);
	int hour = local ? local->tm_hour : 20, i, order[MUSIC_ALBUMS];
	set_text("home.greeting", hour < 5 ? "Good night" : hour < 12 ? "Good morning" : hour < 18 ? "Good afternoon" : "Good evening");

	/* six tiles: the liked songs, playlists and albums */
	Rwidget_clear_children(quick);
	for (i = 0; i < 6; i++) {
		char title[160], action[48];
		int cover;
		if (i < 2) {
			int p = i == 0 ? 0 : 2;
			library_escape(g_playlists[p].title, title, sizeof(title));
			snprintf(action, sizeof(action), "playlist.open.%d", p);
			cover = g_playlists[p].cover;
		} else {
			int a = (i * 5 + 3) % MUSIC_ALBUMS;
			library_escape(g_albums[a].title, title, sizeof(title));
			snprintf(action, sizeof(action), "album.open.%d", a);
			cover = a;
		}
		add_xml(quick, "<QuickCard Album=\"%d\" Title=\"%s\" Action=\"%s\" Key=\"%s\" Slot.Column=\"%d\" Slot.Row=\"%d\"/>",
			cover, title, action, i < 2 ? "" : action + 11, i % 3, i / 3);
	}

	/* the playlists made for you */
	clear("home.mixes");
	row = find("home.mixes");
	for (i = 1; i < MUSIC_PLAYLISTS; i++) {
		char title[96], description[160];
		library_escape(g_playlists[i].title, title, sizeof(title));
		library_escape(g_playlists[i].description, description, sizeof(description));
		add_xml(row, "<AlbumCard Album=\"%d\" Title=\"%s\" Subtitle=\"%s\" Action=\"playlist.open.%d\" Appear=\"Fade\"/>",
			g_playlists[i].cover, title, description, i);
	}
	/* recently played: some albums */
	clear("home.recent");
	row = find("home.recent");
	for (i = 0; i < 10; i++) add_album_card(row, (i * 7 + 2) % MUSIC_ALBUMS, R_FALSE);
	/* artists, round */
	clear("home.artists");
	row = find("home.artists");
	for (i = 0; i < 10; i++) add_album_card(row, (i * 3 + 1) % MUSIC_ALBUMS, R_TRUE);
	/* the newest albums first */
	for (i = 0; i < MUSIC_ALBUMS; i++) order[i] = i;
	for (i = 1; i < MUSIC_ALBUMS; i++) {
		int j = i, value = order[i];
		while (j > 0 && g_albums[order[j - 1]].year < g_albums[value].year) {
			order[j] = order[j - 1];
			j--;
		}
		order[j] = value;
	}
	clear("home.new");
	row = find("home.new");
	for (i = 0; i < 10; i++) add_album_card(row, order[i], R_FALSE);
}

static void fill_library(void) {
	Rid playlists = find("library.playlists"), albums = find("library.albums");
	char text[128];
	int i, liked = 0;
	Rwidget_clear_children(playlists);
	Rwidget_clear_children(albums);
	make_liked_playlist();
	for (i = 0; i < MUSIC_PLAYLISTS; i++) {
		char title[96], subtitle[96];
		library_escape(g_playlists[i].title, title, sizeof(title));
		snprintf(subtitle, sizeof(subtitle), "%d songs", g_playlists[i].count);
		add_xml(playlists, "<AlbumCard Album=\"%d\" Title=\"%s\" Subtitle=\"%s\" Action=\"playlist.open.%d\"/>", g_playlists[i].cover, title, subtitle, i);
	}
	for (i = 0; i < MUSIC_ALBUMS; i++) {
		add_album_card(albums, i, R_FALSE);
		liked += app.liked_albums[i];
	}
	snprintf(text, sizeof(text), "%d playlists, %d albums (%d saved), %d liked songs.", MUSIC_PLAYLISTS, MUSIC_ALBUMS, liked, g_playlists[0].count);
	set_text("library.summary", text);
}

static void fill_genres(void) {
	Rid tiles = find("search.genres");
	int i;
	Rwidget_clear_children(tiles);
	for (i = 0; i < library_genre_count(); i++) {
		char color[16], name[48];
		float rgb[3];
		int a, cover = i % MUSIC_ALBUMS;
		library_genre_color(i, rgb);
		hex(rgb, 1.0f, color, sizeof(color));
		library_escape(library_genre_name(i), name, sizeof(name));
		for (a = 0; a < MUSIC_ALBUMS; a++) {
			if (strcmp(g_albums[a].genre, library_genre_name(i)) == 0) {
				cover = a;
				break;
			}
		}
		add_xml(tiles, "<GenreTile Title=\"%s\" Color=\"%s\" Album=\"%d\" Action=\"genre.open.%d\"/>", name, color, cover, i);
	}
}

/* ============================================================================
 *  Search
 * ========================================================================== */

static int contains(const char* text, const char* part) {
	size_t n = strlen(part), i, k;
	if (n == 0) return R_TRUE;
	for (i = 0; text[i]; i++) {
		for (k = 0; k < n && text[i + k]; k++) {
			char a = text[i + k], b = part[k];
			if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
			if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
			if (a != b) break;
		}
		if (k == n) return R_TRUE;
	}
	return R_FALSE;
}

static void run_search(const char* query) {
	Rid songs = find("search.songs"), albums = find("search.albums");
	int a, t, found_songs = 0, found_albums = 0, top = -1;
	if (!query[0]) {
		Rswitcher_set_index(find("search.view"), 0);
		return;
	}
	Rswitcher_set_index(find("search.view"), 1);
	Rwidget_clear_children(songs);
	Rwidget_clear_children(albums);
	app.row_count = 0;
	for (a = 0; a < MUSIC_ALBUMS; a++) {
		if (contains(g_albums[a].title, query) || contains(g_albums[a].artist, query) || contains(g_albums[a].genre, query)) {
			if (top < 0) top = a;
			if (found_albums < 10) add_album_card(albums, a, R_FALSE);
			found_albums++;
		}
	}
	for (t = 0; t < g_track_count && found_songs < 5; t++) {
		if (contains(g_tracks[t].title, query) || contains(g_albums[g_tracks[t].album].artist, query)) {
			if (top < 0) top = g_tracks[t].album;
			add_track_row(songs, t, found_songs + 1, R_FALSE);
			found_songs++;
		}
	}
	apply_accent();
	app.search_top_album = top;
	Rwidget_set_visibility(find("search.top"), top >= 0 ? R_VISIBLE : R_HIDDEN);
	if (top >= 0) {
		tags_cover_set_album(find("search.top.cover"), top);
		set_text("search.top.title", g_albums[top].title);
		set_text("search.top.artist", g_albums[top].artist);
	}
	{
		char text[160];
		snprintf(text, sizeof(text), "Nothing matches \"%.60s\". Try another word: a genre, a city, a color...", query);
		set_text("search.none", text);
		Rwidget_set_visibility(find("search.none"), top < 0 ? R_VISIBLE : R_COLLAPSED);
	}
}

/* ============================================================================
 *  The look
 * ========================================================================== */

static void sync_look_boxes(void) {
	static const char* const names[] = { "settings.look", "island.look" };
	int i;
	for (i = 0; i < 2; i++) {
		Rid box = find(names[i]);
		if (box != R_INVALID_ID && Rcombo_box_get_selected(box) != app.look) Rcombo_box_set_selected(box, app.look);
	}
}

static void use_look(int index) {
	if (index < 0 || index >= LOOK_COUNT) return;
	app.look = index;
	if (g_looks[index].theme) {
		Rtheme_use(g_looks[index].theme);
	} else {
		/* a look of Raven, and the classes of the player over it */
		Rlook_set(g_looks[index].look == R_LOOK_NATIVE_DARK ? R_LOOK_NATIVE_DARK : g_looks[index].look);
		Rtheme_use("Player over the look");
	}
	apply_accent();
	apply_background();
	update_like_icons();
	update_collection_like();
	sync_look_boxes();
}

static void fill_look_boxes(void) {
	static const char* const names[] = { "settings.look", "island.look" };
	int i, k;
	for (k = 0; k < 2; k++) {
		Rid box = find(names[k]);
		if (box == R_INVALID_ID) continue;
		Rcombo_box_clear_items(box);
		for (i = 0; i < LOOK_COUNT; i++) Rcombo_box_add_item(box, g_looks[i].name);
	}
	sync_look_boxes();
}

/* ============================================================================
 *  Actions
 * ========================================================================== */

static void action_nav(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	if (strcmp(action, "nav.home") == 0) show_page(PAGE_HOME, R_TRUE);
	else if (strcmp(action, "nav.search") == 0) {
		show_page(PAGE_SEARCH, R_TRUE);
		Rwidget_focus(find("search.query"));
	} else if (strcmp(action, "nav.library") == 0) {
		fill_library();
		show_page(PAGE_LIBRARY, R_TRUE);
	} else if (strcmp(action, "nav.settings") == 0) show_page(PAGE_SETTINGS, R_TRUE);
	else if (strcmp(action, "nav.back") == 0 && app.history_count > 0) show_page(app.history[--app.history_count], R_FALSE);
}

static void action_player(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "player.toggle") == 0) {
		if (app.track < 0) {
			open_album(0);
			play_list(app.collection_tracks, app.collection_count, 0, g_albums[0].title);
			return;
		}
		app.playing = !app.playing;
		update_play_icons();
	} else if (strcmp(action, "player.next") == 0) {
		if (app.queue_count) play_position(app.queue_position + 1 < app.queue_count ? app.queue_position + 1 : 0);
	} else if (strcmp(action, "player.previous") == 0) {
		/* a few seconds in, back to the start of the song; else the one before */
		if (app.position > 3.0 || app.queue_position == 0) {
			app.position = 0.0;
			update_progress();
		} else {
			play_position(app.queue_position - 1);
		}
	} else if (strcmp(action, "player.seek") == 0 && app.track >= 0) {
		app.position = Rslider_get_value(sender) * g_tracks[app.track].seconds;
		update_progress();
	} else if (strcmp(action, "player.shuffle") == 0) {
		Rid button = find("player.shuffle");
		app.shuffle = !app.shuffle;
		if (app.shuffle) Rwidget_set_color(button, R_COLOR_TEXT, app.accent[0], app.accent[1], app.accent[2], 1.0f);
		else Rwidget_clear_color(button, R_COLOR_TEXT);
		notice(app.shuffle ? "Shuffle on" : "Shuffle off");
	} else if (strcmp(action, "player.repeat") == 0) {
		Rid button = find("player.repeat");
		app.repeat = (app.repeat + 1) % 3;
		tags_icon_set(find("player.repeat.icon"), app.repeat == REPEAT_ONE ? "repeat-one" : "repeat");
		if (app.repeat != REPEAT_OFF) Rwidget_set_color(button, R_COLOR_TEXT, app.accent[0], app.accent[1], app.accent[2], 1.0f);
		else Rwidget_clear_color(button, R_COLOR_TEXT);
		notice(app.repeat == REPEAT_OFF ? "Repeat off" : app.repeat == REPEAT_ALL ? "Repeat all" : "Repeat this song");
	} else if (strcmp(action, "player.like") == 0 && app.track >= 0) {
		g_tracks[app.track].liked = !g_tracks[app.track].liked;
		update_like_icons();
		notice(g_tracks[app.track].liked ? "Added to Liked songs" : "Removed from Liked songs");
	} else if (strcmp(action, "player.volume") == 0) {
		app.volume = Rslider_get_value(sender);
		app.muted = app.volume <= 0.001f;
		tags_icon_set(find("player.mute.icon"), app.muted ? "muted" : "volume");
	} else if (strcmp(action, "player.mute") == 0) {
		app.muted = !app.muted;
		if (!app.muted && app.volume <= 0.001f) app.volume = 0.5f;
		Rslider_set_value(find("player.volume"), app.muted ? 0.0f : app.volume);
		tags_icon_set(find("player.mute.icon"), app.muted ? "muted" : "volume");
	}
}

static void action_collection(Rid sender, const char* action, void* user_data) {
	char context[96];
	(void)sender;
	(void)user_data;
	snprintf(context, sizeof(context), "%s", app.collection_kind == COLLECTION_ALBUM ? g_albums[app.collection_index].title : g_playlists[app.collection_index].title);
	if (strcmp(action, "collection.play") == 0) {
		int same = app.queue_count == app.collection_count && app.queue_count > 0 &&
			memcmp(app.queue, app.collection_tracks, sizeof(int) * (size_t)app.queue_count) == 0;
		if (same && app.shuffle == 0) {
			app.playing = !app.playing;
			update_play_icons();
		} else {
			play_list(app.collection_tracks, app.collection_count, 0, context);
		}
	} else if (strcmp(action, "collection.shuffle") == 0) {
		int keep = app.shuffle;
		app.shuffle = R_TRUE;
		play_list(app.collection_tracks, app.collection_count, app.collection_count > 0 ? rand() % app.collection_count : 0, context);
		app.shuffle = keep;
	} else if (strcmp(action, "collection.like") == 0) {
		if (app.collection_kind == COLLECTION_ALBUM) {
			app.liked_albums[app.collection_index] = !app.liked_albums[app.collection_index];
			notice(app.liked_albums[app.collection_index] ? "Saved to your library" : "Removed from your library");
		} else {
			notice("It is your playlist already");
		}
		update_collection_like();
	}
}

/* Rows rebuilt after the click that asked for it is over (the button clicked is in them). */
static void on_rebuild(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	if (app.page == PAGE_COLLECTION) {
		if (app.collection_kind == COLLECTION_PLAYLIST && app.collection_index == 0) {
			int t;
			make_liked_playlist();
			app.collection_count = g_playlists[0].count;
			for (t = 0; t < app.collection_count; t++) app.collection_tracks[t] = g_playlists[0].tracks[t];
		}
		fill_collection_rows();
	} else if (app.page == PAGE_SEARCH) {
		run_search(Rtext_input_get_text(find("search.query")));
	}
	update_like_icons();
	fill_sidebar();
}

/* The actions that carry a number: "album.open.3", "track.play.42"... */
static void action_numbered(Rid sender, const char* action, void* user_data) {
	const char* dot = strrchr(action, '.');
	int n = dot ? atoi(dot + 1) : 0;
	(void)sender;
	(void)user_data;
	if (strncmp(action, "album.open.", 11) == 0 && n >= 0 && n < MUSIC_ALBUMS) {
		open_album(n);
	} else if (strncmp(action, "playlist.open.", 14) == 0 && n >= 0 && n < MUSIC_PLAYLISTS) {
		open_playlist(n);
	} else if (strncmp(action, "track.play.", 11) == 0 && n >= 0 && n < g_track_count) {
		int i, start = -1;
		/* the list it is in plays from it: the page, else its album */
		if (app.page == PAGE_COLLECTION) {
			for (i = 0; i < app.collection_count; i++) if (app.collection_tracks[i] == n) start = i;
		}
		if (start >= 0) {
			play_list(app.collection_tracks, app.collection_count, start,
				app.collection_kind == COLLECTION_ALBUM ? g_albums[app.collection_index].title : g_playlists[app.collection_index].title);
		} else {
			const album_t* album = &g_albums[g_tracks[n].album];
			int list[MUSIC_MAX_TRACKS];
			for (i = 0; i < album->track_count; i++) list[i] = album->first_track + i;
			play_list(list, album->track_count, n - album->first_track, album->title);
		}
	} else if (strncmp(action, "track.like.", 11) == 0 && n >= 0 && n < g_track_count) {
		g_tracks[n].liked = !g_tracks[n].liked;
		notice(g_tracks[n].liked ? "Added to Liked songs" : "Removed from Liked songs");
		Rtimer_create(0.0, R_FALSE, on_rebuild, NULL);
	} else if (strncmp(action, "queue.jump.", 11) == 0 && n >= 0 && n < app.queue_count) {
		play_position(n);
	} else if (strncmp(action, "genre.open.", 11) == 0 && n >= 0 && n < library_genre_count()) {
		Rtext_input_set_text(find("search.query"), library_genre_name(n));
		run_search(library_genre_name(n));
	} else if (strncmp(action, "library.filter.", 15) == 0) {
		app.library_filter = strcmp(dot + 1, "playlists") == 0 ? 1 : strcmp(dot + 1, "albums") == 0 ? 2 : 0;
		fill_sidebar();
	} else if (strcmp(action, "home.chip") == 0) {
		notice("Only music here");
	} else {
		/* what this demonstration does not do */
		notice("Not in this demo");
	}
}

static void action_search(Rid sender, const char* action, void* user_data) {
	(void)action;
	(void)user_data;
	if (app.page != PAGE_SEARCH) show_page(PAGE_SEARCH, R_TRUE);
	run_search(Rtext_input_get_text(sender));
}

static void action_search_top(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	if (app.search_top_album >= 0) open_album(app.search_top_album);
}

static void action_look(Rid sender, const char* action, void* user_data) {
	(void)action;
	(void)user_data;
	use_look(Rcombo_box_get_selected(sender));
}

static void set_panel(int shown) {
	Rwidget_set_visibility(app.nowplaying, shown ? R_VISIBLE : R_COLLAPSED);
	Rcheckbox_set_checked(find("settings.panel"), shown);
}

static void action_settings(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "settings.follow") == 0) {
		app.follow_cover = Rcheckbox_is_checked(sender);
		apply_accent();
		update_like_icons();
	} else if (strcmp(action, "settings.accent") == 0) {
		float a;
		Rcolor_picker_get_color(sender, &app.picked[0], &app.picked[1], &app.picked[2], &a);
		app.follow_cover = R_FALSE;
		Rcheckbox_set_checked(find("settings.follow"), R_FALSE);
		apply_accent();
		update_like_icons();
	} else if (strcmp(action, "settings.effect") == 0) {
		app.effect = Rcombo_box_get_selected(sender);
		apply_background();
	} else if (strcmp(action, "settings.animate") == 0) {
		app.animate = Rcheckbox_is_checked(sender);
		apply_background();
		Rwidget_set_attribute(find("collection.hero"), "EffectAnimate", app.animate ? "true" : "false");
		Rwidget_set_attribute(find("np.artistcard"), "EffectAnimate", app.animate ? "true" : "false");
		update_play_icons();
	} else if (strcmp(action, "settings.panel") == 0) {
		set_panel(Rcheckbox_is_checked(sender));
	} else if (strcmp(action, "settings.panel.toggle") == 0 || strcmp(action, "np.close") == 0) {
		set_panel(strcmp(action, "np.close") == 0 ? R_FALSE : Rwidget_get_visibility(app.nowplaying) != R_VISIBLE);
	} else if (strcmp(action, "settings.crossfade") == 0) {
		char text[16];
		snprintf(text, sizeof(text), "%d s", (int)(Rslider_get_value(sender) + 0.5f));
		set_text("settings.crossfade.value", text);
	}
}

/* ============================================================================ */

int main(void) {
	static const char* const player_actions[] = {
		"player.toggle", "player.next", "player.previous", "player.seek", "player.shuffle", "player.repeat",
		"player.like", "player.volume", "player.mute"
	};
	static const char* const settings_actions[] = {
		"settings.follow", "settings.accent", "settings.effect", "settings.animate", "settings.panel",
		"settings.panel.toggle", "np.close", "settings.crossfade"
	};
	const char* look = getenv("RAVEN_MUSIC_LOOK");
	int i;

	if (!example_init("Raven Music")) return 1;
	srand((unsigned)time(NULL));

	/* what the interface needs before it is read: the covers, the tags of the application, the themes */
	library_init();
	tags_init();
	Rtheme_load(RAVEN_EXAMPLE_DIR "/themes/nocturne.xml");
	Rtheme_load(RAVEN_EXAMPLE_DIR "/themes/aurora.xml");
	Rtheme_load(RAVEN_EXAMPLE_DIR "/themes/daylight.xml");
	Rtheme_load(RAVEN_EXAMPLE_DIR "/themes/styles.xml");
	Rtheme_use("Nocturne");

	Raction_bind("nav.home", action_nav, NULL);
	Raction_bind("nav.search", action_nav, NULL);
	Raction_bind("nav.library", action_nav, NULL);
	Raction_bind("nav.settings", action_nav, NULL);
	Raction_bind("nav.back", action_nav, NULL);
	for (i = 0; i < (int)(sizeof(player_actions) / sizeof(player_actions[0])); i++) Raction_bind(player_actions[i], action_player, NULL);
	for (i = 0; i < (int)(sizeof(settings_actions) / sizeof(settings_actions[0])); i++) Raction_bind(settings_actions[i], action_settings, NULL);
	Raction_bind("collection.play", action_collection, NULL);
	Raction_bind("collection.shuffle", action_collection, NULL);
	Raction_bind("collection.like", action_collection, NULL);
	Raction_bind("search.changed", action_search, NULL);
	Raction_bind("search.top", action_search_top, NULL);
	Raction_bind("look.choose", action_look, NULL);
	Raction_set_default_handler(action_numbered, NULL);

	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/ui/main.xml");
	if (app.window == R_INVALID_ID) {
		tags_shutdown();
		library_shutdown();
		Rshutdown();
		return 1;
	}
	app.pages = find("pages");
	app.backdrop = find("backdrop");
	app.island = find("island");
	app.nowplaying = find("nowplaying");
	app.notice_timer = R_INVALID_ID;
	app.track = -1;
	app.volume = 0.7f;
	app.follow_cover = R_TRUE;
	app.animate = R_TRUE;
	app.picked[0] = 0.72f;
	app.picked[1] = 0.61f;
	app.picked[2] = 1.0f;
	app.liked_albums[2] = app.liked_albums[5] = R_TRUE;

	fill_home();
	fill_sidebar();
	fill_genres();
	fill_library();
	fill_look_boxes();
	show_page(PAGE_HOME, R_FALSE);

	/* something is already playing (paused) when the player opens */
	open_album(3);
	show_page(PAGE_HOME, R_FALSE);
	app.history_count = 0;
	play_list(app.collection_tracks, app.collection_count, 2, g_albums[3].title);
	app.position = 47.0;
	app.playing = getenv("RAVEN_MUSIC_PAUSED") == NULL;
	update_play_icons();
	update_progress();
	if (look) {
		for (i = 0; i < LOOK_COUNT; i++) if (strcmp(g_looks[i].name, look) == 0) use_look(i);
	}
	{
		const char* page = getenv("RAVEN_MUSIC_PAGE");
		if (page && strcmp(page, "album") == 0) open_album(7);
		else if (page && strcmp(page, "playlist") == 0) open_playlist(1);
		else if (page && strcmp(page, "search") == 0) {
			Rtext_input_set_text(find("search.query"), "the");
			run_search("the");
			show_page(PAGE_SEARCH, R_TRUE);
		} else if (page && strcmp(page, "genres") == 0) show_page(PAGE_SEARCH, R_TRUE);
		else if (page && strcmp(page, "library") == 0) {
			fill_library();
			show_page(PAGE_LIBRARY, R_TRUE);
		} else if (page && strcmp(page, "settings") == 0) show_page(PAGE_SETTINGS, R_TRUE);
		if (getenv("RAVEN_MUSIC_ISLAND")) Risland_set_expanded(app.island, R_TRUE);
	}

	app.last_tick = Rget_time();
	app.ticker = Rtimer_create(0.25, R_TRUE, on_tick, NULL);
	Rwindow_show(app.window);

	if (!example_automated(app.window)) {
		while (Rwait_frame(-1.0)) {
		}
	}
	Rtimer_destroy(app.ticker);
	tags_shutdown();
	library_shutdown();
	Rshutdown();
	return Rget_exit_code();
}
