/**
 * Raven tests - test_preedit.c
 * The bookkeeping of the input methods of Linux (platform/preedit.c), apart
 * from any window system: the pending state of text-input-v3 applied on
 * done, the serials, what delete_surrounding_text deletes, the surrounding
 * text sent, and the edits of an XIM composition. Runs everywhere.
 */

#include "test.h"
#include "../src/platform/preedit.h"

/* "日本" is 6 bytes, "é" 2 */
#define NIHON "\xE6\x97\xA5\xE6\x9C\xAC"
#define E_ACUTE "\xC3\xA9"

static void test_boundaries(void) {
	const char* text = "a" E_ACUTE NIHON;   /* a(0) é(1..2) 日(3..5) 本(6..8), 9 bytes */
	int length = (int)strlen(text);
	CHECK(rvn_preedit_boundary(text, length, -4) == 0);
	CHECK(rvn_preedit_boundary(text, length, 2) == 1);
	CHECK(rvn_preedit_boundary(text, length, 5) == 3);
	CHECK(rvn_preedit_boundary(text, length, 6) == 6);
	CHECK(rvn_preedit_boundary(text, length, 99) == 9);
	CHECK(rvn_preedit_boundary(NULL, 0, 3) == 0);
	CHECK(rvn_preedit_char_offset(text, length, 0) == 0);
	CHECK(rvn_preedit_char_offset(text, length, 2) == 3);
	CHECK(rvn_preedit_char_offset(text, length, 3) == 6);
	CHECK(rvn_preedit_char_offset(text, length, 10) == 9);
}

static void test_ti3_done(void) {
	rvn_ti3_t ti;
	memset(&ti, 0, sizeof(ti));

	/* nothing applies before done */
	rvn_ti3_preedit_string(&ti, NIHON, 3, 6);
	CHECK(ti.preedit == NULL);
	CHECK(rvn_ti3_done(&ti, 0));
	CHECK_STR(ti.preedit, NIHON);
	CHECK(ti.preedit_begin == 3 && ti.preedit_end == 6);
	CHECK(ti.commit == NULL && ti.delete_before == 0 && ti.delete_after == 0);

	/* a done without preedit_string: the composition is gone (reset to initial) */
	rvn_ti3_commit_string(&ti, NIHON);
	CHECK(rvn_ti3_done(&ti, 0));
	CHECK(ti.preedit == NULL);
	CHECK_STR(ti.commit, NIHON);

	/* and the commit does not come back with the next done */
	CHECK(rvn_ti3_done(&ti, 0));
	CHECK(ti.preedit == NULL && ti.commit == NULL);

	/* the last event of a kind wins; all kinds in one done */
	rvn_ti3_preedit_string(&ti, "x", 0, 0);
	rvn_ti3_preedit_string(&ti, "ka", -1, -1);
	rvn_ti3_commit_string(&ti, "a");
	rvn_ti3_commit_string(&ti, "b");
	rvn_ti3_delete_surrounding(&ti, 1, 0);
	rvn_ti3_delete_surrounding(&ti, 3, 2);
	CHECK(rvn_ti3_done(&ti, 0));
	CHECK_STR(ti.preedit, "ka");
	CHECK(ti.preedit_begin == 2 && ti.preedit_end == 2);   /* -1, -1: hidden, at the end */
	CHECK_STR(ti.commit, "b");
	CHECK(ti.delete_before == 3 && ti.delete_after == 2);

	/* cursor offsets: swapped, out of range, inside a character */
	rvn_ti3_preedit_string(&ti, NIHON, 6, 0);
	CHECK(rvn_ti3_done(&ti, 0));
	CHECK(ti.preedit_begin == 0 && ti.preedit_end == 6);
	rvn_ti3_preedit_string(&ti, NIHON, 1, 4);
	CHECK(rvn_ti3_done(&ti, 0));
	CHECK(ti.preedit_begin == 0 && ti.preedit_end == 3);
	rvn_ti3_preedit_string(&ti, NIHON, 2, 40);
	CHECK(rvn_ti3_done(&ti, 0));
	CHECK(ti.preedit_begin == 6 && ti.preedit_end == 6);

	/* NULL or "" is no composition */
	rvn_ti3_preedit_string(&ti, "", 0, 0);
	rvn_ti3_commit_string(&ti, NULL);
	CHECK(rvn_ti3_done(&ti, 0));
	CHECK(ti.preedit == NULL && ti.commit == NULL);

	/* serials: a done matches once it counts every commit request sent */
	ti.commits = 2;
	rvn_ti3_commit_string(&ti, "z");
	CHECK(!rvn_ti3_done(&ti, 1));
	CHECK_STR(ti.commit, "z");    /* applied all the same */
	CHECK(rvn_ti3_done(&ti, 2));

	/* clear keeps the count of commits */
	rvn_ti3_preedit_string(&ti, "pending", 0, 0);
	rvn_ti3_clear(&ti);
	CHECK(ti.preedit == NULL && ti.pending_preedit == NULL && ti.commits == 2);
	CHECK(rvn_ti3_done(&ti, 2));
	CHECK(ti.preedit == NULL);
	rvn_ti3_clear(&ti);
}

static void test_ti3_delete_range(void) {
	const char* text = "ab" NIHON "cd";   /* a b 日(2..4) 本(5..7) c(8) d(9) */
	int from = -1, to = -1;
	/* two bytes before the caret at 8 cut 本 in two: the whole character goes */
	CHECK(rvn_ti3_delete_range(text, 8, 8, 2, 0, &from, &to));
	CHECK(from == 5 && to == 8);
	CHECK(rvn_ti3_delete_range(text, 8, 8, 3, 1, &from, &to));
	CHECK(from == 5 && to == 9);
	/* after the caret, a cut character is deleted whole too */
	CHECK(rvn_ti3_delete_range(text, 2, 2, 0, 1, &from, &to));
	CHECK(from == 2 && to == 5);
	/* clamped to the text */
	CHECK(rvn_ti3_delete_range(text, 1, 1, 50, 50, &from, &to));
	CHECK(from == 0 && to == 10);
	CHECK(rvn_ti3_delete_range(text, 0, 0, 0xFFFFFFFFu, 0xFFFFFFFFu, &from, &to));
	CHECK(from == 0 && to == 10);
	/* around a selection (given either way) */
	CHECK(rvn_ti3_delete_range(text, 8, 2, 1, 1, &from, &to));
	CHECK(from == 1 && to == 9);
	/* nothing asked, or nothing there */
	CHECK(!rvn_ti3_delete_range(text, 4, 4, 0, 0, &from, &to));
	CHECK(!rvn_ti3_delete_range("", 0, 0, 2, 2, &from, &to));
	CHECK(!rvn_ti3_delete_range(NULL, 0, 0, 2, 2, &from, &to));
}

static void test_ti3_surrounding(void) {
	char big[64];
	int offset, length, cursor, anchor, i;
	/* all of it when it fits */
	rvn_ti3_surrounding("hello", 1, 3, 4000, &offset, &length, &cursor, &anchor);
	CHECK(offset == 0 && length == 5 && cursor == 3 && anchor == 1);
	rvn_ti3_surrounding(NULL, 0, 0, 4000, &offset, &length, &cursor, &anchor);
	CHECK(offset == 0 && length == 0 && cursor == 0 && anchor == 0);

	/* too long: the caret in the middle of what is sent, on boundaries */
	for (i = 0; i < 20; i++) memcpy(big + i * 3, "\xE6\x97\xA5", 3);   /* 20 x 日, 60 bytes */
	big[60] = '\0';
	rvn_ti3_surrounding(big, 30, 30, 10, &offset, &length, &cursor, &anchor);
	CHECK(length <= 10 && offset % 3 == 0 && length % 3 == 0);
	CHECK(offset <= 30 && offset + length >= 30);
	CHECK(cursor == 30 - offset && anchor == cursor);
	/* near the start, near the end */
	rvn_ti3_surrounding(big, 3, 3, 10, &offset, &length, &cursor, &anchor);
	CHECK(offset == 0 && length == 9 && cursor == 3);
	rvn_ti3_surrounding(big, 60, 60, 10, &offset, &length, &cursor, &anchor);
	CHECK(offset + length == 60 && length <= 10 && offset % 3 == 0 && cursor == length);
	/* a selection longer than the room: its beginning */
	rvn_ti3_surrounding(big, 6, 45, 10, &offset, &length, &cursor, &anchor);
	CHECK(offset == 6 && length == 9 && anchor == 0 && cursor == 9);
}

static void test_xim_edits(void) {
	char* text = NULL;
	/* typed one by one, as XIM draws: at the end */
	rvn_preedit_replace_chars(&text, 0, 0, "n");
	CHECK_STR(text, "n");
	rvn_preedit_replace_chars(&text, 0, 1, "\xE3\x81\xAB");   /* に */
	CHECK_STR(text, "\xE3\x81\xAB");
	rvn_preedit_replace_chars(&text, 1, 0, "h");
	CHECK_STR(text, "\xE3\x81\xAB" "h");
	/* characters, not bytes: the second one replaced */
	rvn_preedit_replace_chars(&text, 1, 1, "\xE3\x81\xBB\xE3\x82\x93");   /* ほん */
	CHECK_STR(text, "\xE3\x81\xAB\xE3\x81\xBB\xE3\x82\x93");
	/* converted: all of it */
	rvn_preedit_replace_chars(&text, 0, 3, NIHON);
	CHECK_STR(text, NIHON);
	/* deleting only (no text), and counts beyond the end */
	rvn_preedit_replace_chars(&text, 1, 9, NULL);
	CHECK_STR(text, "\xE6\x97\xA5");
	rvn_preedit_replace_chars(&text, 5, 2, "x");
	CHECK_STR(text, "\xE6\x97\xA5" "x");
	rvn_preedit_replace_chars(&text, -3, -1, "y");
	CHECK_STR(text, "y\xE6\x97\xA5" "x");
	/* emptied: NULL */
	rvn_preedit_replace_chars(&text, 0, 3, "");
	CHECK(text == NULL);
	rvn_preedit_replace_chars(&text, 0, 0, NULL);
	CHECK(text == NULL);
}

static void test_xim_marks(void) {
	uint32_t* marks = NULL;
	int total = 0, start = -1, end = -1;
	const uint32_t three[3] = { 0, 1, 1 };
	const uint32_t one[1] = { 4 };
	const char* text = "a" NIHON "b";   /* a 日 本 b */
	rvn_preedit_replace_marks(&marks, &total, 0, 0, three, 3);
	CHECK(total == 3 && marks[0] == 0 && marks[1] == 1 && marks[2] == 1);
	/* a fourth character, without feedback */
	rvn_preedit_replace_marks(&marks, &total, 3, 0, NULL, 1);
	CHECK(total == 4 && marks[3] == 0);
	/* the clause: characters 1 and 2, bytes 1 to 7 */
	CHECK(rvn_preedit_marked_run(text, marks, total, 1, &start, &end));
	CHECK(start == 1 && end == 7);
	CHECK(!rvn_preedit_marked_run(text, marks, total, 4, &start, &end));
	/* the middle replaced by one character of another feedback, counts clamped */
	rvn_preedit_replace_marks(&marks, &total, 1, 2, one, 1);
	CHECK(total == 3 && marks[0] == 0 && marks[1] == 4 && marks[2] == 0);
	CHECK(rvn_preedit_marked_run("x" "\xC3\xA9" "y", marks, total, 4 | 1, &start, &end));
	CHECK(start == 1 && end == 3);
	rvn_preedit_replace_marks(&marks, &total, 9, 9, one, 1);
	CHECK(total == 4 && marks[3] == 4);
	rvn_preedit_replace_marks(&marks, &total, -1, 99, NULL, 0);
	CHECK(total == 0 && marks == NULL);
	CHECK(!rvn_preedit_marked_run(text, NULL, 0, 1, &start, &end));
}

int main(void) {
	RUN(test_boundaries);
	RUN(test_ti3_done);
	RUN(test_ti3_delete_range);
	RUN(test_ti3_surrounding);
	RUN(test_xim_edits);
	RUN(test_xim_marks);
	return test_report("test_preedit");
}
