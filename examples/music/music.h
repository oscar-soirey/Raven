/**
 * Raven example - music
 * What the parts of the player share.
 *
 *   main.c      the window, the pages, the player (simulated: no sound is played)
 *   library.c   the albums, the tracks and the playlists (made up), the covers
 *   tags.c      the tags the interface uses that the application makes:
 *               <Icon>, <Cover>, <Visualizer> and <Heading>
 *
 * The interface itself is in the XML files of ui/, the themes in themes/.
 */

#ifndef RAVEN_MUSIC_H
#define RAVEN_MUSIC_H

#include <Raven.h>

#include <stddef.h>

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

/* ---- library.c ---- */

#define MUSIC_ALBUMS 24
#define MUSIC_MAX_TRACKS 320
#define MUSIC_PLAYLISTS 6

typedef struct album_t {
	char title[64];
	char artist[64];
	char genre[24];
	int year;
	int first_track, track_count;
	/* the color the album is known by: the accent while it plays */
	float color[3], color2[3];
	Rid texture;
} album_t;

typedef struct track_t {
	char title[64];
	int album;
	int seconds;
	int plays;
	int liked;
} track_t;

typedef struct playlist_t {
	char title[48];
	char description[96];
	int tracks[40];
	int count;
	/* the album whose cover it borrows */
	int cover;
} playlist_t;

extern album_t g_albums[MUSIC_ALBUMS];
extern track_t g_tracks[MUSIC_MAX_TRACKS];
extern int g_track_count;
extern playlist_t g_playlists[MUSIC_PLAYLISTS];

/** Makes up the library and loads the covers (covers/: any .jpg or .png; the albums without one get a drawn one). */
void library_init(void);
void library_shutdown(void);
/** "3:07" */
void library_format_time(int seconds, char* out, size_t size);
/** Writes text into XML, escaped. */
void library_escape(const char* text, char* out, size_t size);
/** The genres shown by Search. */
int library_genre_count(void);
const char* library_genre_name(int index);
void library_genre_color(int index, float rgb[3]);

/* ---- tags.c ---- */

/** Registers <Icon>, <Cover>, <Visualizer> and <Heading>: before the interface is loaded. */
void tags_init(void);
void tags_shutdown(void);
/** The covers that show "the album playing" follow it. */
void tags_set_current_album(int album);
/** The visualizers move while it plays. */
void tags_set_playing(int playing);
/** A <Cover> shows another album (a page reused for another album). */
void tags_cover_set_album(Rid cover, int album);
/** An <Icon> shows another glyph ("play", "pause"...). */
void tags_icon_set(Rid icon, const char* glyph);

#endif
