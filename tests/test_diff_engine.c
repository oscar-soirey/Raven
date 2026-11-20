/**
 * Raven tests - the comparison of texts behind the diff view (core/diff.c).
 *
 * Small known cases, empty texts, random edit scripts (the result applied to
 * the left text must give the right one, and be no longer than the script;
 * on short texts it must keep as many lines as the longest common
 * subsequence), the flags (spaces, case), CRLF, a last line without its end,
 * UTF-8, the words inside a line, unified diffs written then read back and
 * applied, and the time taken by large and pathological inputs.
 */

#include "test.h"
#include "../src/core/diff.h"

#include <stdlib.h>

/* ---- helpers ---- */

static uint32_t g_seed = 12345u;
static uint32_t rnd(void) {
	g_seed = g_seed * 1664525u + 1013904223u;
	return g_seed >> 8;
}

typedef struct text_t {
	char* data;
	size_t length, cap;
} text_t;

static void text_add(text_t* t, const char* s, size_t n) {
	if (t->length + n + 1 > t->cap) {
		t->cap = (t->length + n + 1) * 2;
		t->data = (char*)realloc(t->data, t->cap);
	}
	memcpy(t->data + t->length, s, n);
	t->length += n;
	t->data[t->length] = '\0';
}

static void diff_texts(const char* a, const char* b, uint32_t flags, rvn_diff_lines_t* la, rvn_diff_lines_t* lb, rvn_diff_t* d) {
	rvn_diff_split(a, strlen(a), la);
	rvn_diff_split(b, strlen(b), lb);
	rvn_diff_compute(la->lines, la->count, lb->lines, lb->count, flags, d);
}

/* The runs are consistent, and give back both texts. Returns the number of lines removed + added. */
static int check_runs(const rvn_diff_lines_t* a, const rvn_diff_lines_t* b, const rvn_diff_t* d, uint32_t flags) {
	uint32_t i;
	int left = 0, right = 0, edits = 0, ok = 1;
	for (i = 0; i < d->count; i++) {
		const rvn_diff_run_t* r = &d->runs[i];
		int n;
		if (r->count <= 0) ok = 0;
		if (r->kind == RVN_DIFF_SAME) {
			if (r->left != left || r->right != right) ok = 0;
			for (n = 0; n < r->count && ok; n++) {
				if (!rvn_diff_line_equal(&a->lines[left + n], &b->lines[right + n], flags)) ok = 0;
			}
			left += r->count;
			right += r->count;
		} else if (r->kind == RVN_DIFF_REMOVED) {
			if (r->left != left) ok = 0;
			left += r->count;
			edits += r->count;
		} else {
			if (r->right != right) ok = 0;
			right += r->count;
			edits += r->count;
		}
	}
	if (left != a->count || right != b->count) ok = 0;
	CHECK(ok);
	return edits;
}

static void free_all(rvn_diff_lines_t* a, rvn_diff_lines_t* b, rvn_diff_t* d) {
	rvn_diff_free(d);
	rvn_diff_lines_free(a);
	rvn_diff_lines_free(b);
}

static int count_changes(const char* a, const char* b, uint32_t flags) {
	rvn_diff_lines_t la, lb;
	rvn_diff_t d;
	int count;
	diff_texts(a, b, flags, &la, &lb, &d);
	check_runs(&la, &lb, &d, flags);
	count = rvn_diff_change_count(&d);
	free_all(&la, &lb, &d);
	return count;
}

/* ---- known cases ---- */

static void test_small(void) {
	rvn_diff_lines_t a, b;
	rvn_diff_t d;

	diff_texts("a\nb\nc\n", "a\nx\nc\n", 0, &a, &b, &d);
	CHECK(d.count == 4);
	CHECK(d.runs[0].kind == RVN_DIFF_SAME && d.runs[0].count == 1);
	CHECK(d.runs[1].kind == RVN_DIFF_REMOVED && d.runs[1].left == 1 && d.runs[1].count == 1);
	CHECK(d.runs[2].kind == RVN_DIFF_ADDED && d.runs[2].right == 1 && d.runs[2].count == 1);
	CHECK(d.runs[3].kind == RVN_DIFF_SAME && d.runs[3].left == 2 && d.runs[3].right == 2);
	CHECK(rvn_diff_change_count(&d) == 1);
	CHECK(check_runs(&a, &b, &d, 0) == 2);
	free_all(&a, &b, &d);

	/* the classic of the paper: ABCABBA -> CBABAC, 5 edits */
	diff_texts("A\nB\nC\nA\nB\nB\nA\n", "C\nB\nA\nB\nA\nC\n", 0, &a, &b, &d);
	CHECK(check_runs(&a, &b, &d, 0) == 5);
	CHECK(!d.coarse);
	free_all(&a, &b, &d);

	/* identical */
	diff_texts("x\ny\n", "x\ny\n", 0, &a, &b, &d);
	CHECK(d.count == 1 && d.runs[0].kind == RVN_DIFF_SAME && d.runs[0].count == 2);
	free_all(&a, &b, &d);

	/* lines inserted at the start and removed at the end */
	CHECK(count_changes("b\nc\nd\n", "a\nb\nc\n", 0) == 2);
	/* two separate changes */
	CHECK(count_changes("1\n2\n3\n4\n5\n6\n", "1\nX\n3\n4\nY\n6\n", 0) == 2);
}

static void test_empty(void) {
	rvn_diff_lines_t a, b;
	rvn_diff_t d;

	diff_texts("", "", 0, &a, &b, &d);
	CHECK(a.count == 0 && b.count == 0 && d.count == 0);
	free_all(&a, &b, &d);

	diff_texts("", "a\nb\n", 0, &a, &b, &d);
	CHECK(d.count == 1 && d.runs[0].kind == RVN_DIFF_ADDED && d.runs[0].count == 2);
	free_all(&a, &b, &d);

	diff_texts("a\nb", "", 0, &a, &b, &d);
	CHECK(a.count == 2 && a.lines[1].no_newline);
	CHECK(d.count == 1 && d.runs[0].kind == RVN_DIFF_REMOVED && d.runs[0].count == 2);
	free_all(&a, &b, &d);

	/* "\n" is one empty line */
	diff_texts("\n", "", 0, &a, &b, &d);
	CHECK(a.count == 1 && a.lines[0].length == 0);
	free_all(&a, &b, &d);
}

/* ---- random edit scripts ---- */

/* The longest common subsequence, by dynamic programming (short texts). */
static int lcs(const rvn_diff_lines_t* a, const rvn_diff_lines_t* b) {
	int n = a->count, m = b->count, i, j, result;
	int* row = (int*)calloc((size_t)(m + 1) * 2, sizeof(int));
	int* prev = row;
	int* cur = row + m + 1;
	for (i = 1; i <= n; i++) {
		for (j = 1; j <= m; j++) {
			if (rvn_diff_line_equal(&a->lines[i - 1], &b->lines[j - 1], 0)) cur[j] = prev[j - 1] + 1;
			else cur[j] = prev[j] > cur[j - 1] ? prev[j] : cur[j - 1];
		}
		{
			int* t = prev;
			prev = cur;
			cur = t;
		}
	}
	result = prev[m];
	free(row);
	return result;
}

/* A text of `lines` lines taken from a vocabulary of `words` lines, and the same text after `edits` random edits. */
static void make_pair(int lines, int words, int edits, text_t* left, text_t* right, int* script) {
	int* items = (int*)malloc(sizeof(int) * (size_t)(lines + edits + 1));
	int count = lines, e, i;
	char buffer[32];
	*script = 0;
	for (i = 0; i < lines; i++) items[i] = (int)(rnd() % (uint32_t)words);
	for (i = 0; i < count; i++) {
		int n = snprintf(buffer, sizeof(buffer), "line %d\n", items[i]);
		text_add(left, buffer, (size_t)n);
	}
	for (e = 0; e < edits; e++) {
		int kind = (int)(rnd() % 3u), at = count > 0 ? (int)(rnd() % (uint32_t)count) : 0;
		if (kind == 0 && count > 0) {
			/* delete */
			memmove(items + at, items + at + 1, sizeof(int) * (size_t)(count - at - 1));
			count--;
			*script += 1;
		} else if (kind == 1 || count == 0) {
			/* insert */
			memmove(items + at + 1, items + at, sizeof(int) * (size_t)(count - at));
			items[at] = (int)(rnd() % (uint32_t)(words * 2));
			count++;
			*script += 1;
		} else {
			/* replace */
			items[at] = (int)(rnd() % (uint32_t)(words * 2));
			*script += 2;
		}
	}
	for (i = 0; i < count; i++) {
		int n = snprintf(buffer, sizeof(buffer), "line %d\n", items[i]);
		text_add(right, buffer, (size_t)n);
	}
	free(items);
}

static void test_random(void) {
	int round;
	for (round = 0; round < 300; round++) {
		text_t left = { 0 }, right = { 0 };
		rvn_diff_lines_t a, b;
		rvn_diff_t d;
		int script, edits, small = round < 200;
		int lines = small ? (int)(rnd() % 60u) : 200 + (int)(rnd() % 2000u);
		int words = small ? 2 + (int)(rnd() % 8u) : 5 + (int)(rnd() % 400u);
		make_pair(lines, words, (int)(rnd() % (small ? 12u : 80u)), &left, &right, &script);
		text_add(&left, "", 0);
		text_add(&right, "", 0);
		diff_texts(left.data, right.data, 0, &a, &b, &d);
		edits = check_runs(&a, &b, &d, 0);
		/* never longer than the edits that made it */
		CHECK(edits <= script || d.coarse);
		/* and the shortest one: as many lines kept as the longest common subsequence */
		if (small && !d.coarse) CHECK((a.count + b.count - edits) / 2 == lcs(&a, &b));
		free_all(&a, &b, &d);
		free(left.data);
		free(right.data);
	}
}

/* ---- flags, ends of lines, UTF-8 ---- */

static void test_flags(void) {
	CHECK(count_changes("a b\nc\n", "a  b\nc\n", 0) == 1);
	CHECK(count_changes("a b\nc\n", "a  b\nc\n", R_DIFF_IGNORE_TRAILING_SPACE) == 1);
	CHECK(count_changes("a b\nc\n", "a\tb \nc\n", R_DIFF_IGNORE_SPACE) == 0);
	CHECK(count_changes("ab\n", "a b\n", R_DIFF_IGNORE_SPACE) == 0);
	CHECK(count_changes("x = 1;  \n", "x = 1;\n", R_DIFF_IGNORE_TRAILING_SPACE) == 0);
	CHECK(count_changes("x = 1;  \n", "x = 1;\n", 0) == 1);
	CHECK(count_changes("Hello World\n", "hello world\n", 0) == 1);
	CHECK(count_changes("Hello World\n", "hello world\n", R_DIFF_IGNORE_CASE) == 0);
	CHECK(count_changes("Hello  World\n", "hello world \n", R_DIFF_IGNORE_CASE | R_DIFF_IGNORE_SPACE) == 0);
	/* blank lines and lines of spaces */
	CHECK(count_changes("a\n   \nb\n", "a\n\nb\n", R_DIFF_IGNORE_TRAILING_SPACE) == 0);
}

static void test_line_ends(void) {
	rvn_diff_lines_t a;
	/* CRLF: the '\r' is part of the line, unless spaces at the end do not count */
	CHECK(count_changes("a\r\nb\r\n", "a\nb\n", 0) == 1);
	CHECK(count_changes("a\r\nb\r\n", "a\nb\n", R_DIFF_IGNORE_TRAILING_SPACE) == 0);
	rvn_diff_split("a\r\nb", 4, &a);
	CHECK(a.count == 2 && a.lines[0].length == 2 && a.lines[1].length == 1 && a.lines[1].no_newline);
	rvn_diff_lines_free(&a);
	/* a last line without its end differs from the same line with it */
	CHECK(count_changes("a\nb", "a\nb\n", 0) == 1);
	CHECK(count_changes("a\nb", "a\nb\n", R_DIFF_IGNORE_TRAILING_SPACE) == 0);
	CHECK(count_changes("a\nb", "a\nb", 0) == 0);
}

static void test_utf8(void) {
	rvn_diff_range_t ra[8], rb[8];
	int na, nb;
	const char* a = "caf\xC3\xA9 cr\xC3\xA8me";
	const char* b = "caf\xC3\xA9 br\xC3\xBBl\xC3\xA9";
	CHECK(count_changes("na\xC3\xAFve\n\xE6\x97\xA5\xE6\x9C\xAC\n", "na\xC3\xAFve\n\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E\n", 0) == 1);
	/* a word of several bytes is one word */
	CHECK(rvn_diff_words(a, (int)strlen(a), b, (int)strlen(b), 0, ra, &na, rb, &nb, 8));
	CHECK(na == 1 && ra[0].start == 6 && ra[0].end == (int)strlen(a));
	CHECK(nb == 1 && rb[0].start == 6 && rb[0].end == (int)strlen(b));
}

/* ---- words ---- */

static void test_words(void) {
	rvn_diff_range_t ra[8], rb[8];
	int na, nb;
	const char* a = "int count = 3;";
	const char* b = "int total = 3;";
	CHECK(rvn_diff_words(a, (int)strlen(a), b, (int)strlen(b), 0, ra, &na, rb, &nb, 8));
	CHECK(na == 1 && ra[0].start == 4 && ra[0].end == 9);
	CHECK(nb == 1 && rb[0].start == 4 && rb[0].end == 9);

	/* words next to each other, apart by spaces only, are one mark */
	a = "foo bar baz";
	b = "foo qux quux baz";
	CHECK(rvn_diff_words(a, (int)strlen(a), b, (int)strlen(b), 0, ra, &na, rb, &nb, 8));
	CHECK(na == 1 && ra[0].start == 4 && ra[0].end == 7);
	CHECK(nb == 1 && rb[0].start == 4 && rb[0].end == 12);

	/* two changes far apart: two marks */
	a = "return add(first, second);";
	b = "return sub(first, third);";
	CHECK(rvn_diff_words(a, (int)strlen(a), b, (int)strlen(b), 0, ra, &na, rb, &nb, 8));
	CHECK(na == 2 && nb == 2);

	/* only spaces changed, spaces do not count: nothing to mark */
	a = "x = 1;";
	b = "x  =  1;";
	CHECK(rvn_diff_words(a, (int)strlen(a), b, (int)strlen(b), R_DIFF_IGNORE_SPACE, ra, &na, rb, &nb, 8));
	CHECK(na == 0 && nb == 0);

	/* lines with nothing in common: no marks, the whole line is the change */
	a = "alpha beta gamma";
	b = "#include <stdio.h>";
	CHECK(!rvn_diff_words(a, (int)strlen(a), b, (int)strlen(b), 0, ra, &na, rb, &nb, 8));
	CHECK(na == 0 && nb == 0);

	/* a carriage return is not a change of the line */
	a = "value = 1\r";
	b = "value = 2";
	CHECK(rvn_diff_words(a, (int)strlen(a), b, (int)strlen(b), 0, ra, &na, rb, &nb, 8));
	CHECK(na == 1 && ra[0].start == 8 && ra[0].end == 9);
}

/* ---- unified diffs ---- */

/* Applies a patch read by rvn_patch_parse to a text; NULL when it does not apply. */
static char* apply_patch(const char* original, const rvn_patch_t* patch) {
	rvn_diff_lines_t lines;
	text_t out = { 0 };
	uint32_t h;
	int at = 0, no_newline = 0;
	rvn_diff_split(original, strlen(original), &lines);
	text_add(&out, "", 0);
	for (h = 0; h < patch->hunk_count; h++) {
		const rvn_patch_hunk_t* hunk = &patch->hunks[h];
		int start = hunk->left_count == 0 ? hunk->left_start : hunk->left_start - 1;
		uint32_t l;
		if (start < at || start > lines.count) goto fail;
		for (; at < start; at++) {
			text_add(&out, lines.lines[at].text, (size_t)lines.lines[at].length);
			text_add(&out, "\n", 1);
		}
		for (l = hunk->first; l < hunk->first + hunk->count; l++) {
			const rvn_patch_line_t* pl = &patch->lines[l];
			if (pl->kind != RVN_DIFF_ADDED) {
				if (at >= lines.count || lines.lines[at].length != pl->length ||
					memcmp(lines.lines[at].text, pl->text, (size_t)pl->length) != 0) goto fail;
				at++;
			}
			if (pl->kind != RVN_DIFF_REMOVED) {
				text_add(&out, pl->text, (size_t)pl->length);
				if (!pl->no_newline) text_add(&out, "\n", 1);
				else no_newline = 1;
			}
		}
	}
	for (; at < lines.count; at++) {
		text_add(&out, lines.lines[at].text, (size_t)lines.lines[at].length);
		if (!lines.lines[at].no_newline) text_add(&out, "\n", 1);
	}
	(void)no_newline;
	rvn_diff_lines_free(&lines);
	return out.data;
fail:
	rvn_diff_lines_free(&lines);
	free(out.data);
	return NULL;
}

static void test_unified(void) {
	size_t size = 0;
	char* patch = Rdiff_unified("a\nb\nc\nd\ne\nf\ng\nh\n", "a\nb\nc\nD\ne\nf\ng\nh\ni\n", "a/x.txt", "b/x.txt", 1, 0, &size);
	CHECK(patch && size == strlen(patch));
	CHECK_STR(patch,
		"--- a/x.txt\n+++ b/x.txt\n"
		"@@ -3,3 +3,3 @@\n c\n-d\n+D\n e\n"
		"@@ -8 +8,2 @@\n h\n+i\n");
	Rmemory_free(patch);

	/* the same texts: an empty patch */
	patch = Rdiff_unified("same\n", "same\n", NULL, NULL, 3, 0, NULL);
	CHECK_STR(patch, "");
	Rmemory_free(patch);
	CHECK(Rdiff_unified(NULL, "x", NULL, NULL, 3, 0, NULL) == NULL);

	/* a new file, and a last line without its end */
	patch = Rdiff_unified("", "one\ntwo", NULL, NULL, 3, 0, NULL);
	CHECK_STR(patch, "--- a\n+++ b\n@@ -0,0 +1,2 @@\n+one\n+two\n\\ No newline at end of file\n");
	Rmemory_free(patch);

	CHECK(Rdiff_count_changes("1\n2\n3\n", "1\n3\n4\n", 0) == 2);
	CHECK(Rdiff_count_changes(NULL, NULL, 0) == 0);
}

/* Random texts: the patch written, read back and applied gives the right text. */
static void test_round_trip(void) {
	int round;
	for (round = 0; round < 120; round++) {
		text_t left = { 0 }, right = { 0 };
		rvn_patch_t parsed;
		int script, context = (int)(rnd() % 4u);
		char* patch;
		char* applied;
		make_pair(1 + (int)(rnd() % 300u), 3 + (int)(rnd() % 50u), (int)(rnd() % 30u), &left, &right, &script);
		text_add(&left, "", 0);
		text_add(&right, "", 0);
		/* sometimes without the end of the last line */
		if (round % 5 == 1 && right.length > 0) right.data[--right.length] = '\0';
		patch = Rdiff_unified(left.data, right.data, "a/f", "b/f", context, 0, NULL);
		if (patch[0]) {
			CHECK(rvn_patch_parse(patch, strlen(patch), &parsed));
			CHECK(parsed.file_count == 1);
			applied = apply_patch(left.data, &parsed);
			CHECK(applied != NULL);
			if (applied) CHECK(strcmp(applied, right.data) == 0);
			free(applied);
			rvn_patch_free(&parsed);
		} else {
			CHECK(strcmp(left.data, right.data) == 0);
		}
		Rmemory_free(patch);
		free(left.data);
		free(right.data);
	}
}

static void test_patch_parse(void) {
	static const char* git =
		"commit 0123456\n"
		"Author: Someone\n\n    A message\n\n"
		"diff --git a/src/main.c b/src/main.c\n"
		"index 1111111..2222222 100644\n"
		"--- a/src/main.c\n"
		"+++ b/src/main.c\n"
		"@@ -1,4 +1,5 @@ int main(void)\n"
		" #include <stdio.h>\n"
		"-int x;\n"
		"+int y;\n"
		"+int z;\n"
		" \n"
		"\n"
		"@@ -10 +11 @@\n"
		"-old\n"
		"+new\n"
		"\\ No newline at end of file\n"
		"diff --git a/img.png b/img.png\n"
		"Binary files a/img.png and b/img.png differ\n"
		"diff --git a/README b/README\n"
		"new file mode 100644\n"
		"--- /dev/null\n"
		"+++ b/README\n"
		"@@ -0,0 +1,2 @@\n"
		"+hello\r\n"
		"+world\r\n";
	rvn_patch_t p;
	CHECK(rvn_patch_parse(git, strlen(git), &p));
	/* the binary file has no hunk: it is not a file of the patch */
	CHECK(p.file_count == 2);
	CHECK(p.hunk_count == 3);
	if (p.file_count == 2 && p.hunk_count == 3) {
		CHECK_STR(p.files[0].left_name, "a/src/main.c");
		CHECK_STR(p.files[0].right_name, "b/src/main.c");
		CHECK_STR(p.files[1].left_name, "/dev/null");
		CHECK(p.files[0].hunk_count == 2 && p.files[1].first_hunk == 2);
		CHECK(p.hunks[0].count == 6);
		CHECK(p.hunks[0].header_length == (int)strlen("@@ -1,4 +1,5 @@ int main(void)"));
		CHECK(p.lines[0].kind == RVN_DIFF_SAME && p.lines[0].left == 1 && p.lines[0].right == 1);
		CHECK(p.lines[1].kind == RVN_DIFF_REMOVED && p.lines[1].left == 2 && p.lines[1].right == 0);
		CHECK(p.lines[3].kind == RVN_DIFF_ADDED && p.lines[3].right == 3);
		/* " " and "" are both an empty kept line */
		CHECK(p.lines[4].kind == RVN_DIFF_SAME && p.lines[4].length == 0 && p.lines[5].kind == RVN_DIFF_SAME);
		CHECK(p.lines[5].left == 4 && p.lines[5].right == 5);
		/* "@@ -10 +11 @@": counts of 1 */
		CHECK(p.hunks[1].left_start == 10 && p.hunks[1].left_count == 1 && p.hunks[1].right_count == 1);
		CHECK(p.lines[7].kind == RVN_DIFF_ADDED && p.lines[7].no_newline);
		/* the lines of a CRLF file keep their '\r' */
		CHECK(p.lines[8].length == 6 && p.lines[8].text[5] == '\r');
	}
	rvn_patch_free(&p);

	CHECK(!rvn_patch_parse("not a patch\n", 12, &p));
	rvn_patch_free(&p);
	/* a bare hunk, without names */
	CHECK(rvn_patch_parse("@@ -1 +1 @@\n-a\n+b\n", 18, &p));
	CHECK(p.file_count == 1 && p.files[0].left_name == NULL);
	rvn_patch_free(&p);
}

/* ---- time ---- */

static double best_of(int runs, const rvn_diff_lines_t* a, const rvn_diff_lines_t* b, int* coarse, int* edits) {
	double best = 1e9;
	int r;
	for (r = 0; r < runs; r++) {
		rvn_diff_t d;
		double start = Rget_time(), t;
		rvn_diff_compute(a->lines, a->count, b->lines, b->count, 0, &d);
		t = Rget_time() - start;
		if (t < best) best = t;
		*coarse = d.coarse;
		*edits = check_runs(a, b, &d, 0);
		rvn_diff_free(&d);
	}
	return best;
}

static void test_performance(void) {
	text_t left = { 0 }, right = { 0 };
	rvn_diff_lines_t a, b;
	int coarse, edits, i;
	double t;
	char line[64];

	/* 50 000 lines of code-like text, 1% of them edited */
	g_seed = 777u;
	for (i = 0; i < 50000; i++) {
		int n = snprintf(line, sizeof(line), "    value_%u = compute(%u, %d);\n", rnd() % 5000u, rnd() % 100u, i % 7);
		text_add(&left, line, (size_t)n);
	}
	{
		rvn_diff_lines_t base;
		rvn_diff_split(left.data, left.length, &base);
		for (i = 0; i < base.count; i++) {
			if (rnd() % 100u == 0) {
				int n = snprintf(line, sizeof(line), "    edited_%u();\n", rnd());
				text_add(&right, line, (size_t)n);
				if (rnd() % 2u) continue;
			}
			text_add(&right, base.lines[i].text, (size_t)base.lines[i].length);
			text_add(&right, "\n", 1);
		}
		rvn_diff_lines_free(&base);
	}
	rvn_diff_split(left.data, left.length, &a);
	rvn_diff_split(right.data, right.length, &b);
	t = best_of(3, &a, &b, &coarse, &edits);
	printf("  50k lines, 1%% edited: %.1f ms, %d lines removed or added%s\n", t * 1000.0, edits, coarse ? " (coarse)" : "");
	CHECK(t < 0.150);
	CHECK(!coarse);
	rvn_diff_lines_free(&a);
	rvn_diff_lines_free(&b);
	free(left.data);
	free(right.data);

	/* nothing in common: the lines of one side are not searched for in the other */
	left.data = right.data = NULL;
	left.length = right.length = left.cap = right.cap = 0;
	for (i = 0; i < 50000; i++) {
		int n = snprintf(line, sizeof(line), "left %d\n", i);
		text_add(&left, line, (size_t)n);
		n = snprintf(line, sizeof(line), "right %d\n", i);
		text_add(&right, line, (size_t)n);
	}
	rvn_diff_split(left.data, left.length, &a);
	rvn_diff_split(right.data, right.length, &b);
	t = best_of(1, &a, &b, &coarse, &edits);
	printf("  50k lines, all different: %.1f ms\n", t * 1000.0);
	CHECK(edits == 100000);
	CHECK(t < 0.150);
	rvn_diff_lines_free(&a);
	rvn_diff_lines_free(&b);
	free(left.data);
	free(right.data);

	/* the worst: the same three lines in another random order on each side */
	left.data = right.data = NULL;
	left.length = right.length = left.cap = right.cap = 0;
	for (i = 0; i < 50000; i++) {
		static const char* three[3] = { "{\n", "}\n", "\n" };
		const char* x = three[rnd() % 3u];
		const char* y = three[rnd() % 3u];
		text_add(&left, x, strlen(x));
		text_add(&right, y, strlen(y));
	}
	rvn_diff_split(left.data, left.length, &a);
	rvn_diff_split(right.data, right.length, &b);
	t = best_of(1, &a, &b, &coarse, &edits);
	printf("  50k lines, three lines shuffled: %.1f ms, %d edits%s\n", t * 1000.0, edits, coarse ? " (coarse)" : "");
	/* bounded: well under a second even unoptimized */
	CHECK(t < 1.5);
	rvn_diff_lines_free(&a);
	rvn_diff_lines_free(&b);
	free(left.data);
	free(right.data);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	RUN(test_small);
	RUN(test_empty);
	RUN(test_random);
	RUN(test_flags);
	RUN(test_line_ends);
	RUN(test_utf8);
	RUN(test_words);
	RUN(test_unified);
	RUN(test_round_trip);
	RUN(test_patch_parse);
	RUN(test_performance);
	Rshutdown();
	return test_report("test_diff_engine");
}
