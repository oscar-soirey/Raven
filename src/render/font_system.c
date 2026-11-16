/**
 * Raven - render/font_system.c
 * The fonts of the system, for the characters that the fonts of the
 * application do not have: other alphabets (Arabic, Hebrew, Cyrillic,
 * Greek, Devanagari, Thai...), Chinese, Japanese, Korean, symbols and emoji.
 *
 * For a character, the candidates are the files made for its script, then
 * the broad fonts. font.c opens them on first need, keeps those that load,
 * and remembers which font has each character.
 *
 *   Windows  %WINDIR%\Fonts: Segoe UI, Segoe UI Emoji (color, COLR),
 *            Segoe UI Symbol, Microsoft YaHei, Yu Gothic, Malgun Gothic,
 *            Nirmala UI, Ebrima, Tahoma, Arial...
 *   macOS    /System/Library/Fonts (and Supplemental): Apple Color Emoji
 *            (color, sbix), Geeza Pro, Arial Hebrew, PingFang, Hiragino,
 *            Apple SD Gothic Neo, Kohinoor, Thonburi, Apple Symbols...
 *   iOS      the font CoreText would draw the character with: the files
 *            of the system move between versions of iOS, so they are not
 *            named here. Apple Color Emoji keeps its pictures in a format
 *            of Apple's own there (emjc): CoreText draws those.
 *   Linux    every .ttf / .otf / .ttc in the font folders, chosen by name
 *            (Noto Sans <Script>, Noto Color Emoji (color, CBDT or COLR),
 *            DejaVu Sans, FreeSans...)
 *   Android  the same, in /system/fonts (Roboto, Noto Sans <Script>, Noto
 *            Sans CJK, Noto Color Emoji)
 */

#include "font.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if defined(__APPLE__) && TARGET_OS_IPHONE
#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>
#define RVN_SYSTEM_CORETEXT 1
#endif

typedef enum rvn_script_t {
	RVN_SCRIPT_LATIN = 0, RVN_SCRIPT_GREEK, RVN_SCRIPT_CYRILLIC, RVN_SCRIPT_ARMENIAN, RVN_SCRIPT_HEBREW, RVN_SCRIPT_ARABIC,
	RVN_SCRIPT_SYRIAC, RVN_SCRIPT_THAANA, RVN_SCRIPT_INDIC, RVN_SCRIPT_THAI, RVN_SCRIPT_LAO, RVN_SCRIPT_GEORGIAN,
	RVN_SCRIPT_ETHIOPIC, RVN_SCRIPT_KHMER, RVN_SCRIPT_MYANMAR, RVN_SCRIPT_SINHALA, RVN_SCRIPT_TIBETAN,
	RVN_SCRIPT_HANGUL, RVN_SCRIPT_KANA, RVN_SCRIPT_HAN, RVN_SCRIPT_SYMBOL, RVN_SCRIPT_EMOJI, RVN_SCRIPT_COUNT
} rvn_script_t;

static rvn_script_t rvn_script_of(uint32_t c) {
	if (c < 0x0370) return RVN_SCRIPT_LATIN;
	if (c < 0x0400) return RVN_SCRIPT_GREEK;
	if (c < 0x0530) return RVN_SCRIPT_CYRILLIC;
	if (c < 0x0590) return RVN_SCRIPT_ARMENIAN;
	if (c < 0x0600) return RVN_SCRIPT_HEBREW;
	if (c < 0x0700) return RVN_SCRIPT_ARABIC;
	if (c < 0x0780) return RVN_SCRIPT_SYRIAC;
	if (c < 0x07C0) return RVN_SCRIPT_THAANA;
	if (c >= 0x0860 && c < 0x0900) return RVN_SCRIPT_ARABIC;
	if (c >= 0x0900 && c < 0x0D80) return RVN_SCRIPT_INDIC;
	if (c >= 0x0D80 && c < 0x0E00) return RVN_SCRIPT_SINHALA;
	if (c >= 0x0E00 && c < 0x0E80) return RVN_SCRIPT_THAI;
	if (c >= 0x0E80 && c < 0x0F00) return RVN_SCRIPT_LAO;
	if (c >= 0x0F00 && c < 0x1000) return RVN_SCRIPT_TIBETAN;
	if (c >= 0x1000 && c < 0x10A0) return RVN_SCRIPT_MYANMAR;
	if (c >= 0x10A0 && c < 0x1100) return RVN_SCRIPT_GEORGIAN;
	if (c >= 0x1100 && c < 0x1200) return RVN_SCRIPT_HANGUL;
	if (c >= 0x1200 && c < 0x13A0) return RVN_SCRIPT_ETHIOPIC;
	if (c >= 0x1780 && c < 0x1800) return RVN_SCRIPT_KHMER;
	if (c >= 0x1E00 && c < 0x2000) return c < 0x1F00 ? RVN_SCRIPT_LATIN : RVN_SCRIPT_GREEK;
	if (c >= 0x2000 && c < 0x2E80) return RVN_SCRIPT_SYMBOL;
	if (c >= 0x3040 && c < 0x3100) return RVN_SCRIPT_KANA;
	if (c >= 0x31F0 && c < 0x3200) return RVN_SCRIPT_KANA;
	if (c >= 0x3130 && c < 0x3190) return RVN_SCRIPT_HANGUL;
	if (c >= 0xAC00 && c < 0xD7B0) return RVN_SCRIPT_HANGUL;
	if ((c >= 0x2E80 && c < 0xA000) || (c >= 0xF900 && c < 0xFB00) || (c >= 0x20000 && c < 0x32000)) return RVN_SCRIPT_HAN;
	if (c >= 0xFB1D && c < 0xFB50) return RVN_SCRIPT_HEBREW;
	if (c >= 0xFB50 && c < 0xFE00) return RVN_SCRIPT_ARABIC;
	if (c >= 0xFE70 && c < 0xFF00) return RVN_SCRIPT_ARABIC;
	if (c >= 0xFF00 && c < 0xFFF0) return RVN_SCRIPT_HAN;
	if ((c >= 0x1F000 && c < 0x1FB00) || c == 0x2764 || (c >= 0x2600 && c < 0x27C0)) return RVN_SCRIPT_EMOJI;
	if (c >= 0x1D400 && c < 0x1D800) return RVN_SCRIPT_SYMBOL;
	return RVN_SCRIPT_LATIN;
}

static struct {
	int ready;
	/* every font found, once */
	rvn_system_file_t** files;
	int file_count;
	/* the candidates, per script, then the broad ones */
	rvn_system_file_t** lists[RVN_SCRIPT_COUNT + 1];
	int counts[RVN_SCRIPT_COUNT + 1];
} g_system;

/* The font `face` of the file at `path`, made once: NULL when the file cannot be read. */
static rvn_system_file_t* rvn_system_file(const char* path, int face) {
	rvn_system_file_t* file;
	FILE* f;
	int i;
	for (i = 0; i < g_system.file_count; i++) {
		if (g_system.files[i]->face == face && strcmp(g_system.files[i]->path, path) == 0) return g_system.files[i];
	}
	f = fopen(path, "rb");
	if (!f) return NULL;
	fclose(f);
	file = (rvn_system_file_t*)rvn_calloc(sizeof(rvn_system_file_t));
	file->path = rvn_strdup(path);
	file->face = face;
	g_system.files = (rvn_system_file_t**)rvn_realloc(g_system.files, (size_t)(g_system.file_count + 1) * sizeof(rvn_system_file_t*));
	g_system.files[g_system.file_count++] = file;
	return file;
}

static void rvn_system_add_file(int list, rvn_system_file_t* file) {
	int i;
	if (!file) return;
	for (i = 0; i < g_system.counts[list]; i++)
		if (g_system.lists[list][i] == file) return;
	g_system.lists[list] = (rvn_system_file_t**)rvn_realloc(g_system.lists[list], (size_t)(g_system.counts[list] + 1) * sizeof(rvn_system_file_t*));
	g_system.lists[list][g_system.counts[list]++] = file;
}

#if !defined(RVN_SYSTEM_CORETEXT)
static void rvn_system_add(int list, const char* path) {
	rvn_system_add_file(list, rvn_system_file(path, 0));
}
#endif

#if defined(_WIN32) || (defined(__APPLE__) && !defined(RVN_SYSTEM_CORETEXT))

typedef struct rvn_system_font_t {
	int script; /* RVN_SCRIPT_COUNT: broad */
	const char* file;
} rvn_system_font_t;

static void rvn_system_add_table(const char* folder, const rvn_system_font_t* table, size_t count) {
	size_t i;
	char path[1024];
	for (i = 0; i < count; i++) {
		snprintf(path, sizeof(path), "%s%s", folder, table[i].file);
		rvn_system_add(table[i].script, path);
	}
}

#endif

#if defined(_WIN32)

static const rvn_system_font_t g_windows_fonts[] = {
	{ RVN_SCRIPT_EMOJI, "seguiemj.ttf" }, { RVN_SCRIPT_EMOJI, "seguisym.ttf" },
	{ RVN_SCRIPT_SYMBOL, "seguisym.ttf" }, { RVN_SCRIPT_SYMBOL, "segoeui.ttf" }, { RVN_SCRIPT_SYMBOL, "cambria.ttc" },
	{ RVN_SCRIPT_ARABIC, "segoeui.ttf" }, { RVN_SCRIPT_ARABIC, "tahoma.ttf" }, { RVN_SCRIPT_ARABIC, "arial.ttf" },
	{ RVN_SCRIPT_HEBREW, "segoeui.ttf" }, { RVN_SCRIPT_HEBREW, "arial.ttf" },
	{ RVN_SCRIPT_GREEK, "segoeui.ttf" }, { RVN_SCRIPT_CYRILLIC, "segoeui.ttf" }, { RVN_SCRIPT_LATIN, "segoeui.ttf" },
	{ RVN_SCRIPT_ARMENIAN, "segoeui.ttf" }, { RVN_SCRIPT_ARMENIAN, "sylfaen.ttf" }, { RVN_SCRIPT_GEORGIAN, "segoeui.ttf" },
	{ RVN_SCRIPT_THAI, "leelawui.ttf" }, { RVN_SCRIPT_THAI, "tahoma.ttf" }, { RVN_SCRIPT_LAO, "leelawui.ttf" },
	{ RVN_SCRIPT_KHMER, "leelawui.ttf" }, { RVN_SCRIPT_MYANMAR, "mmrtext.ttf" },
	{ RVN_SCRIPT_INDIC, "Nirmala.ttc" }, { RVN_SCRIPT_INDIC, "Nirmala.ttf" }, { RVN_SCRIPT_SINHALA, "Nirmala.ttc" }, { RVN_SCRIPT_SINHALA, "Nirmala.ttf" },
	{ RVN_SCRIPT_ETHIOPIC, "ebrima.ttf" }, { RVN_SCRIPT_THAANA, "mvboli.ttf" }, { RVN_SCRIPT_SYRIAC, "estre.ttf" }, { RVN_SCRIPT_TIBETAN, "himalaya.ttf" },
	{ RVN_SCRIPT_HANGUL, "malgun.ttf" },
	{ RVN_SCRIPT_KANA, "YuGothM.ttc" }, { RVN_SCRIPT_KANA, "meiryo.ttc" }, { RVN_SCRIPT_KANA, "msgothic.ttc" },
	{ RVN_SCRIPT_HAN, "msyh.ttc" }, { RVN_SCRIPT_HAN, "YuGothM.ttc" }, { RVN_SCRIPT_HAN, "simsun.ttc" }, { RVN_SCRIPT_HAN, "msjh.ttc" },
	{ RVN_SCRIPT_COUNT, "segoeui.ttf" }, { RVN_SCRIPT_COUNT, "arial.ttf" }, { RVN_SCRIPT_COUNT, "seguisym.ttf" }, { RVN_SCRIPT_COUNT, "msyh.ttc" }
};

static void rvn_system_scan(void) {
	char windows[MAX_PATH], folder[MAX_PATH + 16];
	UINT n = GetWindowsDirectoryA(windows, MAX_PATH);
	if (n == 0 || n >= MAX_PATH) strcpy(windows, "C:\\Windows");
	snprintf(folder, sizeof(folder), "%s\\Fonts\\", windows);
	rvn_system_add_table(folder, g_windows_fonts, RVN_COUNT_OF(g_windows_fonts));
}

#elif defined(RVN_SYSTEM_CORETEXT)

/* The fonts CoreText chose so far, by PostScript name. */
static struct {
	char* name;
	rvn_system_file_t* file;
} * g_coretext;
static int g_coretext_count;
/* The system font, and the fonts of its fallback list (CoreText is asked for each character). */
static CTFontRef g_coretext_system;
static CFArrayRef g_coretext_cascade;
static CTFontRef* g_coretext_fallbacks;
static CFIndex g_coretext_fallback_count;

static void rvn_system_scan(void) {
	CFIndex i;
	g_coretext_system = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, 17.0, NULL);
	g_coretext_cascade = g_coretext_system ? CTFontCopyDefaultCascadeListForLanguages(g_coretext_system, NULL) : NULL;
	if (!g_coretext_cascade) return;
	g_coretext_fallbacks = (CTFontRef*)rvn_calloc((size_t)CFArrayGetCount(g_coretext_cascade) * sizeof(CTFontRef));
	for (i = 0; i < CFArrayGetCount(g_coretext_cascade); i++) {
		CTFontRef font = CTFontCreateWithFontDescriptor((CTFontDescriptorRef)CFArrayGetValueAtIndex(g_coretext_cascade, i), 17.0, NULL);
		if (font) g_coretext_fallbacks[g_coretext_fallback_count++] = font;
	}
}

/* The file and the font in it of a font of CoreText: NULL when they cannot be read. */
static rvn_system_file_t* rvn_coretext_file(CTFontRef font) {
	CFStringRef name = CTFontCopyPostScriptName(font);
	CFURLRef url = (CFURLRef)CTFontCopyAttribute(font, kCTFontURLAttribute);
	char key[256], path[1024];
	rvn_system_file_t* file = NULL;
	int i, face = 0;
	if (!name || !CFStringGetCString(name, key, sizeof(key), kCFStringEncodingUTF8)) key[0] = '\0';
	if (name) CFRelease(name);
	for (i = 0; i < g_coretext_count; i++) {
		if (strcmp(g_coretext[i].name, key) == 0) {
			if (url) CFRelease(url);
			return g_coretext[i].file;
		}
	}
	if (url && CFURLGetFileSystemRepresentation(url, true, (UInt8*)path, sizeof(path))) {
		size_t size = 0;
		uint8_t* data = rvn_map_file(path, &size);
		/* in a collection (.ttc), the font of that name (else the first) */
		if (data) face = rvn_maxi(0, rvn_ttf_find_face(data, size, key));
		rvn_unmap_file(data, size);
		file = data ? rvn_system_file(path, face) : NULL;
	}
	if (url) CFRelease(url);
	/* a font of color glyphs: kept, for the pictures Raven cannot decode (rvn_font_system_draw) */
	if (file && !file->native && (CTFontGetSymbolicTraits(font) & kCTFontTraitColorGlyphs)) file->native = (void*)CFRetain(font);
	g_coretext = rvn_realloc(g_coretext, (size_t)(g_coretext_count + 1) * sizeof(*g_coretext));
	g_coretext[g_coretext_count].name = rvn_strdup(key);
	g_coretext[g_coretext_count].file = file;
	g_coretext_count++;
	return file;
}

/* The font CoreText draws the character with, added to the candidates of its script. */
static void rvn_system_ask(uint32_t codepoint, int list) {
	UniChar units[2];
	CFIndex count = 1;
	CFStringRef text;
	CTFontRef font;
	if (codepoint >= 0x10000u) {
		units[0] = (UniChar)(0xD800u + ((codepoint - 0x10000u) >> 10));
		units[1] = (UniChar)(0xDC00u + ((codepoint - 0x10000u) & 0x3FFu));
		count = 2;
	} else {
		units[0] = (UniChar)codepoint;
	}
	text = CFStringCreateWithCharacters(NULL, units, count);
	if (text && g_coretext_system) {
		CFIndex i, found = 0;
		font = CTFontCreateForString(g_coretext_system, text, CFRangeMake(0, count));
		if (font) {
			rvn_system_add_file(list, rvn_coretext_file(font));
			CFRelease(font);
		}
		/* then the other fonts of the system's fallback list that have it: CoreText's choice may be in
		 * a format Raven does not read (iOS 26: PingFang's outlines are in Apple's own hvgl table) */
		for (i = 0; i < g_coretext_fallback_count && found < 4; i++) {
			CGGlyph glyphs[2] = { 0, 0 };
			/* (some of the list are fonts of fonts, with no file of their own) */
			if (CTFontGetGlyphsForCharacters(g_coretext_fallbacks[i], units, glyphs, count) && glyphs[0]) {
				rvn_system_file_t* file = rvn_coretext_file(g_coretext_fallbacks[i]);
				if (file) {
					rvn_system_add_file(list, file);
					found++;
				}
			}
		}
	}
	if (text) CFRelease(text);
}

uint8_t* rvn_font_system_draw(const rvn_system_file_t* file, int glyph, float size_px, int* x0, int* y0, int* width, int* height) {
	CTFontRef font;
	CGGlyph id = (CGGlyph)glyph;
	CGRect box;
	CGColorSpaceRef space;
	CGContextRef context;
	CGPoint position;
	uint8_t* pixels;
	int w, h, left, top, i;
	if (!file || !file->native || glyph <= 0 || glyph > 0xFFFF || size_px < 1.0f) return NULL;
	font = CTFontCreateCopyWithAttributes((CTFontRef)file->native, (CGFloat)size_px, NULL, NULL);
	if (!font) return NULL;
	box = CTFontGetBoundingRectsForGlyphs(font, kCTFontOrientationHorizontal, &id, NULL, 1);
	/* the box of the glyph in pixels (y up), with a pixel to spare on each side */
	left = (int)floor(box.origin.x) - 1;
	top = (int)ceil(box.origin.y + box.size.height) + 1;
	w = (int)ceil(box.origin.x + box.size.width) + 1 - left;
	h = top - ((int)floor(box.origin.y) - 1);
	if (w <= 0 || h <= 0 || w > 1024 || h > 1024) {
		CFRelease(font);
		return NULL;
	}
	pixels = (uint8_t*)rvn_calloc((size_t)w * (size_t)h * 4);
	space = CGColorSpaceCreateDeviceRGB();
	context = CGBitmapContextCreate(pixels, (size_t)w, (size_t)h, 8, (size_t)w * 4, space,
		(CGBitmapInfo)kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
	CGColorSpaceRelease(space);
	if (!context) {
		rvn_free(pixels);
		CFRelease(font);
		return NULL;
	}
	position = CGPointMake((CGFloat)-left, (CGFloat)(h - top));
	CTFontDrawGlyphs(font, &id, &position, 1, context);
	CGContextRelease(context);
	CFRelease(font);
	/* CoreGraphics gives premultiplied alpha, Raven's textures are straight */
	for (i = 0; i < w * h; i++) {
		uint8_t* p = pixels + (size_t)i * 4;
		if (p[3] && p[3] < 255) {
			p[0] = (uint8_t)rvn_mini(255, (p[0] * 255 + p[3] / 2) / p[3]);
			p[1] = (uint8_t)rvn_mini(255, (p[1] * 255 + p[3] / 2) / p[3]);
			p[2] = (uint8_t)rvn_mini(255, (p[2] * 255 + p[3] / 2) / p[3]);
		}
	}
	*x0 = left;
	*y0 = -top;
	*width = w;
	*height = h;
	return pixels;
}

#elif defined(__APPLE__)

static const rvn_system_font_t g_mac_fonts[] = {
	{ RVN_SCRIPT_ARABIC, "GeezaPro.ttc" }, { RVN_SCRIPT_ARABIC, "Supplemental/GeezaPro.ttc" }, { RVN_SCRIPT_ARABIC, "SFArabic.ttf" },
	{ RVN_SCRIPT_HEBREW, "SFHebrew.ttf" }, { RVN_SCRIPT_HEBREW, "Supplemental/Arial Hebrew.ttc" }, { RVN_SCRIPT_HEBREW, "ArialHB.ttc" },
	{ RVN_SCRIPT_GREEK, "Helvetica.ttc" }, { RVN_SCRIPT_CYRILLIC, "Helvetica.ttc" }, { RVN_SCRIPT_LATIN, "Helvetica.ttc" },
	{ RVN_SCRIPT_ARMENIAN, "SFArmenian.ttf" }, { RVN_SCRIPT_GEORGIAN, "SFGeorgian.ttf" },
	{ RVN_SCRIPT_THAI, "Thonburi.ttc" }, { RVN_SCRIPT_THAI, "Supplemental/Thonburi.ttc" }, { RVN_SCRIPT_LAO, "Supplemental/Lao Sangam MN.ttf" },
	{ RVN_SCRIPT_INDIC, "Kohinoor.ttc" }, { RVN_SCRIPT_INDIC, "Supplemental/DevanagariMT.ttc" }, { RVN_SCRIPT_INDIC, "Supplemental/Tamil Sangam MN.ttc" },
	{ RVN_SCRIPT_KHMER, "Supplemental/Khmer Sangam MN.ttf" }, { RVN_SCRIPT_MYANMAR, "Supplemental/Myanmar Sangam MN.ttc" },
	{ RVN_SCRIPT_SINHALA, "Supplemental/Sinhala Sangam MN.ttc" }, { RVN_SCRIPT_ETHIOPIC, "Supplemental/Kefa.ttc" },
	{ RVN_SCRIPT_TIBETAN, "Supplemental/Kailasa.ttc" }, { RVN_SCRIPT_THAANA, "Supplemental/Arial Unicode.ttf" },
	{ RVN_SCRIPT_HANGUL, "AppleSDGothicNeo.ttc" },
	{ RVN_SCRIPT_KANA, "\xE3\x83\x92\xE3\x83\xA9\xE3\x82\xAE\xE3\x83\x8E\xE8\xA7\x92\xE3\x82\xB4\xE3\x82\xB7\xE3\x83\x83\xE3\x82\xAF W3.ttc" }, { RVN_SCRIPT_KANA, "Hiragino Sans GB.ttc" },
	{ RVN_SCRIPT_HAN, "PingFang.ttc" }, { RVN_SCRIPT_HAN, "Hiragino Sans GB.ttc" }, { RVN_SCRIPT_HAN, "STHeiti Light.ttc" },
	{ RVN_SCRIPT_SYMBOL, "Apple Symbols.ttf" }, { RVN_SCRIPT_SYMBOL, "Supplemental/Arial Unicode.ttf" },
	{ RVN_SCRIPT_EMOJI, "Apple Color Emoji.ttc" }, { RVN_SCRIPT_EMOJI, "Apple Symbols.ttf" }, { RVN_SCRIPT_EMOJI, "Supplemental/Arial Unicode.ttf" },
	{ RVN_SCRIPT_COUNT, "Supplemental/Arial Unicode.ttf" }, { RVN_SCRIPT_COUNT, "Helvetica.ttc" }, { RVN_SCRIPT_COUNT, "Apple Symbols.ttf" }
};

static void rvn_system_scan(void) {
	rvn_system_add_table("/System/Library/Fonts/", g_mac_fonts, RVN_COUNT_OF(g_mac_fonts));
	rvn_system_add_table("/Library/Fonts/", g_mac_fonts, RVN_COUNT_OF(g_mac_fonts));
}

#else

/* Names of files that cover a script, the best first (they are searched for in the file names). */
static const char* const* rvn_linux_patterns(int script) {
	static const char* const latin[] = { "NotoSans-Regular", "Roboto-Regular", "DejaVuSans.", "LiberationSans-Regular", "FreeSans.", NULL };
	static const char* const arabic[] = { "NotoSansArabic-Regular", "NotoNaskhArabic-Regular", "NotoSansArabicUI-Regular", "DejaVuSans.", "FreeSerif.", NULL };
	static const char* const hebrew[] = { "NotoSansHebrew-Regular", "DejaVuSans.", "FreeSans.", NULL };
	static const char* const armenian[] = { "NotoSansArmenian-Regular", "DejaVuSans.", "FreeSans.", NULL };
	static const char* const georgian[] = { "NotoSansGeorgian-Regular", "DejaVuSans.", "FreeSans.", NULL };
	static const char* const indic[] = { "NotoSansDevanagari-Regular", "NotoSansBengali-Regular", "NotoSansTamil-Regular", "NotoSansTelugu-Regular",
		"NotoSansGujarati-Regular", "NotoSansKannada-Regular", "NotoSansMalayalam-Regular", "NotoSansGurmukhi-Regular", "NotoSansOriya-Regular",
		"Lohit", "FreeSans.", NULL };
	static const char* const thai[] = { "NotoSansThai-Regular", "NotoSansThaiUI-Regular", "Garuda", "FreeSerif.", NULL };
	static const char* const lao[] = { "NotoSansLao-Regular", NULL };
	static const char* const khmer[] = { "NotoSansKhmer-Regular", NULL };
	static const char* const myanmar[] = { "NotoSansMyanmar-Regular", NULL };
	static const char* const sinhala[] = { "NotoSansSinhala-Regular", NULL };
	static const char* const tibetan[] = { "NotoSerifTibetan-Regular", "NotoSansTibetan", NULL };
	static const char* const ethiopic[] = { "NotoSansEthiopic-Regular", "Abyssinica", "FreeSerif.", NULL };
	static const char* const syriac[] = { "NotoSansSyriac", NULL };
	static const char* const thaana[] = { "NotoSansThaana-Regular", "FreeSerif.", NULL };
	static const char* const cjk[] = { "NotoSansCJK-Regular", "NotoSansCJKsc-Regular", "NotoSansCJKjp-Regular", "NotoSansSC-Regular",
		"NotoSansJP-Regular", "NotoSansKR-Regular", "wqy-microhei", "wqy-zenhei", "DroidSansFallback", "ukai", "uming", NULL };
	static const char* const symbol[] = { "NotoSansSymbols-Regular", "NotoSansSymbols2-Regular", "NotoSansMath-Regular", "DejaVuSans.", "FreeSerif.", "Symbola", NULL };
	/* (Android: the Legacy one is in CBDT, the other may be COLRv1, whose gradients Raven does not draw) */
	static const char* const emoji[] = { "Noto-COLRv1", "NotoColorEmoji-COLR", "NotoColorEmojiLegacy.", "NotoColorEmoji.", "Twemoji", "NotoEmoji-Regular", "NotoEmoji", "Symbola", "NotoSansSymbols2-Regular", "DejaVuSans.", NULL };
	static const char* const broad[] = { "DejaVuSans.", "NotoSans-Regular", "Roboto-Regular", "FreeSans.", "FreeSerif.", "unifont", NULL };
	switch (script) {
		case RVN_SCRIPT_ARABIC: return arabic;
		case RVN_SCRIPT_HEBREW: return hebrew;
		case RVN_SCRIPT_ARMENIAN: return armenian;
		case RVN_SCRIPT_GEORGIAN: return georgian;
		case RVN_SCRIPT_INDIC: return indic;
		case RVN_SCRIPT_THAI: return thai;
		case RVN_SCRIPT_LAO: return lao;
		case RVN_SCRIPT_KHMER: return khmer;
		case RVN_SCRIPT_MYANMAR: return myanmar;
		case RVN_SCRIPT_SINHALA: return sinhala;
		case RVN_SCRIPT_TIBETAN: return tibetan;
		case RVN_SCRIPT_ETHIOPIC: return ethiopic;
		case RVN_SCRIPT_SYRIAC: return syriac;
		case RVN_SCRIPT_THAANA: return thaana;
		case RVN_SCRIPT_HANGUL: case RVN_SCRIPT_KANA: case RVN_SCRIPT_HAN: return cjk;
		case RVN_SCRIPT_SYMBOL: return symbol;
		case RVN_SCRIPT_EMOJI: return emoji;
		case RVN_SCRIPT_COUNT: return broad;
		default: return latin;
	}
}

static char** g_linux_files;
static int g_linux_file_count;

static void rvn_linux_walk(const char* folder, int depth) {
	DIR* dir;
	struct dirent* entry;
	if (depth > 5 || g_linux_file_count > 20000) return;
	dir = opendir(folder);
	if (!dir) return;
	while ((entry = readdir(dir)) != NULL) {
		char path[1024];
		struct stat info;
		size_t n = strlen(entry->d_name);
		if (entry->d_name[0] == '.') continue;
		snprintf(path, sizeof(path), "%s/%s", folder, entry->d_name);
		if (stat(path, &info) != 0) continue;
		if (S_ISDIR(info.st_mode)) {
			rvn_linux_walk(path, depth + 1);
		} else if (n > 4 && (rvn_stricmp(entry->d_name + n - 4, ".ttf") == 0 || rvn_stricmp(entry->d_name + n - 4, ".otf") == 0 ||
			rvn_stricmp(entry->d_name + n - 4, ".ttc") == 0)) {
			g_linux_files = (char**)rvn_realloc(g_linux_files, (size_t)(g_linux_file_count + 1) * sizeof(char*));
			g_linux_files[g_linux_file_count++] = rvn_strdup(path);
		}
	}
	closedir(dir);
}

static void rvn_system_scan(void) {
	const char* home = getenv("HOME");
	const char* data = getenv("XDG_DATA_HOME");
	char path[1024];
	int script, i, p;
	rvn_linux_walk("/usr/share/fonts", 0);
	rvn_linux_walk("/usr/local/share/fonts", 0);
#if defined(__ANDROID__)
	rvn_linux_walk("/system/fonts", 0);
	rvn_linux_walk("/product/fonts", 0);
#endif
	if (data && data[0]) {
		snprintf(path, sizeof(path), "%s/fonts", data);
		rvn_linux_walk(path, 0);
	}
	if (home) {
		snprintf(path, sizeof(path), "%s/.local/share/fonts", home);
		rvn_linux_walk(path, 0);
		snprintf(path, sizeof(path), "%s/.fonts", home);
		rvn_linux_walk(path, 0);
	}
	for (script = 0; script <= RVN_SCRIPT_COUNT; script++) {
		const char* const* patterns = rvn_linux_patterns(script);
		for (p = 0; patterns[p]; p++) {
			for (i = 0; i < g_linux_file_count; i++) {
				const char* name = strrchr(g_linux_files[i], '/');
				name = name ? name + 1 : g_linux_files[i];
				if (strncmp(name, patterns[p], strlen(patterns[p])) == 0) rvn_system_add(script, g_linux_files[i]);
			}
		}
	}
	/* the other Noto Sans files, for the scripts named nowhere above */
	for (i = 0; i < g_linux_file_count; i++) {
		const char* name = strrchr(g_linux_files[i], '/');
		name = name ? name + 1 : g_linux_files[i];
		if (strncmp(name, "NotoSans", 8) == 0 && strstr(name, "-Regular.")) rvn_system_add(RVN_SCRIPT_COUNT, g_linux_files[i]);
	}
	for (i = 0; i < g_linux_file_count; i++) rvn_free(g_linux_files[i]);
	rvn_free(g_linux_files);
	g_linux_files = NULL;
	g_linux_file_count = 0;
}

#endif

#if !defined(RVN_SYSTEM_CORETEXT)

uint8_t* rvn_font_system_draw(const rvn_system_file_t* file, int glyph, float size_px, int* x0, int* y0, int* width, int* height) {
	RVN_UNUSED(file);
	RVN_UNUSED(glyph);
	RVN_UNUSED(size_px);
	RVN_UNUSED(x0);
	RVN_UNUSED(y0);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	return NULL;
}

#endif

int rvn_font_system_candidates(uint32_t codepoint, const rvn_system_file_t** files, int max) {
	int script, count = 0, i, k;
	if (!g_system.ready) {
		g_system.ready = 1;
		rvn_system_scan();
	}
	script = (int)rvn_script_of(codepoint);
#if defined(RVN_SYSTEM_CORETEXT)
	rvn_system_ask(codepoint, script);
#endif
	for (k = 0; k < 2; k++) {
		int list = k == 0 ? script : RVN_SCRIPT_COUNT;
		for (i = 0; i < g_system.counts[list] && count < max; i++) {
			int j, seen = 0;
			for (j = 0; j < count; j++) if (files[j] == g_system.lists[list][i]) seen = 1;
			if (!seen) files[count++] = g_system.lists[list][i];
		}
	}
	return count;
}

void rvn_font_system_shutdown(void) {
	int list, i;
	for (list = 0; list <= RVN_SCRIPT_COUNT; list++) rvn_free(g_system.lists[list]);
	for (i = 0; i < g_system.file_count; i++) {
#if defined(RVN_SYSTEM_CORETEXT)
		if (g_system.files[i]->native) CFRelease((CTFontRef)g_system.files[i]->native);
#endif
		rvn_free(g_system.files[i]->path);
		rvn_free(g_system.files[i]);
	}
	rvn_free(g_system.files);
#if defined(RVN_SYSTEM_CORETEXT)
	for (i = 0; i < g_coretext_count; i++) rvn_free(g_coretext[i].name);
	rvn_free(g_coretext);
	g_coretext = NULL;
	g_coretext_count = 0;
	for (i = 0; i < (int)g_coretext_fallback_count; i++) CFRelease(g_coretext_fallbacks[i]);
	rvn_free(g_coretext_fallbacks);
	g_coretext_fallbacks = NULL;
	g_coretext_fallback_count = 0;
	if (g_coretext_cascade) CFRelease(g_coretext_cascade);
	if (g_coretext_system) CFRelease(g_coretext_system);
	g_coretext_cascade = NULL;
	g_coretext_system = NULL;
#endif
	memset(&g_system, 0, sizeof(g_system));
}
