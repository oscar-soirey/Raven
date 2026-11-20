/**
 * Raven - platform/web_input.c
 * The bookkeeping of the text field of the browser platform (see web_input.h).
 */

#include "web_input.h"
#include "preedit.h"

/* Bytes of the character that starts at text[offset] (a stray continuation byte is one character), clamped to length. */
static int rvn_web_char_bytes(const char* text, int length, int offset) {
	unsigned char lead = (unsigned char)text[offset];
	int size = lead < 0xC0u ? 1 : lead < 0xE0u ? 2 : lead < 0xF0u ? 3 : 4;
	int end = offset + 1;
	/* (a character cut short ends at the first byte that does not continue it) */
	while (end < length && end < offset + size && ((unsigned char)text[end] & 0xC0u) == 0x80u) end++;
	return end - offset;
}

/* UTF-16 units of the character at text[offset]: two for a four-byte one (out of the BMP). */
static int rvn_web_char_units(const char* text, int length, int offset) {
	return rvn_web_char_bytes(text, length, offset) == 4 ? 2 : 1;
}

int rvn_utf16_count(const char* text, int bytes) {
	int offset = 0, units = 0;
	if (!text || bytes <= 0) return 0;
	while (offset < bytes && text[offset]) {
		units += rvn_web_char_units(text, bytes, offset);
		offset += rvn_web_char_bytes(text, bytes, offset);
	}
	return units;
}

int rvn_utf16_to_utf8(const char* text, int length, int units) {
	int offset = 0, counted = 0;
	if (!text || units <= 0) return 0;
	while (offset < length) {
		int size = rvn_web_char_units(text, length, offset);
		if (counted + size > units) break;
		counted += size;
		offset += rvn_web_char_bytes(text, length, offset);
	}
	return offset;
}

void rvn_web_surrounding(const char* text, int start, int end, int max_units, rvn_web_surrounding_t* out) {
	int total = text ? (int)strlen(text) : 0;
	int low, high, from, to, units;
	start = rvn_preedit_boundary(text, total, start);
	end = rvn_preedit_boundary(text, total, end);
	low = rvn_mini(start, end);
	high = rvn_maxi(start, end);
	if (max_units < 0) max_units = 0;
	/* back from the selection, a character at a time, while it fits */
	from = low;
	units = 0;
	while (from > 0) {
		int previous = rvn_preedit_boundary(text, total, from - 1);
		int size = rvn_web_char_units(text, total, previous);
		if (units + size > max_units) break;
		units += size;
		from = previous;
	}
	/* and forward after it */
	to = high;
	units = 0;
	while (to < total) {
		int size = rvn_web_char_units(text, total, to);
		if (units + size > max_units) break;
		units += size;
		to += rvn_web_char_bytes(text, total, to);
	}
	out->offset = from;
	out->length = to - from;
	out->start16 = rvn_utf16_count(text + from, low - from);
	out->end16 = out->start16 + rvn_utf16_count(text + low, high - low);
}

static const char* rvn_web_enter_hint(RReturnKey key, RTextInputType type) {
	switch (key) {
		case R_RETURN_KEY_DONE: return "done";
		case R_RETURN_KEY_GO: return "go";
		case R_RETURN_KEY_NEXT: return "next";
		case R_RETURN_KEY_SEARCH: return "search";
		case R_RETURN_KEY_SEND: return "send";
		default: break;
	}
	/* (the default: what the browser shows for the kind of field) */
	return type == R_TEXT_INPUT_TYPE_SEARCH ? "search" : "";
}

void rvn_web_field_hints(int wants_text, rvn_web_field_kind_t kind, const RTextInputInfo* info, rvn_web_field_hints_t* out) {
	memset(out, 0, sizeof(*out));
	out->mode = RVN_WEB_FIELD_NONE;
	out->inputmode = "text";
	out->enterkeyhint = "";
	out->autocomplete = "off";
	out->autocapitalize = "off";
	if (!wants_text) return;
	if (kind == RVN_WEB_KIND_TEXT_INPUT && info) {
		out->read_only = info->read_only != 0;
		out->enterkeyhint = rvn_web_enter_hint(info->return_key, info->type);
		if (info->password) {
			/* no suggestions, no learning: a password manager may fill it */
			out->mode = RVN_WEB_FIELD_SECURE;
			out->autocomplete = "current-password";
			return;
		}
		out->mode = RVN_WEB_FIELD_MIRROR;
		switch (info->type) {
			case R_TEXT_INPUT_TYPE_EMAIL:
				out->inputmode = "email";
				out->autocomplete = "email";
				break;
			case R_TEXT_INPUT_TYPE_NUMBER: out->inputmode = "numeric"; break;
			case R_TEXT_INPUT_TYPE_DECIMAL: out->inputmode = "decimal"; break;
			case R_TEXT_INPUT_TYPE_PHONE:
				out->inputmode = "tel";
				out->autocomplete = "tel";
				break;
			case R_TEXT_INPUT_TYPE_URL:
				out->inputmode = "url";
				out->autocomplete = "url";
				break;
			case R_TEXT_INPUT_TYPE_SEARCH:
				out->inputmode = "search";
				out->spellcheck = R_TRUE;
				break;
			default:
				/* prose: suggestions, corrections, a capital at the start of a sentence */
				out->autocapitalize = "sentences";
				out->spellcheck = R_TRUE;
				break;
		}
		return;
	}
	/* the core gives no text around the caret: keys and typed text, without corrections it could not apply */
	out->mode = RVN_WEB_FIELD_KEYS;
	if (kind == RVN_WEB_KIND_TEXT_EDITOR) out->enterkeyhint = "enter";
	else if (kind == RVN_WEB_KIND_SPIN_BOX) out->inputmode = "decimal";
}
