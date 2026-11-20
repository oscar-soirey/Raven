/**
 * Raven - core/diff.c
 * Differences between two texts.
 *
 * Lines are compared by classes: every line gets the number of the first
 * line equal to it (for the flags: spaces, case), through a hash table, so
 * the comparison itself only compares integers.
 *
 * The comparison is Myers' O(ND) algorithm in linear space (E. Myers, "An
 * O(ND) Difference Algorithm and Its Variations", 1986, section 4b): the
 * middle snake of the two sequences is found by searching from both ends at
 * once, and both halves are compared the same way. It gives the fewest lines
 * removed and added (what `git diff` shows by default) in memory linear in
 * the size of the texts, and in time N * D: fast when the texts are alike,
 * which is what a diff is used for. Before it:
 *
 *   - the lines in common at the start and at the end are set aside;
 *   - the lines found on one side only are changes whatever the rest is:
 *     they are marked and left out of the search. Two texts with nothing in
 *     common (a file replaced) are then compared in the time of reading them.
 *
 * What remains costly is texts with very many differences made of lines
 * found on both sides (the same few lines in another order). Two limits keep
 * it bounded: a search that goes past `limit` differences stops at the
 * diagonal that went furthest and cuts there (as GNU diff does without
 * --minimal), and a budget of work for the whole comparison, past which the
 * parts left are taken as removed then added as a whole. The result is then
 * longer than the shortest one but always correct.
 *
 * Patience and histogram diffs were considered: they read better on code
 * where blocks moved, but have no bound of their own on the worst case and
 * fall back to Myers anyway when no line is unique; Myers with the two
 * filters above gives the shortest result with a known cost.
 */

#include "diff.h"

#include <limits.h>

/* ---------------------------------------------------------------------------
 *  Lines
 * ------------------------------------------------------------------------- */

void rvn_diff_split(const char* text, size_t length, rvn_diff_lines_t* out) {
	uint32_t cap = 0;
	size_t at = 0;
	out->lines = NULL;
	out->count = 0;
	if (!text) return;
	while (at < length) {
		const char* end = (const char*)memchr(text + at, '\n', length - at);
		size_t stop = end ? (size_t)(end - text) : length;
		if (stop - at > (size_t)INT_MAX) break;
		RVN_ARRAY_RESERVE(out->lines, cap, (uint32_t)out->count + 1);
		out->lines[out->count].text = text + at;
		out->lines[out->count].length = (int)(stop - at);
		out->lines[out->count].no_newline = end ? R_FALSE : R_TRUE;
		out->count++;
		at = end ? stop + 1 : length;
	}
}

void rvn_diff_lines_free(rvn_diff_lines_t* lines) {
	rvn_free(lines->lines);
	lines->lines = NULL;
	lines->count = 0;
}

static int rvn_diff_is_space(char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
}

static unsigned char rvn_diff_fold(unsigned char c, uint32_t flags) {
	return (flags & R_DIFF_IGNORE_CASE) && c >= 'A' && c <= 'Z' ? (unsigned char)(c + 32) : c;
}

/* The length that counts: without the spaces at the end when they do not. */
static int rvn_diff_effective(const rvn_diff_line_t* line, uint32_t flags) {
	int length = line->length;
	if (flags & (R_DIFF_IGNORE_TRAILING_SPACE | R_DIFF_IGNORE_SPACE)) {
		while (length > 0 && rvn_diff_is_space(line->text[length - 1])) length--;
	}
	return length;
}

static uint32_t rvn_diff_line_hash(const rvn_diff_line_t* line, uint32_t flags) {
	uint32_t hash = 2166136261u;
	int i, length = rvn_diff_effective(line, flags);
	for (i = 0; i < length; i++) {
		unsigned char c = (unsigned char)line->text[i];
		if ((flags & R_DIFF_IGNORE_SPACE) && rvn_diff_is_space((char)c)) continue;
		hash = (hash ^ rvn_diff_fold(c, flags)) * 16777619u;
	}
	/* a last line without its end differs from the same line with it, unless spaces do not count */
	if (line->no_newline && !(flags & (R_DIFF_IGNORE_TRAILING_SPACE | R_DIFF_IGNORE_SPACE))) hash = (hash ^ 0x5Au) * 16777619u;
	return hash;
}

int rvn_diff_line_equal(const rvn_diff_line_t* a, const rvn_diff_line_t* b, uint32_t flags) {
	int la = rvn_diff_effective(a, flags), lb = rvn_diff_effective(b, flags);
	int i = 0, j = 0;
	if (!(flags & (R_DIFF_IGNORE_TRAILING_SPACE | R_DIFF_IGNORE_SPACE)) && a->no_newline != b->no_newline) return R_FALSE;
	if (!(flags & (R_DIFF_IGNORE_SPACE | R_DIFF_IGNORE_CASE))) return la == lb && memcmp(a->text, b->text, (size_t)la) == 0;
	for (;;) {
		if (flags & R_DIFF_IGNORE_SPACE) {
			while (i < la && rvn_diff_is_space(a->text[i])) i++;
			while (j < lb && rvn_diff_is_space(b->text[j])) j++;
		}
		if (i >= la || j >= lb) return i >= la && j >= lb;
		if (rvn_diff_fold((unsigned char)a->text[i], flags) != rvn_diff_fold((unsigned char)b->text[j], flags)) return R_FALSE;
		i++;
		j++;
	}
}

/* ---------------------------------------------------------------------------
 *  Myers
 * ------------------------------------------------------------------------- */

typedef struct rvn_myers_t {
	const int* a;
	const int* b;
	/* the furthest x reached on each diagonal k = x - y, forward and backward: fd[k], from fd_base + offset */
	int* fd;
	int* bd;
	/* a search stops past this many steps (d / 2), and cuts where it got furthest */
	int limit;
	long budget;
	int coarse;
} rvn_myers_t;

typedef struct rvn_myers_box_t {
	int xoff, xlim, yoff, ylim;
} rvn_myers_box_t;

/* Where to cut a[xoff..xlim) x b[yoff..ylim) (both not empty, different at both ends). */
static void rvn_myers_split(rvn_myers_t* s, int xoff, int xlim, int yoff, int ylim, int* xmid, int* ymid) {
	const int* a = s->a;
	const int* b = s->b;
	int* fd = s->fd;
	int* bd = s->bd;
	int dmin = xoff - ylim, dmax = xlim - yoff;
	int fmid = xoff - yoff, bmid = xlim - ylim;
	int fmin = fmid, fmax = fmid, bmin = bmid, bmax = bmid;
	int odd = (fmid - bmid) & 1;
	int c, d;

	fd[fmid] = xoff;
	bd[bmid] = xlim;
	for (c = 1;; c++) {
		long work = 0;
		/* one step forward on every diagonal reached */
		if (fmin > dmin) fd[--fmin - 1] = -1;
		else fmin++;
		if (fmax < dmax) fd[++fmax + 1] = -1;
		else fmax--;
		for (d = fmax; d >= fmin; d -= 2) {
			int tlo = fd[d - 1], thi = fd[d + 1];
			int x0 = tlo >= thi ? tlo + 1 : thi, x = x0, y = x0 - d;
			while (x < xlim && y < ylim && a[x] == b[y]) {
				x++;
				y++;
			}
			work += 1 + (x - x0);
			fd[d] = x;
			if (odd && bmin <= d && d <= bmax && bd[d] <= x) {
				*xmid = x;
				*ymid = y;
				s->budget -= work;
				return;
			}
		}
		/* one step backward */
		if (bmin > dmin) bd[--bmin - 1] = INT_MAX;
		else bmin++;
		if (bmax < dmax) bd[++bmax + 1] = INT_MAX;
		else bmax--;
		for (d = bmax; d >= bmin; d -= 2) {
			int tlo = bd[d - 1], thi = bd[d + 1];
			int x0 = tlo < thi ? tlo : thi - 1, x = x0, y = x0 - d;
			while (x > xoff && y > yoff && a[x - 1] == b[y - 1]) {
				x--;
				y--;
			}
			work += 1 + (x0 - x);
			bd[d] = x;
			if (!odd && fmin <= d && d <= fmax && x <= fd[d]) {
				*xmid = x;
				*ymid = y;
				s->budget -= work;
				return;
			}
		}
		s->budget -= work;

		if (c >= s->limit || s->budget <= 0) {
			/* too long: cut at the point that went furthest, forward or backward */
			int fxybest = -1, fxbest = xoff, bxybest = INT_MAX, bxbest = xlim;
			for (d = fmax; d >= fmin; d -= 2) {
				int x = fd[d] < xlim ? fd[d] : xlim, y = x - d;
				if (ylim < y) {
					x = ylim + d;
					y = ylim;
				}
				if (fxybest < x + y) {
					fxybest = x + y;
					fxbest = x;
				}
			}
			for (d = bmax; d >= bmin; d -= 2) {
				int x = bd[d] > xoff ? bd[d] : xoff, y = x - d;
				if (y < yoff) {
					x = yoff + d;
					y = yoff;
				}
				if (x + y < bxybest) {
					bxybest = x + y;
					bxbest = x;
				}
			}
			s->coarse = R_TRUE;
			if ((xlim + ylim) - bxybest < fxybest - (xoff + yoff)) {
				*xmid = fxbest;
				*ymid = fxybest - fxbest;
			} else {
				*xmid = bxbest;
				*ymid = bxybest - bxbest;
			}
			return;
		}
	}
}

int rvn_diff_sequences(const int* a, int n, const int* b, int m, uint8_t* changed_a, uint8_t* changed_b, long budget) {
	rvn_myers_t s;
	rvn_myers_box_t* stack = NULL;
	uint32_t depth = 0, cap = 0;
	int* fd;
	int* bd;
	int limit = 256, size;

	if (n <= 0 || m <= 0) {
		if (n > 0) memset(changed_a, 1, (size_t)n);
		if (m > 0) memset(changed_b, 1, (size_t)m);
		return R_FALSE;
	}
	/* the diagonals run from -m - 1 to n + 1 */
	size = n + m + 3;
	fd = (int*)rvn_alloc((size_t)size * 2 * sizeof(int));
	bd = fd + size;
	s.a = a;
	s.b = b;
	s.fd = fd + m + 1;
	s.bd = bd + m + 1;
	/* about the square root of the size, as GNU diff, but never less than 256 */
	while ((long)limit * limit < (long)(n + m) * 4) limit *= 2;
	s.limit = limit;
	s.budget = budget > 0 ? budget : 24000000L + 32L * (n + m);
	s.coarse = R_FALSE;

	RVN_ARRAY_RESERVE(stack, cap, 16);
	stack[depth].xoff = 0;
	stack[depth].xlim = n;
	stack[depth].yoff = 0;
	stack[depth].ylim = m;
	depth++;
	while (depth > 0) {
		rvn_myers_box_t box = stack[--depth];
		int xmid, ymid;
		/* what is the same at both ends is not a change */
		while (box.xoff < box.xlim && box.yoff < box.ylim && a[box.xoff] == b[box.yoff]) {
			box.xoff++;
			box.yoff++;
		}
		while (box.xlim > box.xoff && box.ylim > box.yoff && a[box.xlim - 1] == b[box.ylim - 1]) {
			box.xlim--;
			box.ylim--;
		}
		if (box.xoff == box.xlim || box.yoff == box.ylim || s.budget <= 0) {
			/* one side is empty (or the work is spent): the rest of both sides is changed */
			if (box.xlim > box.xoff) memset(changed_a + box.xoff, 1, (size_t)(box.xlim - box.xoff));
			if (box.ylim > box.yoff) memset(changed_b + box.yoff, 1, (size_t)(box.ylim - box.yoff));
			if (s.budget <= 0 && box.xoff < box.xlim && box.yoff < box.ylim) s.coarse = R_TRUE;
			continue;
		}
		rvn_myers_split(&s, box.xoff, box.xlim, box.yoff, box.ylim, &xmid, &ymid);
		if ((xmid <= box.xoff && ymid <= box.yoff) || (xmid >= box.xlim && ymid >= box.ylim) ||
			xmid < box.xoff || xmid > box.xlim || ymid < box.yoff || ymid > box.ylim) {
			/* no cut that makes it smaller: take it as a whole */
			memset(changed_a + box.xoff, 1, (size_t)(box.xlim - box.xoff));
			memset(changed_b + box.yoff, 1, (size_t)(box.ylim - box.yoff));
			s.coarse = R_TRUE;
			continue;
		}
		RVN_ARRAY_RESERVE(stack, cap, depth + 2);
		stack[depth].xoff = xmid;
		stack[depth].xlim = box.xlim;
		stack[depth].yoff = ymid;
		stack[depth].ylim = box.ylim;
		depth++;
		stack[depth].xoff = box.xoff;
		stack[depth].xlim = xmid;
		stack[depth].yoff = box.yoff;
		stack[depth].ylim = ymid;
		depth++;
	}
	rvn_free(stack);
	rvn_free(fd);
	return s.coarse;
}

/* ---------------------------------------------------------------------------
 *  Line by line
 * ------------------------------------------------------------------------- */

typedef struct rvn_diff_slot_t {
	uint32_t hash;
	int line;   /* the first line of its class: index in the left lines, or left_count + index in the right ones; -1: empty */
	int cls;
} rvn_diff_slot_t;

static void rvn_diff_push(rvn_diff_t* out, rvn_diff_kind_t kind, int left, int right, int count) {
	if (count <= 0) return;
	if (out->count > 0) {
		rvn_diff_run_t* last = &out->runs[out->count - 1];
		if (last->kind == (uint8_t)kind && (kind == RVN_DIFF_REMOVED ? last->left + last->count == left :
			kind == RVN_DIFF_ADDED ? last->right + last->count == right : last->left + last->count == left)) {
			last->count += count;
			return;
		}
	}
	RVN_ARRAY_RESERVE(out->runs, out->cap, out->count + 1);
	out->runs[out->count].kind = (uint8_t)kind;
	out->runs[out->count].left = left;
	out->runs[out->count].right = right;
	out->runs[out->count].count = count;
	out->count++;
}

void rvn_diff_compute(const rvn_diff_line_t* left, int left_count, const rvn_diff_line_t* right, int right_count,
	uint32_t flags, rvn_diff_t* out) {

	int total = left_count + right_count, i, j, n = 0, m = 0, classes = 0;
	uint32_t table_size = 16, mask;
	rvn_diff_slot_t* table;
	int* cls;          /* the class of every line: left ones, then right ones */
	int* in_left;      /* lines of each class on each side */
	int* in_right;
	int* ra;
	int* rb;
	int* map_a;
	int* map_b;
	uint8_t* changed;  /* left ones, then right ones */
	uint8_t* rca;
	uint8_t* rcb;
	int prefix = 0, suffix = 0;

	out->runs = NULL;
	out->count = 0;
	out->cap = 0;
	out->coarse = R_FALSE;
	if (total == 0) return;

	/* ---- classes ---- */
	while (table_size < (uint32_t)total * 2u) table_size *= 2u;
	mask = table_size - 1u;
	table = (rvn_diff_slot_t*)rvn_alloc(table_size * sizeof(rvn_diff_slot_t));
	for (i = 0; i < (int)table_size; i++) table[i].line = -1;
	cls = (int*)rvn_alloc((size_t)total * sizeof(int));
	for (i = 0; i < total; i++) {
		const rvn_diff_line_t* line = i < left_count ? &left[i] : &right[i - left_count];
		uint32_t hash = rvn_diff_line_hash(line, flags), slot = hash & mask;
		for (;;) {
			rvn_diff_slot_t* s = &table[slot];
			if (s->line < 0) {
				s->hash = hash;
				s->line = i;
				s->cls = classes++;
				cls[i] = s->cls;
				break;
			}
			if (s->hash == hash) {
				const rvn_diff_line_t* other = s->line < left_count ? &left[s->line] : &right[s->line - left_count];
				if (rvn_diff_line_equal(line, other, flags)) {
					cls[i] = s->cls;
					break;
				}
			}
			slot = (slot + 1u) & mask;
		}
	}
	rvn_free(table);

	/* ---- the same at the start and at the end ---- */
	while (prefix < left_count && prefix < right_count && cls[prefix] == cls[left_count + prefix]) prefix++;
	while (suffix < left_count - prefix && suffix < right_count - prefix &&
		cls[left_count - 1 - suffix] == cls[left_count + right_count - 1 - suffix]) suffix++;

	/* ---- lines found on one side only are changes ---- */
	in_left = (int*)rvn_calloc((size_t)(classes ? classes : 1) * 2 * sizeof(int));
	in_right = in_left + classes;
	for (i = prefix; i < left_count - suffix; i++) in_left[cls[i]]++;
	for (j = prefix; j < right_count - suffix; j++) in_right[cls[left_count + j]]++;
	changed = (uint8_t*)rvn_calloc((size_t)total);
	ra = (int*)rvn_alloc((size_t)(total + 2) * sizeof(int) * 2);
	rb = ra + left_count + 1;
	map_a = rb + right_count + 1;
	map_b = map_a + left_count + 1;
	for (i = prefix; i < left_count - suffix; i++) {
		if (in_right[cls[i]] == 0) changed[i] = 1;
		else {
			map_a[n] = i;
			ra[n++] = cls[i];
		}
	}
	for (j = prefix; j < right_count - suffix; j++) {
		if (in_left[cls[left_count + j]] == 0) changed[left_count + j] = 1;
		else {
			map_b[m] = j;
			rb[m++] = cls[left_count + j];
		}
	}
	rvn_free(in_left);

	/* ---- the rest: Myers ---- */
	rca = (uint8_t*)rvn_calloc((size_t)(n + m + 2));
	rcb = rca + n + 1;
	out->coarse = rvn_diff_sequences(ra, n, rb, m, rca, rcb, 0);
	for (i = 0; i < n; i++) if (rca[i]) changed[map_a[i]] = 1;
	for (j = 0; j < m; j++) if (rcb[j]) changed[left_count + map_b[j]] = 1;
	rvn_free(rca);
	rvn_free(ra);
	rvn_free(cls);

	/* ---- runs ---- */
	i = 0;
	j = 0;
	while (i < left_count || j < right_count) {
		if (i < left_count && j < right_count && !changed[i] && !changed[left_count + j]) {
			int start_i = i, start_j = j;
			while (i < left_count && j < right_count && !changed[i] && !changed[left_count + j]) {
				i++;
				j++;
			}
			rvn_diff_push(out, RVN_DIFF_SAME, start_i, start_j, i - start_i);
		} else {
			int removed = 0, added = 0;
			while (i + removed < left_count && changed[i + removed]) removed++;
			while (j + added < right_count && changed[left_count + j + added]) added++;
			/* (never happens: as many lines are kept on both sides; it keeps the loop finite) */
			if (removed == 0 && added == 0) {
				if (i < left_count) removed = 1;
				else added = 1;
			}
			rvn_diff_push(out, RVN_DIFF_REMOVED, i, j, removed);
			rvn_diff_push(out, RVN_DIFF_ADDED, i + removed, j, added);
			i += removed;
			j += added;
		}
	}
	rvn_free(changed);
}

void rvn_diff_free(rvn_diff_t* diff) {
	rvn_free(diff->runs);
	diff->runs = NULL;
	diff->count = 0;
	diff->cap = 0;
}

int rvn_diff_change_count(const rvn_diff_t* diff) {
	uint32_t i;
	int count = 0, in_change = R_FALSE;
	for (i = 0; i < diff->count; i++) {
		int change = diff->runs[i].kind != RVN_DIFF_SAME;
		if (change && !in_change) count++;
		in_change = change;
	}
	return count;
}

/* ---------------------------------------------------------------------------
 *  Word by word
 * ------------------------------------------------------------------------- */

typedef struct rvn_diff_token_t {
	int start;
	int end;
	uint8_t space;
} rvn_diff_token_t;

static int rvn_diff_is_word(unsigned char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c >= 0x80;
}

/* Words, runs of spaces, and every other character alone. */
static int rvn_diff_tokens(const char* text, int length, rvn_diff_token_t* tokens) {
	int i = 0, count = 0;
	while (i < length) {
		unsigned char c = (unsigned char)text[i];
		int start = i;
		if (c == ' ' || c == '\t') {
			while (i < length && (text[i] == ' ' || text[i] == '\t')) i++;
		} else if (rvn_diff_is_word(c)) {
			while (i < length && rvn_diff_is_word((unsigned char)text[i])) i++;
		} else {
			i++;
		}
		tokens[count].start = start;
		tokens[count].end = i;
		tokens[count].space = (uint8_t)(c == ' ' || c == '\t');
		count++;
	}
	return count;
}

static int rvn_diff_token_equal(const char* a, const rvn_diff_token_t* ta, const char* b, const rvn_diff_token_t* tb, uint32_t flags) {
	int length = ta->end - ta->start, i;
	if (ta->space && tb->space && (flags & R_DIFF_IGNORE_SPACE)) return R_TRUE;
	if (length != tb->end - tb->start) return R_FALSE;
	if (!(flags & R_DIFF_IGNORE_CASE)) return memcmp(a + ta->start, b + tb->start, (size_t)length) == 0;
	for (i = 0; i < length; i++) {
		if (rvn_diff_fold((unsigned char)a[ta->start + i], flags) != rvn_diff_fold((unsigned char)b[tb->start + i], flags)) return R_FALSE;
	}
	return R_TRUE;
}

/* Byte ranges of the changed tokens of one side; ranges apart by spaces only are joined, ranges of spaces alone dropped when spaces do not count. */
static int rvn_diff_ranges(const char* text, const rvn_diff_token_t* tokens, int count, const uint8_t* changed, uint32_t flags,
	rvn_diff_range_t* ranges, int max) {

	int i, n = 0;
	for (i = 0; i < count; i++) {
		int only_space;
		int j = i;
		if (!changed[i]) continue;
		only_space = R_TRUE;
		while (j < count && changed[j]) {
			if (!tokens[j].space) only_space = R_FALSE;
			j++;
		}
		if (!(only_space && (flags & (R_DIFF_IGNORE_SPACE | R_DIFF_IGNORE_TRAILING_SPACE)))) {
			if (n > 0 && i > 0) {
				/* joined to the range before when only spaces are between */
				int k = i - 1, spaces = R_TRUE;
				while (k >= 0 && tokens[k].start >= ranges[n - 1].end) {
					if (!tokens[k].space) spaces = R_FALSE;
					k--;
				}
				if (spaces) {
					ranges[n - 1].end = tokens[j - 1].end;
					i = j - 1;
					continue;
				}
			}
			if (n < max) {
				ranges[n].start = tokens[i].start;
				ranges[n].end = tokens[j - 1].end;
				n++;
			}
		}
		i = j - 1;
	}
	(void)text;
	return n;
}

int rvn_diff_words(const char* a, int a_length, const char* b, int b_length, uint32_t flags,
	rvn_diff_range_t* a_ranges, int* a_count, rvn_diff_range_t* b_ranges, int* b_count, int max) {

	rvn_diff_token_t* ta;
	rvn_diff_token_t* tb;
	int na, nb, i, j, classes = 0, same = 0, total = 0, result = R_TRUE;
	int* ca;
	int* cb;
	uint8_t* changed;

	*a_count = 0;
	*b_count = 0;
	/* the end of the line is not a word */
	while (a_length > 0 && (a[a_length - 1] == '\r' || ((flags & (R_DIFF_IGNORE_SPACE | R_DIFF_IGNORE_TRAILING_SPACE)) && rvn_diff_is_space(a[a_length - 1])))) a_length--;
	while (b_length > 0 && (b[b_length - 1] == '\r' || ((flags & (R_DIFF_IGNORE_SPACE | R_DIFF_IGNORE_TRAILING_SPACE)) && rvn_diff_is_space(b[b_length - 1])))) b_length--;
	if (a_length > RVN_DIFF_MAX_WORD_LINE || b_length > RVN_DIFF_MAX_WORD_LINE) return R_FALSE;

	ta = (rvn_diff_token_t*)rvn_alloc((size_t)(a_length + b_length + 2) * sizeof(rvn_diff_token_t));
	tb = ta + a_length + 1;
	na = rvn_diff_tokens(a, a_length, ta);
	nb = rvn_diff_tokens(b, b_length, tb);
	ca = (int*)rvn_alloc((size_t)(na + nb + 2) * sizeof(int));
	cb = ca + na + 1;
	changed = (uint8_t*)rvn_calloc((size_t)(na + nb + 2));

	/* classes of the tokens, through a small hash table */
	{
		uint32_t size = 16, mask, k;
		int* slots;
		while (size < (uint32_t)(na + nb) * 2u) size *= 2u;
		mask = size - 1u;
		slots = (int*)rvn_alloc(size * sizeof(int));
		for (k = 0; k < size; k++) slots[k] = -1;
		for (i = 0; i < na + nb; i++) {
			const char* text = i < na ? a : b;
			const rvn_diff_token_t* token = i < na ? &ta[i] : &tb[i - na];
			uint32_t hash = 2166136261u, slot;
			int c;
			if (token->space && (flags & R_DIFF_IGNORE_SPACE)) hash = 7u;
			else for (c = token->start; c < token->end; c++) hash = (hash ^ rvn_diff_fold((unsigned char)text[c], flags)) * 16777619u;
			for (slot = hash & mask;; slot = (slot + 1u) & mask) {
				int other = slots[slot];
				if (other < 0) {
					slots[slot] = i;
					if (i < na) ca[i] = classes++;
					else cb[i - na] = classes++;
					break;
				}
				if (rvn_diff_token_equal(text, token, other < na ? a : b, other < na ? &ta[other] : &tb[other - na], flags)) {
					if (i < na) ca[i] = other < na ? ca[other] : cb[other - na];
					else cb[i - na] = other < na ? ca[other] : cb[other - na];
					break;
				}
			}
		}
		rvn_free(slots);
	}
	rvn_diff_sequences(ca, na, cb, nb, changed, changed + na + 1, 400000L);

	/* how much is kept, not counting spaces: too little and the marks would cover nearly everything */
	for (i = 0; i < na; i++) {
		if (ta[i].space) continue;
		total += ta[i].end - ta[i].start;
		if (!changed[i]) same += ta[i].end - ta[i].start;
	}
	for (j = 0; j < nb; j++) {
		if (tb[j].space) continue;
		total += tb[j].end - tb[j].start;
		if (!changed[na + 1 + j]) same += tb[j].end - tb[j].start;
	}
	if (total > 0 && same * 10 < total * 4) {
		result = R_FALSE;
	} else {
		*a_count = rvn_diff_ranges(a, ta, na, changed, flags, a_ranges, max);
		*b_count = rvn_diff_ranges(b, tb, nb, changed + na + 1, flags, b_ranges, max);
	}
	rvn_free(changed);
	rvn_free(ca);
	rvn_free(ta);
	return result;
}

/* ---------------------------------------------------------------------------
 *  Unified diffs: writing
 * ------------------------------------------------------------------------- */

static void rvn_diff_write_line(rvn_text_t* out, char mark, const rvn_diff_line_t* line) {
	char prefix[1];
	prefix[0] = mark;
	rvn_text_append(out, prefix, 1);
	rvn_text_append(out, line->text, (size_t)line->length);
	rvn_text_append(out, "\n", 1);
	if (line->no_newline) rvn_text_add(out, "\\ No newline at end of file\n");
}

static void rvn_diff_write_range(rvn_text_t* out, char sign, int start, int count) {
	/* a range of no line is named by the line before it */
	if (count == 1) rvn_text_printf(out, "%c%d", sign, start + 1);
	else rvn_text_printf(out, "%c%d,%d", sign, count == 0 ? start : start + 1, count);
}

typedef struct rvn_diff_group_t {
	uint32_t first_run, end_run;
	int left_start, left_end, right_start, right_end;
} rvn_diff_group_t;

void rvn_diff_write_unified(rvn_text_t* out, const rvn_diff_lines_t* left, const rvn_diff_lines_t* right, const rvn_diff_t* diff,
	const char* left_name, const char* right_name, int context) {

	rvn_diff_group_t* groups = NULL;
	uint32_t group_count = 0, group_cap = 0, i, g;
	if (context < 0) context = 0;

	/* the changes: runs of removed and added lines between kept ones */
	for (i = 0; i < diff->count;) {
		rvn_diff_group_t group;
		if (diff->runs[i].kind == RVN_DIFF_SAME) {
			i++;
			continue;
		}
		group.first_run = i;
		group.left_start = diff->runs[i].left;
		group.right_start = diff->runs[i].right;
		group.left_end = group.left_start;
		group.right_end = group.right_start;
		while (i < diff->count && diff->runs[i].kind != RVN_DIFF_SAME) {
			if (diff->runs[i].kind == RVN_DIFF_REMOVED) group.left_end = diff->runs[i].left + diff->runs[i].count;
			else group.right_end = diff->runs[i].right + diff->runs[i].count;
			i++;
		}
		group.end_run = i;
		RVN_ARRAY_RESERVE(groups, group_cap, group_count + 1);
		groups[group_count++] = group;
	}
	if (group_count == 0) return;

	rvn_text_printf(out, "--- %s\n+++ %s\n", left_name ? left_name : "a", right_name ? right_name : "b");
	for (g = 0; g < group_count;) {
		uint32_t last = g, k;
		int hunk_left, hunk_right, hunk_left_end, before;
		int at;
		/* changes closer than twice the context share a hunk */
		while (last + 1 < group_count && groups[last + 1].left_start - groups[last].left_end <= 2 * context) last++;
		before = groups[g].left_start < context ? groups[g].left_start : context;
		hunk_left = groups[g].left_start - before;
		hunk_right = groups[g].right_start - before;
		hunk_left_end = groups[last].left_end + context;
		if (hunk_left_end > left->count) hunk_left_end = left->count;
		rvn_text_add(out, "@@ ");
		rvn_diff_write_range(out, '-', hunk_left, hunk_left_end - hunk_left);
		rvn_text_add(out, " ");
		rvn_diff_write_range(out, '+', hunk_right, hunk_left_end - groups[last].left_end + groups[last].right_end - hunk_right);
		rvn_text_add(out, " @@\n");
		at = hunk_left;
		for (k = g; k <= last; k++) {
			uint32_t r;
			for (; at < groups[k].left_start; at++) rvn_diff_write_line(out, ' ', &left->lines[at]);
			for (r = groups[k].first_run; r < groups[k].end_run; r++) {
				const rvn_diff_run_t* run = &diff->runs[r];
				int n;
				for (n = 0; n < run->count; n++) {
					if (run->kind == RVN_DIFF_REMOVED) rvn_diff_write_line(out, '-', &left->lines[run->left + n]);
					else rvn_diff_write_line(out, '+', &right->lines[run->right + n]);
				}
			}
			at = groups[k].left_end;
		}
		for (; at < hunk_left_end; at++) rvn_diff_write_line(out, ' ', &left->lines[at]);
		g = last + 1;
	}
	rvn_free(groups);
}

/* ---------------------------------------------------------------------------
 *  Unified diffs: reading
 * ------------------------------------------------------------------------- */

static int rvn_patch_starts(const char* line, int length, const char* prefix) {
	int n = (int)strlen(prefix);
	return length >= n && memcmp(line, prefix, (size_t)n) == 0;
}

static int rvn_patch_number(const char** at, const char* end, int* value) {
	const char* p = *at;
	long number = 0;
	if (p >= end || *p < '0' || *p > '9') return R_FALSE;
	while (p < end && *p >= '0' && *p <= '9') {
		if (number < 100000000L) number = number * 10 + (*p - '0');
		p++;
	}
	*value = (int)number;
	*at = p;
	return R_TRUE;
}

/* "-12,7 +12,8 @@": a missing count is 1. */
static int rvn_patch_hunk_header(const char* line, int length, rvn_patch_hunk_t* hunk) {
	const char* p = line + 4;
	const char* end = line + length;
	hunk->left_count = 1;
	hunk->right_count = 1;
	if (!rvn_patch_number(&p, end, &hunk->left_start)) return R_FALSE;
	if (p < end && *p == ',') {
		p++;
		if (!rvn_patch_number(&p, end, &hunk->left_count)) return R_FALSE;
	}
	if (end - p < 2 || p[0] != ' ' || p[1] != '+') return R_FALSE;
	p += 2;
	if (!rvn_patch_number(&p, end, &hunk->right_start)) return R_FALSE;
	if (p < end && *p == ',') {
		p++;
		if (!rvn_patch_number(&p, end, &hunk->right_count)) return R_FALSE;
	}
	return end - p >= 3 && memcmp(p, " @@", 3) == 0;
}

/* A name after "--- " or "+++ ": up to a tab (the date some tools add). */
static char* rvn_patch_name(const char* text, int length) {
	int n = 0;
	while (n < length && text[n] != '\t') n++;
	while (n > 0 && (text[n - 1] == ' ' || text[n - 1] == '\r')) n--;
	return rvn_strndup(text, (size_t)n);
}

static rvn_patch_file_t* rvn_patch_add_file(rvn_patch_t* patch) {
	rvn_patch_file_t* file;
	RVN_ARRAY_RESERVE(patch->files, patch->file_cap, patch->file_count + 1);
	file = &patch->files[patch->file_count++];
	memset(file, 0, sizeof(*file));
	file->first_hunk = patch->hunk_count;
	return file;
}

int rvn_patch_parse(const char* text, size_t length, rvn_patch_t* out) {
	size_t at = 0;
	rvn_patch_file_t* file = NULL;
	rvn_patch_hunk_t* hunk = NULL;
	int left_left = 0, right_left = 0, left_no = 0, right_no = 0;
	/* a "diff --git" line opened a file that has no hunk yet */
	int fresh = R_FALSE;

	memset(out, 0, sizeof(*out));
	if (!text) return R_FALSE;
	while (at < length) {
		const char* line = text + at;
		const char* nl = (const char*)memchr(line, '\n', length - at);
		int n = (int)(nl ? nl - line : (const char*)text + length - line);
		int bare = n;  /* without a '\r' at the end, for the lines that are not content */
		at += (size_t)n + (nl ? 1u : 0u);
		while (bare > 0 && line[bare - 1] == '\r') bare--;

		if (hunk && (left_left > 0 || right_left > 0)) {
			char c = n > 0 ? line[0] : ' ';
			rvn_patch_line_t* pl;
			if (c == '\\') {
				if (out->line_count > hunk->first) out->lines[out->line_count - 1].no_newline = R_TRUE;
				continue;
			}
			if (c == ' ' || c == '-' || c == '+') {
				RVN_ARRAY_RESERVE(out->lines, out->line_cap, out->line_count + 1);
				pl = &out->lines[out->line_count++];
				memset(pl, 0, sizeof(*pl));
				pl->text = n > 0 ? line + 1 : line;
				pl->length = n > 0 ? n - 1 : 0;
				if (c == ' ') {
					pl->kind = RVN_DIFF_SAME;
					pl->left = left_no++;
					pl->right = right_no++;
					left_left--;
					right_left--;
				} else if (c == '-') {
					pl->kind = RVN_DIFF_REMOVED;
					pl->left = left_no++;
					left_left--;
				} else {
					pl->kind = RVN_DIFF_ADDED;
					pl->right = right_no++;
					right_left--;
				}
				hunk->count++;
				continue;
			}
			/* a hunk shorter than its header said: what follows is read as headers */
			left_left = right_left = 0;
		}
		if (n > 0 && line[0] == '\\') {
			/* the mark after the last line of a hunk */
			if (hunk && out->line_count > hunk->first) out->lines[out->line_count - 1].no_newline = R_TRUE;
			continue;
		}
		if (rvn_patch_starts(line, bare, "diff --git ")) {
			const char* names = line + 11;
			int names_length = bare - 11, k;
			file = rvn_patch_add_file(out);
			hunk = NULL;
			fresh = R_TRUE;
			/* "a/x b/x": cut at the last " b/" */
			for (k = names_length - 3; k > 0; k--) {
				if (memcmp(names + k, " b/", 3) == 0) {
					file->left_name = rvn_strndup(names, (size_t)k);
					file->right_name = rvn_strndup(names + k + 1, (size_t)(names_length - k - 1));
					break;
				}
			}
			continue;
		}
		if (rvn_patch_starts(line, bare, "--- ") && at < length && rvn_patch_starts(text + at, (int)(length - at), "+++ ")) {
			const char* next = text + at;
			const char* next_nl = (const char*)memchr(next, '\n', length - at);
			int next_n = (int)(next_nl ? next_nl - next : (const char*)text + length - next);
			at += (size_t)next_n + (next_nl ? 1u : 0u);
			if (!file || !fresh) file = rvn_patch_add_file(out);
			fresh = R_FALSE;
			hunk = NULL;
			rvn_free(file->left_name);
			rvn_free(file->right_name);
			file->left_name = rvn_patch_name(line + 4, bare - 4);
			file->right_name = rvn_patch_name(next + 4, next_n - 4);
			continue;
		}
		if (rvn_patch_starts(line, bare, "@@ -")) {
			rvn_patch_hunk_t header;
			memset(&header, 0, sizeof(header));
			if (!rvn_patch_hunk_header(line, bare, &header)) continue;
			if (!file) file = rvn_patch_add_file(out);
			fresh = R_FALSE;
			RVN_ARRAY_RESERVE(out->hunks, out->hunk_cap, out->hunk_count + 1);
			hunk = &out->hunks[out->hunk_count++];
			*hunk = header;
			hunk->first = out->line_count;
			hunk->count = 0;
			hunk->header = line;
			hunk->header_length = bare;
			file->hunk_count++;
			left_left = header.left_count;
			right_left = header.right_count;
			left_no = header.left_count == 0 ? header.left_start + 1 : header.left_start;
			right_no = header.right_count == 0 ? header.right_start + 1 : header.right_start;
			if (left_no < 1) left_no = 1;
			if (right_no < 1) right_no = 1;
			continue;
		}
		/* anything else (index, mode, binary files, text around the patch) is skipped */
	}
	/* files that had no hunk (a mode change, a binary file) are dropped */
	{
		uint32_t f, kept = 0;
		for (f = 0; f < out->file_count; f++) {
			if (out->files[f].hunk_count == 0) {
				rvn_free(out->files[f].left_name);
				rvn_free(out->files[f].right_name);
				continue;
			}
			out->files[kept++] = out->files[f];
		}
		out->file_count = kept;
	}
	return out->hunk_count > 0;
}

void rvn_patch_free(rvn_patch_t* patch) {
	uint32_t i;
	for (i = 0; i < patch->file_count; i++) {
		rvn_free(patch->files[i].left_name);
		rvn_free(patch->files[i].right_name);
	}
	rvn_free(patch->files);
	rvn_free(patch->hunks);
	rvn_free(patch->lines);
	memset(patch, 0, sizeof(*patch));
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

char* Rdiff_unified(const char* left, const char* right, const char* left_name, const char* right_name, int context,
	uint32_t flags, size_t* size) {
	rvn_diff_lines_t a, b;
	rvn_diff_t diff;
	rvn_text_t out = { NULL, 0, 0 };
	if (size) *size = 0;
	if (!left || !right) return NULL;
	rvn_diff_split(left, strlen(left), &a);
	rvn_diff_split(right, strlen(right), &b);
	rvn_diff_compute(a.lines, a.count, b.lines, b.count, flags, &diff);
	rvn_diff_write_unified(&out, &a, &b, &diff, left_name, right_name, context);
	rvn_diff_free(&diff);
	rvn_diff_lines_free(&a);
	rvn_diff_lines_free(&b);
	if (!out.data) {
		out.data = (char*)rvn_alloc(1);
		out.data[0] = '\0';
	}
	if (size) *size = out.length;
	return out.data;
}

int Rdiff_count_changes(const char* left, const char* right, uint32_t flags) {
	rvn_diff_lines_t a, b;
	rvn_diff_t diff;
	int count;
	rvn_diff_split(left ? left : "", left ? strlen(left) : 0, &a);
	rvn_diff_split(right ? right : "", right ? strlen(right) : 0, &b);
	rvn_diff_compute(a.lines, a.count, b.lines, b.count, flags, &diff);
	count = rvn_diff_change_count(&diff);
	rvn_diff_free(&diff);
	rvn_diff_lines_free(&a);
	rvn_diff_lines_free(&b);
	return count;
}
