/**
 * Raven - core/utf8.c
 * UTF-8 decoding, encoding and caret movement, and the comparison of a
 * search (without regard to case or accents).
 */

#include "base.h"

uint32_t rvn_utf8_decode(const char** text, const char* end) {
	const unsigned char* s = (const unsigned char*)*text;
	const unsigned char* e = (const unsigned char*)end;
	uint32_t c, cp;
	int extra, i;

	if (s >= e) return 0;
	c = *s++;
	if (c < 0x80u) {
		*text = (const char*)s;
		return c;
	}
	if ((c & 0xE0u) == 0xC0u) { cp = c & 0x1Fu; extra = 1; }
	else if ((c & 0xF0u) == 0xE0u) { cp = c & 0x0Fu; extra = 2; }
	else if ((c & 0xF8u) == 0xF0u) { cp = c & 0x07u; extra = 3; }
	else {
		*text = (const char*)s;
		return 0xFFFDu;
	}
	for (i = 0; i < extra; i++) {
		if (s >= e || (*s & 0xC0u) != 0x80u) {
			*text = (const char*)s;
			return 0xFFFDu;
		}
		cp = (cp << 6) | (*s++ & 0x3Fu);
	}
	*text = (const char*)s;
	/* reject overlong forms, surrogates and out-of-range values */
	if ((extra == 1 && cp < 0x80u) || (extra == 2 && cp < 0x800u) || (extra == 3 && cp < 0x10000u) ||
		(cp >= 0xD800u && cp <= 0xDFFFu) || cp > 0x10FFFFu) {
		return 0xFFFDu;
	}
	return cp;
}

int rvn_utf8_encode(uint32_t cp, char out[4]) {
	if (cp < 0x80u) {
		out[0] = (char)cp;
		return 1;
	}
	if (cp < 0x800u) {
		out[0] = (char)(0xC0u | (cp >> 6));
		out[1] = (char)(0x80u | (cp & 0x3Fu));
		return 2;
	}
	if (cp < 0x10000u) {
		out[0] = (char)(0xE0u | (cp >> 12));
		out[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
		out[2] = (char)(0x80u | (cp & 0x3Fu));
		return 3;
	}
	out[0] = (char)(0xF0u | ((cp >> 18) & 0x07u));
	out[1] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
	out[2] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
	out[3] = (char)(0x80u | (cp & 0x3Fu));
	return 4;
}

int rvn_utf8_next(const char* text, int length, int offset) {
	if (offset >= length) return length;
	offset++;
	while (offset < length && ((unsigned char)text[offset] & 0xC0u) == 0x80u) offset++;
	return offset;
}

int rvn_utf8_prev(const char* text, int offset) {
	if (offset <= 0) return 0;
	offset--;
	while (offset > 0 && ((unsigned char)text[offset] & 0xC0u) == 0x80u) offset--;
	return offset;
}

int rvn_utf8_count(const char* text, int length) {
	int i, count = 0;
	for (i = 0; i < length; i++) {
		if (((unsigned char)text[i] & 0xC0u) != 0x80u) count++;
	}
	return count;
}

/* ---------------------------------------------------------------------------
 *  Folding, for searches
 * ------------------------------------------------------------------------- */

/* U+00C0..U+00FF: the letter without its accent; '.' keeps the code point (lower-cased). */
static const char g_rvn_fold_latin1[] =
	"aaaaaa.ceeeeiiii.nooooo.ouuuuy.."   /* C0..DF */
	"aaaaaa.ceeeeiiii.nooooo.ouuuuy.y";  /* E0..FF */
/* U+0100..U+017F (Latin Extended-A): the letter; '*' pairs a capital (even) with its small letter; '-' keeps it. */
static const char g_rvn_fold_latin_a[] =
	"aaaaaacccccccc" "dddd" "eeeeeeeeee" "gggggggg" "hhhh" "iiiiiiiiii" "**" "jj" "kk" "-"
	"llllllllll" "nnnnnn" "n" "**" "oooooo" "**" "rrrrrr" "ssssssss" "tttttt" "uuuuuuuuuuuu"
	"ww" "yy" "y" "zzzzzz" "s";

uint32_t rvn_utf8_fold(uint32_t c) {
	if (c >= 'A' && c <= 'Z') return c + 32u;
	if (c < 0xC0u) return c;
	if (c <= 0xFFu) {
		char base = g_rvn_fold_latin1[c - 0xC0u];
		if (base != '.') return (uint32_t)base;
		/* Æ Ð Þ: their small letters; × ÷ ß stay */
		return (c >= 0xC0u && c <= 0xDEu && c != 0xD7u) ? c + 32u : c;
	}
	if (c <= 0x17Fu) {
		char base = g_rvn_fold_latin_a[c - 0x100u];
		if (base == '*') return (c & 1u) ? c : c + 1u;
		return base == '-' ? c : (uint32_t)base;
	}
	if (c >= 0x300u && c <= 0x36Fu) return 0;
	/* Greek: the capitals and the letters with a tonos or a dialytika */
	if (c >= 0x391u && c <= 0x3A9u && c != 0x3A2u) return c + 32u;
	switch (c) {
		case 0x386u: case 0x3ACu: return 0x3B1u;
		case 0x388u: case 0x3ADu: return 0x3B5u;
		case 0x389u: case 0x3AEu: return 0x3B7u;
		case 0x38Au: case 0x3AFu: case 0x3AAu: case 0x3CAu: case 0x390u: return 0x3B9u;
		case 0x38Cu: case 0x3CCu: return 0x3BFu;
		case 0x38Eu: case 0x3CDu: case 0x3ABu: case 0x3CBu: case 0x3B0u: return 0x3C5u;
		case 0x38Fu: case 0x3CEu: return 0x3C9u;
		case 0x3C2u: return 0x3C3u;
		/* Cyrillic: Ё ё are searched as Е е */
		case 0x401u: case 0x451u: return 0x435u;
		default: break;
	}
	if (c >= 0x400u && c <= 0x40Fu) return c + 0x50u;
	if (c >= 0x410u && c <= 0x42Fu) return c + 32u;
	return c;
}

/* The next code point of a text that a search sees (combining marks skipped); 0 at the end. */
static uint32_t rvn_utf8_next_folded(const char** text, const char* end) {
	while (*text < end) {
		uint32_t c = rvn_utf8_fold(rvn_utf8_decode(text, end));
		if (c) return c;
	}
	return 0;
}

int rvn_utf8_contains_folded(const char* text, const char* part) {
	const char* text_end;
	const char* part_end;
	if (!part || !part[0]) return 1;
	if (!text) return 0;
	text_end = text + strlen(text);
	part_end = part + strlen(part);
	while (text < text_end) {
		const char* a = text;
		const char* b = part;
		uint32_t x = 0, y;
		/* compare from here */
		for (;;) {
			y = rvn_utf8_next_folded(&b, part_end);
			if (!y) return 1;
			x = rvn_utf8_next_folded(&a, text_end);
			if (x != y) break;
		}
		if (!x) return 0;
		rvn_utf8_decode(&text, text_end);
	}
	return 0;
}
