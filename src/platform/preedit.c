/**
 * Raven - platform/preedit.c
 * The bookkeeping of the input methods of Linux and the BSDs (see preedit.h).
 */

#include "preedit.h"

int rvn_preedit_boundary(const char* text, int length, int offset) {
	if (offset <= 0 || !text) return 0;
	if (offset >= length) return length;
	while (offset > 0 && ((unsigned char)text[offset] & 0xC0u) == 0x80u) offset--;
	return offset;
}

int rvn_preedit_char_offset(const char* text, int length, int index) {
	int offset = 0;
	if (!text) return 0;
	while (index-- > 0 && offset < length) offset = rvn_utf8_next(text, length, offset);
	return offset;
}

/* ---------------------------------------------------------------------------
 *  Wayland: zwp_text_input_v3
 * ------------------------------------------------------------------------- */

static void rvn_ti3_clear_pending(rvn_ti3_t* ti) {
	rvn_free(ti->pending_preedit);
	rvn_free(ti->pending_commit);
	ti->pending_preedit = NULL;
	ti->pending_commit = NULL;
	ti->pending_begin = ti->pending_end = 0;
	ti->pending_before = ti->pending_after = 0;
}

static void rvn_ti3_clear_applied(rvn_ti3_t* ti) {
	rvn_free(ti->preedit);
	rvn_free(ti->commit);
	ti->preedit = NULL;
	ti->commit = NULL;
	ti->preedit_begin = ti->preedit_end = 0;
	ti->delete_before = ti->delete_after = 0;
}

void rvn_ti3_clear(rvn_ti3_t* ti) {
	rvn_ti3_clear_pending(ti);
	rvn_ti3_clear_applied(ti);
}

void rvn_ti3_preedit_string(rvn_ti3_t* ti, const char* text, int cursor_begin, int cursor_end) {
	rvn_str_set(&ti->pending_preedit, text && text[0] ? text : NULL);
	ti->pending_begin = cursor_begin;
	ti->pending_end = cursor_end;
}

void rvn_ti3_commit_string(rvn_ti3_t* ti, const char* text) {
	rvn_str_set(&ti->pending_commit, text && text[0] ? text : NULL);
}

void rvn_ti3_delete_surrounding(rvn_ti3_t* ti, uint32_t before, uint32_t after) {
	ti->pending_before = before;
	ti->pending_after = after;
}

int rvn_ti3_done(rvn_ti3_t* ti, uint32_t serial) {
	rvn_ti3_clear_applied(ti);
	/* (moved, not copied: the pending state is then back to its initial value) */
	ti->preedit = ti->pending_preedit;
	ti->commit = ti->pending_commit;
	ti->pending_preedit = NULL;
	ti->pending_commit = NULL;
	if (ti->preedit) {
		int length = (int)strlen(ti->preedit);
		int begin = ti->pending_begin, end = ti->pending_end;
		/* -1 and -1: no caret shown; it stays at the end of the composition */
		if (begin < 0 || end < 0 || begin > length || end > length) begin = end = length;
		if (begin > end) {
			int swap = begin;
			begin = end;
			end = swap;
		}
		ti->preedit_begin = rvn_preedit_boundary(ti->preedit, length, begin);
		ti->preedit_end = rvn_preedit_boundary(ti->preedit, length, end);
	}
	ti->delete_before = ti->pending_before;
	ti->delete_after = ti->pending_after;
	ti->pending_begin = ti->pending_end = 0;
	ti->pending_before = ti->pending_after = 0;
	return serial == ti->commits;
}

int rvn_ti3_delete_range(const char* text, int start, int end, uint32_t before, uint32_t after, int* from, int* to) {
	int length = text ? (int)strlen(text) : 0;
	int low, high;
	if (start > end) {
		int swap = start;
		start = end;
		end = swap;
	}
	start = rvn_preedit_boundary(text, length, start);
	end = rvn_preedit_boundary(text, length, end);
	/* (in 64 bits: a length near 4 GB must not wrap around) */
	low = (int64_t)start - (int64_t)before < 0 ? 0 : start - (int)before;
	high = (int64_t)end + (int64_t)after > (int64_t)length ? length : end + (int)after;
	low = rvn_preedit_boundary(text, length, low);
	/* the end goes forward to a boundary: a character cut in two is deleted whole */
	while (high < length && ((unsigned char)text[high] & 0xC0u) == 0x80u) high++;
	*from = low;
	*to = high;
	return (before > 0 || after > 0) && high > low;
}

void rvn_ti3_surrounding(const char* text, int start, int end, int max_bytes, int* offset, int* length, int* cursor, int* anchor) {
	int total = text ? (int)strlen(text) : 0;
	int low, high, from, to;
	start = rvn_preedit_boundary(text, total, start);
	end = rvn_preedit_boundary(text, total, end);
	low = rvn_mini(start, end);
	high = rvn_maxi(start, end);
	if (max_bytes < 0) max_bytes = 0;
	if (total <= max_bytes) {
		from = 0;
		to = total;
	} else if (high - low >= max_bytes) {
		/* a selection longer than the room: its beginning */
		from = low;
		to = low + max_bytes;
	} else {
		/* the selection in the middle of what is sent */
		int room = max_bytes - (high - low);
		from = low - room / 2;
		to = high + (room - room / 2);
		if (from < 0) {
			to -= from;
			from = 0;
		}
		if (to > total) {
			from -= to - total;
			to = total;
		}
		if (from < 0) from = 0;
	}
	/* on character boundaries, inside the room: the start goes forward, the end back */
	while (from < total && from > 0 && ((unsigned char)text[from] & 0xC0u) == 0x80u) from++;
	to = rvn_preedit_boundary(text, total, to);
	if (to < from) to = from;
	*offset = from;
	*length = to - from;
	*cursor = rvn_maxi(0, rvn_mini(end - from, to - from));
	*anchor = rvn_maxi(0, rvn_mini(start - from, to - from));
}

/* ---------------------------------------------------------------------------
 *  X11: XIM on the spot
 * ------------------------------------------------------------------------- */

void rvn_preedit_replace_chars(char** text, int first, int count, const char* insert) {
	const char* old = *text ? *text : "";
	int length = (int)strlen(old);
	int insert_length = insert ? (int)strlen(insert) : 0;
	int from, to;
	char* result;
	if (first < 0) first = 0;
	if (count < 0) count = 0;
	from = rvn_preedit_char_offset(old, length, first);
	to = from + rvn_preedit_char_offset(old + from, length - from, count);
	if (length - (to - from) + insert_length == 0) {
		rvn_free(*text);
		*text = NULL;
		return;
	}
	result = (char*)rvn_alloc((size_t)(length - (to - from) + insert_length) + 1);
	memcpy(result, old, (size_t)from);
	if (insert_length > 0) memcpy(result + from, insert, (size_t)insert_length);
	memcpy(result + from + insert_length, old + to, (size_t)(length - to));
	result[length - (to - from) + insert_length] = '\0';
	rvn_free(*text);
	*text = result;
}

void rvn_preedit_replace_marks(uint32_t** marks, int* total, int first, int count, const uint32_t* insert, int insert_count) {
	int old = *total, size;
	uint32_t* result;
	if (first < 0) first = 0;
	if (first > old) first = old;
	if (count < 0) count = 0;
	if (count > old - first) count = old - first;
	if (insert_count < 0) insert_count = 0;
	size = old - count + insert_count;
	if (size == 0) {
		rvn_free(*marks);
		*marks = NULL;
		*total = 0;
		return;
	}
	result = (uint32_t*)rvn_calloc((size_t)size * sizeof(uint32_t));
	if (first > 0) memcpy(result, *marks, (size_t)first * sizeof(uint32_t));
	if (insert && insert_count > 0) memcpy(result + first, insert, (size_t)insert_count * sizeof(uint32_t));
	if (old - first - count > 0) memcpy(result + first + insert_count, *marks + first + count, (size_t)(old - first - count) * sizeof(uint32_t));
	rvn_free(*marks);
	*marks = result;
	*total = size;
}

int rvn_preedit_marked_run(const char* text, const uint32_t* marks, int total, uint32_t mask, int* start, int* end) {
	int length = text ? (int)strlen(text) : 0;
	int i = 0, first;
	*start = *end = 0;
	if (!marks) return R_FALSE;
	while (i < total && !(marks[i] & mask)) i++;
	if (i >= total) return R_FALSE;
	first = i;
	while (i < total && (marks[i] & mask)) i++;
	*start = rvn_preedit_char_offset(text, length, first);
	*end = rvn_preedit_char_offset(text, length, i);
	return *end > *start;
}
