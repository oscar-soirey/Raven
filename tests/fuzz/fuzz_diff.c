/**
 * Raven fuzzers - diffs (core/diff.c): the reader of unified patches
 * (rvn_patch_parse: `git diff` output, CRLF, binary and mode-only files),
 * and the comparison of two texts written as a patch (Rdiff_unified), read
 * back and applied. The input is the patch; split at its first 0 byte (or
 * in the middle), it is also the two texts.
 *
 * Invariants: what the reader gives points into the text it read; the
 * patch Raven writes reads back, and applied to the left text gives the
 * right one.
 */

#include "fuzz.h"
#include "../../src/core/diff.h"

int LLVMFuzzerInitialize(int* argc, char*** argv) {
	(void)argc;
	(void)argv;
	return 0;
}

static void fuzz_check_patch(const rvn_patch_t* patch, const char* text, size_t size) {
	uint32_t i;
	for (i = 0; i < patch->file_count; i++) {
		const rvn_patch_file_t* file = &patch->files[i];
		FUZZ_CHECK(file->first_hunk <= patch->hunk_count && file->hunk_count <= patch->hunk_count - file->first_hunk);
	}
	for (i = 0; i < patch->hunk_count; i++) {
		const rvn_patch_hunk_t* hunk = &patch->hunks[i];
		FUZZ_CHECK(hunk->first <= patch->line_count && hunk->count <= patch->line_count - hunk->first);
		FUZZ_CHECK(hunk->header_length >= 0);
		if (hunk->header_length > 0) FUZZ_CHECK(hunk->header >= text && hunk->header + hunk->header_length <= text + size);
	}
	for (i = 0; i < patch->line_count; i++) {
		const rvn_patch_line_t* line = &patch->lines[i];
		FUZZ_CHECK(line->kind == RVN_DIFF_SAME || line->kind == RVN_DIFF_REMOVED || line->kind == RVN_DIFF_ADDED);
		FUZZ_CHECK(line->length >= 0);
		if (line->length > 0) FUZZ_CHECK(line->text >= text && line->text + line->length <= text + size);
	}
}

typedef struct fuzz_text_t {
	char* data;
	size_t length, cap;
} fuzz_text_t;

static void fuzz_text_add(fuzz_text_t* t, const char* bytes, size_t n) {
	if (t->length + n + 1 > t->cap) {
		t->cap = (t->length + n + 1) * 2;
		t->data = (char*)realloc(t->data, t->cap);
	}
	memcpy(t->data + t->length, bytes, n);
	t->length += n;
	t->data[t->length] = '\0';
}

/* Applies a patch of one file to a text (as tests/test_diff_engine.c does); NULL when it does not apply. */
static char* fuzz_apply(const char* original, const rvn_patch_t* patch) {
	rvn_diff_lines_t lines;
	fuzz_text_t out = { NULL, 0, 0 };
	uint32_t h;
	int at = 0;
	rvn_diff_split(original, strlen(original), &lines);
	fuzz_text_add(&out, "", 0);
	for (h = 0; h < patch->hunk_count; h++) {
		const rvn_patch_hunk_t* hunk = &patch->hunks[h];
		int start = hunk->left_count == 0 ? hunk->left_start : hunk->left_start - 1;
		uint32_t l;
		if (start < at || start > lines.count) goto fail;
		for (; at < start; at++) {
			fuzz_text_add(&out, lines.lines[at].text, (size_t)lines.lines[at].length);
			fuzz_text_add(&out, "\n", 1);
		}
		for (l = hunk->first; l < hunk->first + hunk->count; l++) {
			const rvn_patch_line_t* pl = &patch->lines[l];
			if (pl->kind != RVN_DIFF_ADDED) {
				if (at >= lines.count || lines.lines[at].length != pl->length ||
					memcmp(lines.lines[at].text, pl->text, (size_t)pl->length) != 0) goto fail;
				at++;
			}
			if (pl->kind != RVN_DIFF_REMOVED) {
				fuzz_text_add(&out, pl->text, (size_t)pl->length);
				if (!pl->no_newline) fuzz_text_add(&out, "\n", 1);
			}
		}
	}
	for (; at < lines.count; at++) {
		fuzz_text_add(&out, lines.lines[at].text, (size_t)lines.lines[at].length);
		if (!lines.lines[at].no_newline) fuzz_text_add(&out, "\n", 1);
	}
	rvn_diff_lines_free(&lines);
	return out.data;
fail:
	rvn_diff_lines_free(&lines);
	free(out.data);
	return NULL;
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	const char* text = (const char*)data;
	rvn_patch_t patch;
	const uint8_t* zero;
	size_t split;
	char* left;
	char* right;
	char* written;
	size_t written_size = 0;

	memset(&patch, 0, sizeof(patch));
	if (rvn_patch_parse(text, size, &patch)) fuzz_check_patch(&patch, text, size);
	rvn_patch_free(&patch);

	/* the two texts (C strings: up to a 0 byte) */
	zero = size ? (const uint8_t*)memchr(data, 0, size) : NULL;
	split = zero ? (size_t)(zero - data) : size / 2;
	left = (char*)malloc(split + 1);
	memcpy(left, data, split);
	left[split] = '\0';
	right = (char*)malloc(size - split + 1);
	memcpy(right, data + split + (zero ? 1 : 0), size - split - (zero ? 1 : 0));
	right[size - split - (zero ? 1 : 0)] = '\0';

	(void)Rdiff_count_changes(left, right, (uint32_t)(size % 8));
	written = Rdiff_unified(left, right, "a/f", "b/f", (int)(size % 5), 0, &written_size);
	FUZZ_CHECK(written != NULL && written_size == strlen(written));
	if (written[0]) {
		char* applied;
		memset(&patch, 0, sizeof(patch));
		FUZZ_CHECK(rvn_patch_parse(written, written_size, &patch));
		FUZZ_CHECK(patch.file_count == 1);
		fuzz_check_patch(&patch, written, written_size);
		applied = fuzz_apply(left, &patch);
		FUZZ_CHECK(applied != NULL);
		FUZZ_CHECK(strcmp(applied, right) == 0);
		free(applied);
		rvn_patch_free(&patch);
	} else {
		FUZZ_CHECK(strcmp(left, right) == 0);
	}
	Rmemory_free(written);
	free(left);
	free(right);
	return 0;
}
