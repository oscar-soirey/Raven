/**
 * Raven example - music
 * library.c : the albums, the tracks and the playlists, all made up, and
 * the covers.
 *
 * The covers are read from covers/01.jpg, 02.jpg... (or .png), in that
 * order; an album with no file gets a cover drawn here. Each cover gives
 * its album two colors (the most vivid ones of the picture): the accent of
 * the interface and the colors of the background while the album plays.
 */

#include "music.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

album_t g_albums[MUSIC_ALBUMS];
track_t g_tracks[MUSIC_MAX_TRACKS];
int g_track_count;
playlist_t g_playlists[MUSIC_PLAYLISTS];

/* ---- made-up names ---- */

static unsigned g_seed = 20261101u;

static unsigned next_random(void) {
	g_seed = g_seed * 1664525u + 1013904223u;
	return g_seed >> 8;
}

static int pick(int count) {
	return (int)(next_random() % (unsigned)count);
}

#define PICK(list) list[pick((int)(sizeof(list) / sizeof(list[0])))]

static const char* const g_adjectives[] = {
	"Velvet", "Neon", "Paper", "Glass", "Midnight", "Golden", "Silent", "Electric", "Hollow", "Crimson",
	"Northern", "Lucid", "Static", "Wild", "Slow", "Distant", "Burning", "Quiet", "Endless", "Broken",
	"Little", "Blue", "Copper", "Stray", "Silver", "Tender", "Restless", "Faded", "Bright", "Lonely"
};
static const char* const g_nouns[] = {
	"Harbor", "Satellites", "Weather", "Gardens", "Echoes", "Highways", "Motel", "Summer", "Tides", "Signals",
	"Lanterns", "Cathedral", "Rivers", "Daydreams", "Machines", "Orchards", "Avenue", "Fever", "Horizon", "Comets",
	"Heartbeat", "Mirrors", "Wolves", "Arcade", "Paradise", "Skyline", "Strangers", "Waves", "Embers", "Ghosts"
};
static const char* const g_places[] = {
	"Tokyo", "the Rain", "Lisbon", "the Dark", "July", "Berlin", "the Valley", "Room 9", "the Desert", "Brooklyn",
	"Marseille", "the Morning", "Seoul", "the Deep End", "Nowhere"
};
static const char* const g_verbs[] = {
	"Call", "Hold", "Wake", "Take", "Find", "Leave", "Save", "Show", "Keep", "Tell"
};
static const char* const g_adverbs[] = {
	"Slowly", "Again", "Tonight", "Home", "Closer", "Gently", "Back", "Forever"
};
static const char* const g_first[] = {
	"Mira", "Jonah", "Elio", "Sasha", "Noor", "Theo", "Lena", "Kai", "Ines", "Felix", "Ava", "Remy", "Yuna", "Otis", "Zadie", "Milo"
};
static const char* const g_last[] = {
	"Solen", "Hart", "Vale", "Okafor", "Lindqvist", "Moreau", "Kane", "Rivers", "Sato", "Bellamy", "Quinn", "Delacroix", "Ash", "Novak"
};
static const char* const g_genres[] = {
	"Pop", "Hip-Hop", "Rock", "Indie", "R&B", "Electronic", "Soul", "Folk", "Jazz", "Chill"
};
static const unsigned g_genre_colors[] = {
	0xE8115B, 0xBA5D07, 0xE91429, 0x608108, 0x8D67AB, 0x0D73EC, 0xDC148C, 0x477D95, 0x1E3264, 0x27856A
};

static void make_artist(char* out, size_t size) {
	switch (pick(4)) {
		case 0: snprintf(out, size, "The %s %s", PICK(g_adjectives), PICK(g_nouns)); break;
		case 1: snprintf(out, size, "%s %s", PICK(g_adjectives), PICK(g_nouns)); break;
		default: snprintf(out, size, "%s %s", PICK(g_first), PICK(g_last)); break;
	}
}

static void make_album_title(char* out, size_t size) {
	switch (pick(5)) {
		case 0: snprintf(out, size, "%s", PICK(g_nouns)); break;
		case 1: snprintf(out, size, "%s in %s", PICK(g_nouns), PICK(g_places)); break;
		case 2: snprintf(out, size, "The %s %s", PICK(g_adjectives), PICK(g_nouns)); break;
		default: snprintf(out, size, "%s %s", PICK(g_adjectives), PICK(g_nouns)); break;
	}
}

static void make_track_title(char* out, size_t size) {
	switch (pick(7)) {
		case 0: snprintf(out, size, "%s %s", PICK(g_adjectives), PICK(g_nouns)); break;
		case 1: snprintf(out, size, "%s in %s", PICK(g_nouns), PICK(g_places)); break;
		case 2: snprintf(out, size, "%s Me %s", PICK(g_verbs), PICK(g_adverbs)); break;
		case 3: snprintf(out, size, "The %s", PICK(g_nouns)); break;
		case 4: snprintf(out, size, "%s", PICK(g_nouns)); break;
		case 5: snprintf(out, size, "%s %s (Interlude)", PICK(g_adjectives), PICK(g_nouns)); break;
		default: snprintf(out, size, "%s, %s", PICK(g_places), PICK(g_adverbs)); break;
	}
}

/* ---- colors ---- */

static void rgb_to_hsv(float r, float g, float b, float* h, float* s, float* v) {
	float max = fmaxf(r, fmaxf(g, b)), min = fminf(r, fminf(g, b)), d = max - min;
	*v = max;
	*s = max > 0.0f ? d / max : 0.0f;
	if (d <= 0.0f) *h = 0.0f;
	else if (max == r) *h = fmodf((g - b) / d + 6.0f, 6.0f) / 6.0f;
	else if (max == g) *h = ((b - r) / d + 2.0f) / 6.0f;
	else *h = ((r - g) / d + 4.0f) / 6.0f;
}

static void hsv_to_rgb(float h, float s, float v, float* rgb) {
	float k[3] = { 5.0f, 3.0f, 1.0f };
	int i;
	for (i = 0; i < 3; i++) {
		float x = fmodf(k[i] + h * 6.0f, 6.0f);
		rgb[i] = v - v * s * fmaxf(0.0f, fminf(fminf(x, 4.0f - x), 1.0f));
	}
}

/* A color that reads well as an accent on a dark window: vivid enough, light enough. */
static void make_accent(const float* in, float* out) {
	float h, s, v;
	rgb_to_hsv(in[0], in[1], in[2], &h, &s, &v);
	s = fminf(fmaxf(s, 0.45f), 0.85f);
	v = fminf(fmaxf(v, 0.78f), 0.98f);
	hsv_to_rgb(h, s, v, out);
}

/* The two most vivid colors of a picture: twelve families of hue, weighted by how vivid each pixel is. */
static void cover_colors(const uint8_t* pixels, int width, int height, album_t* album, int genre) {
	float weight[12] = { 0 }, sum[12][3] = { { 0 } };
	int x, y, i, first = -1, second = -1, step = width / 48 > 0 ? width / 48 : 1;
	for (y = 0; y < height; y += step) {
		for (x = 0; x < width; x += step) {
			const uint8_t* p = pixels + ((size_t)y * width + x) * 4;
			float r = p[0] / 255.0f, g = p[1] / 255.0f, b = p[2] / 255.0f, h, s, v, w;
			int bin;
			rgb_to_hsv(r, g, b, &h, &s, &v);
			if (v < 0.18f) continue;
			w = s * s * v;
			bin = (int)(h * 12.0f) % 12;
			weight[bin] += w;
			sum[bin][0] += r * w;
			sum[bin][1] += g * w;
			sum[bin][2] += b * w;
		}
	}
	for (i = 0; i < 12; i++) if (first < 0 || weight[i] > weight[first]) first = i;
	for (i = 0; i < 12; i++) {
		int distance = abs(i - first);
		if (distance > 6) distance = 12 - distance;
		if (distance >= 2 && (second < 0 || weight[i] > weight[second])) second = i;
	}
	if (weight[first] < 2.0f) {
		/* a picture with almost no color (black and white): the color of its genre */
		unsigned c = g_genre_colors[genre];
		float base[3] = { ((c >> 16) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f, (c & 255) / 255.0f };
		make_accent(base, album->color);
		for (i = 0; i < 3; i++) album->color2[i] = album->color[i] * 0.35f;
		return;
	}
	{
		float base[3];
		for (i = 0; i < 3; i++) base[i] = sum[first][i] / weight[first];
		make_accent(base, album->color);
		if (second >= 0 && weight[second] > weight[first] * 0.12f) {
			for (i = 0; i < 3; i++) base[i] = sum[second][i] / weight[second];
			make_accent(base, album->color2);
		} else {
			float h, s, v;
			rgb_to_hsv(album->color[0], album->color[1], album->color[2], &h, &s, &v);
			hsv_to_rgb(fmodf(h + 0.12f, 1.0f), s, v * 0.7f, album->color2);
		}
	}
}

/* ---- the covers drawn here ---- */

#define DRAWN 256

static float smooth(float edge0, float edge1, float x) {
	float t = fminf(fmaxf((x - edge0) / (edge1 - edge0), 0.0f), 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

/* Four kinds of abstract covers, in the colors of the album. */
static uint8_t* draw_cover(int index, const float* a, const float* b) {
	uint8_t* pixels = (uint8_t*)malloc((size_t)DRAWN * DRAWN * 4);
	int x, y, kind = index % 4;
	unsigned noise = 12345u + (unsigned)index * 977u;
	for (y = 0; y < DRAWN; y++) {
		for (x = 0; x < DRAWN; x++) {
			float u = (x + 0.5f) / DRAWN, v = (y + 0.5f) / DRAWN, t, mix = 0.0f, light = 1.0f, grain;
			float c[3];
			int i;
			uint8_t* p = pixels + ((size_t)y * DRAWN + x) * 4;
			switch (kind) {
				case 0: { /* rings around a point */
					float dx = u - 0.32f, dy = v - 0.68f, d = sqrtf(dx * dx + dy * dy);
					mix = 0.5f + 0.5f * sinf(d * 38.0f);
					light = 1.05f - d * 0.6f;
					break;
				}
				case 1: { /* a sun over the sea */
					float dx = u - 0.5f, dy = v - 0.46f, d = sqrtf(dx * dx + dy * dy);
					float sun = 1.0f - smooth(0.2f, 0.205f, d);
					float band = v > 0.46f ? (fmodf(v * 30.0f, 1.0f) < 0.5f + (v - 0.46f) * 0.9f ? 1.0f : 0.0f) : 1.0f;
					mix = v;
					light = 0.35f + sun * band * 0.9f + (1.0f - sun) * 0.25f;
					break;
				}
				case 2: { /* stripes across */
					t = (u + v) * 7.0f;
					mix = smooth(0.45f, 0.55f, fmodf(t, 1.0f)) * 0.8f + u * 0.2f;
					light = 0.6f + 0.5f * (1.0f - v);
					break;
				}
				default: { /* soft blobs */
					float d1 = (u - 0.3f) * (u - 0.3f) + (v - 0.3f) * (v - 0.3f);
					float d2 = (u - 0.75f) * (u - 0.75f) + (v - 0.65f) * (v - 0.65f);
					mix = expf(-d2 * 9.0f) / (expf(-d1 * 9.0f) + expf(-d2 * 9.0f));
					light = 0.4f + 0.8f * fmaxf(expf(-d1 * 7.0f), expf(-d2 * 7.0f));
					break;
				}
			}
			noise = noise * 1103515245u + 12345u;
			grain = ((noise >> 16) & 255) / 255.0f * 0.06f - 0.03f;
			for (i = 0; i < 3; i++) {
				c[i] = (a[i] * (1.0f - mix) + b[i] * mix) * light + grain;
				p[i] = (uint8_t)(fminf(fmaxf(c[i], 0.0f), 1.0f) * 255.0f);
			}
			p[3] = 255;
		}
	}
	return pixels;
}

static Rid load_cover(int index, album_t* album, int genre) {
	static const char* const extensions[] = { "jpg", "png", "jpeg" };
	char path[512];
	int e, width = 0, height = 0;
	for (e = 0; e < 3; e++) {
		size_t size = 0;
		char* data;
		uint8_t* pixels;
		snprintf(path, sizeof(path), "%s/covers/%02d.%s", RAVEN_EXAMPLE_DIR, index + 1, extensions[e]);
		data = Rfile_read(path, &size);
		if (!data) continue;
		pixels = Rimage_decode(data, size, &width, &height);
		Rmemory_free(data);
		if (pixels) {
			Rid texture = Rtexture_create_from_pixels(pixels, width, height);
			cover_colors(pixels, width, height, album, genre);
			Rpixels_free(pixels);
			Rtexture_set_filter(texture, R_FILTER_MIPMAP);
			return texture;
		}
	}
	/* no file: a cover drawn in the colors of the genre, turned a little */
	{
		unsigned c = g_genre_colors[genre];
		float base[3] = { ((c >> 16) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f, (c & 255) / 255.0f }, h, s, v, other[3];
		uint8_t* pixels;
		Rid texture;
		rgb_to_hsv(base[0], base[1], base[2], &h, &s, &v);
		hsv_to_rgb(fmodf(h + 0.08f * (float)(index % 5) + 0.5f, 1.0f), fminf(s + 0.1f, 1.0f), 0.55f, other);
		pixels = draw_cover(index, base, other);
		texture = Rtexture_create_from_pixels(pixels, DRAWN, DRAWN);
		Rtexture_set_filter(texture, R_FILTER_MIPMAP);
		make_accent(base, album->color);
		make_accent(other, album->color2);
		free(pixels);
		return texture;
	}
}

/* ---- the library ---- */

void library_init(void) {
	static const char* const playlist_titles[MUSIC_PLAYLISTS] = {
		"Liked songs", "Late night drive", "Focus flow", "Sunday morning", "Heavy rotation", "Fresh finds"
	};
	static const char* const playlist_descriptions[MUSIC_PLAYLISTS] = {
		"The songs you marked with a heart.",
		"Long roads, city lights and the windows down.",
		"Quiet songs to keep your head down.",
		"Slow coffee, open windows, nowhere to be.",
		"What you have been playing the most lately.",
		"New songs picked for you, every Friday."
	};
	int a, t, p;
	g_track_count = 0;
	for (a = 0; a < MUSIC_ALBUMS; a++) {
		album_t* album = &g_albums[a];
		int genre = pick((int)(sizeof(g_genres) / sizeof(g_genres[0])));
		make_album_title(album->title, sizeof(album->title));
		make_artist(album->artist, sizeof(album->artist));
		snprintf(album->genre, sizeof(album->genre), "%s", g_genres[genre]);
		album->year = 2026 - pick(40) * (pick(3) == 0 ? 1 : 0) - pick(6);
		album->first_track = g_track_count;
		album->track_count = 6 + pick(7);
		for (t = 0; t < album->track_count && g_track_count < MUSIC_MAX_TRACKS; t++) {
			track_t* track = &g_tracks[g_track_count++];
			make_track_title(track->title, sizeof(track->title));
			track->album = a;
			track->seconds = 128 + pick(200);
			track->plays = 20000 + pick(4000) * pick(900);
			track->liked = pick(6) == 0;
		}
		album->track_count = g_track_count - album->first_track;
		album->texture = load_cover(a, album, genre);
	}
	for (p = 0; p < MUSIC_PLAYLISTS; p++) {
		playlist_t* list = &g_playlists[p];
		snprintf(list->title, sizeof(list->title), "%s", playlist_titles[p]);
		snprintf(list->description, sizeof(list->description), "%s", playlist_descriptions[p]);
		list->cover = p == 0 ? 15 : (p * 7 + 3) % MUSIC_ALBUMS;
		list->count = 0;
		/* (the liked songs are made when they are shown) */
		if (p == 0) continue;
		while (list->count < 14 + p * 3 && list->count < 40) {
			list->tracks[list->count++] = pick(g_track_count);
		}
	}
}

void library_shutdown(void) {
	int a;
	for (a = 0; a < MUSIC_ALBUMS; a++) {
		if (g_albums[a].texture != R_INVALID_ID) Rtexture_destroy(g_albums[a].texture);
		g_albums[a].texture = R_INVALID_ID;
	}
}

void library_format_time(int seconds, char* out, size_t size) {
	if (seconds < 0) seconds = 0;
	snprintf(out, size, "%d:%02d", seconds / 60, seconds % 60);
}

void library_escape(const char* text, char* out, size_t size) {
	size_t n = 0;
	for (; *text && n + 7 < size; text++) {
		const char* entity = NULL;
		switch (*text) {
			case '&': entity = "&amp;"; break;
			case '<': entity = "&lt;"; break;
			case '>': entity = "&gt;"; break;
			case '"': entity = "&quot;"; break;
			default: break;
		}
		if (entity) {
			size_t length = strlen(entity);
			memcpy(out + n, entity, length);
			n += length;
		} else {
			out[n++] = *text;
		}
	}
	out[n] = '\0';
}

int library_genre_count(void) {
	return (int)(sizeof(g_genres) / sizeof(g_genres[0]));
}

const char* library_genre_name(int index) {
	return g_genres[index];
}

void library_genre_color(int index, float rgb[3]) {
	unsigned c = g_genre_colors[index];
	rgb[0] = ((c >> 16) & 255) / 255.0f;
	rgb[1] = ((c >> 8) & 255) / 255.0f;
	rgb[2] = (c & 255) / 255.0f;
}
