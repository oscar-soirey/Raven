/**
 * Raven tests - formatted text: Markdown in labels and in the text editor,
 * and the styles of the fonts they use.
 */

#include "test.h"

#include <stdlib.h>

static int g_links;
static char g_link[256];
static int g_actions;
static char g_action[64];

static void on_link(Rid label, const char* url, void* user_data) {
	(void)label;
	(void)user_data;
	g_links++;
	snprintf(g_link, sizeof(g_link), "%s", url);
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	g_actions++;
	snprintf(g_action, sizeof(g_action), "%s", action);
}

static Rid g_window;

/* A Markdown label alone in a window 400 wide, laid out. */
static Rid markdown_label(const char* text) {
	Rid label = Rlabel_create(text);
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rlabel_set_markdown(label, R_TRUE);
	Rslot_set_valign(label, R_VALIGN_TOP);
	Rwidget_add_child(box, label);
	Rwindow_set_content(g_window, box);
	Rdraw_frame();
	return label;
}

static const char* plain(const char* text) {
	return Rlabel_get_plain_text(markdown_label(text));
}

static float height_of(Rid widget) {
	float x, y, w, h;
	Rwidget_get_rect(widget, &x, &y, &w, &h);
	return h;
}

static void test_inline(void) {
	CHECK_STR(plain("Hello **strong** and *soft* text"), "Hello strong and soft text");
	CHECK_STR(plain("__strong__ _soft_ ***both*** ~~gone~~"), "strong soft both gone");
	CHECK_STR(plain("a `code *not soft*` b"), "a code *not soft* b");
	CHECK_STR(plain("``a ` tick``"), "a ` tick");
	/* marks that open nothing are text */
	CHECK_STR(plain("2 * 3 * 4 and snake_case_name and a * alone"), "2 * 3 * 4 and snake_case_name and a * alone");
	CHECK_STR(plain("**never closed"), "**never closed");
	CHECK_STR(plain("escaped \\*stars\\* and \\_bars\\_"), "escaped *stars* and _bars_");
	CHECK_STR(plain("AT&amp;T &lt;tag&gt; &#65;&#x42; &copy; &unknown;"), "AT&T <tag> AB \xC2\xA9 &unknown;");
	/* lines of a paragraph are one line; two spaces or a backslash break it */
	CHECK_STR(plain("one\ntwo\n   three"), "one two three");
	CHECK_STR(plain("one  \ntwo\\\nthree"), "one\ntwo\nthree");
	CHECK_STR(plain("a [link](https://example.com/a_b) b"), "a link b");
	CHECK_STR(plain("see <https://example.com> or https://example.org/x."), "see https://example.com or https://example.org/x.");
	CHECK_STR(plain("![picture of a bird](missing.png) in a line"), "picture of a bird in a line");
	CHECK_STR(plain(""), "");
	/* not Markdown: the text as it is */
	{
		Rid label = Rlabel_create("**as is**");
		CHECK(!Rlabel_get_markdown(label));
		CHECK_STR(Rlabel_get_plain_text(label), "**as is**");
		Rwidget_destroy(label);
	}
}

static void test_blocks(void) {
	Rid label;
	float one_line, h;

	CHECK_STR(plain("# Title\n\nText under it.\n\n## Second ##\nMore"), "Title\nText under it.\nSecond\nMore");
	CHECK_STR(plain("Title\n=====\n\nSub\n---\ntext"), "Title\nSub\ntext");
	CHECK_STR(plain("- one\n- two\n  - inner\n- [x] done\n- [ ] todo\n\n1. first\n2. second"), "one\ntwo\ninner\ndone\ntodo\nfirst\nsecond");
	CHECK_STR(plain("> quoted *text*\n> on two lines\n\nafter"), "quoted text on two lines\nafter");
	CHECK_STR(plain("```c\nint a = 1;\n  indented();\n```\nafter"), "int a = 1;\n  indented();\nafter");
	CHECK_STR(plain("~~~\n``` not a fence\n~~~"), "``` not a fence");
	CHECK_STR(plain("a\n\n---\n\nb"), "a\n\nb");
	CHECK_STR(plain("| A | B |\n|---|:-:|\n| 1 | **2** |\n| 3 | 4 |\n\nend"), "A\tB\n1\t2\n3\t4\nend");
	/* a table of more rows than 16 bits can count keeps them all */
	{
		const int rows = 65537;
		size_t length = 0;
		int r, lines = 1;
		char* text = (char*)malloc((size_t)rows * 6 + 16);
		const char* out;
		memcpy(text, "| h |\n|---|\n", 12);
		length = 12;
		for (r = 1; r < rows; r++) {
			memcpy(text + length, "| r |\n", 6);
			length += 6;
		}
		text[length] = 0;
		label = markdown_label(text);
		out = Rlabel_get_plain_text(label);
		for (; *out; out++) lines += *out == '\n';
		CHECK(lines == rows);
		CHECK(height_of(label) > 100000.0f);
		free(text);
	}
	/* indented with the XML element it is written in */
	CHECK_STR(plain("\n\t\t# Title\n\n\t\t- item\n\t\t  - inner\n\t"), "Title\nitem\ninner");

	/* sizes: a title is taller than a line, a paragraph wraps on the width of the label */
	label = markdown_label("one line");
	one_line = height_of(label);
	CHECK(one_line > 10.0f && one_line < 30.0f);
	label = markdown_label("# Title");
	CHECK(height_of(label) > one_line * 1.4f);
	label = markdown_label("word word word word word word word word word word word word word word word word word word word word word word word word word word word word");
	h = height_of(label);
	CHECK(h > one_line * 1.8f);
	{
		float x, y, w, hh;
		Rwidget_get_rect(label, &x, &y, &w, &hh);
		CHECK(w <= 400.5f);
	}
	/* ... unless it was told not to */
	Rlabel_set_wrap(label, R_FALSE);
	Rdraw_frame();
	CHECK_NEAR(height_of(label), one_line);
	label = markdown_label("a\n\nb\n\nc");
	CHECK(height_of(label) > one_line * 3.0f);
	label = markdown_label("```\n1\n2\n3\n4\n```");
	CHECK(height_of(label) > one_line * 4.0f);
	/* the text changes: so does the layout */
	Rlabel_set_text(label, "short");
	Rdraw_frame();
	CHECK_NEAR(height_of(label), one_line);
	CHECK_STR(Rlabel_get_plain_text(label), "short");
	/* turned off: the marks show again */
	Rlabel_set_text(label, "# T");
	Rlabel_set_markdown(label, R_FALSE);
	Rdraw_frame();
	CHECK_STR(Rlabel_get_plain_text(label), "# T");
	Rwidget_set_attribute(label, "Markdown", "true");
	CHECK(Rlabel_get_markdown(label));
	CHECK_STR(Rlabel_get_plain_text(label), "T");

	/* a long document, and odd ones, in every theme */
	{
		static const char* const documents[] = {
			"# T\n\n> - a\n> - b\n>\n> ```\n> code\n> ```\n\n| a |\n|---|\n\n1. x\n   continued\n\n   new paragraph\n2. y",
			"***\n___\n- \n-\n#\n####### seven\n|\n||\n|-|\n```",
			"[a](b) [c] (d) [e]() ![](x) <> < > <a@b.c> &; & &#; &#xZZ; \\",
			"* * *\n*a\n_\n__\n** **\n*** x ***\n`\n``\n~~~",
			"| a | b | c |\n|---|---|\n| 1 |\n| 1 | 2 | 3 | 4 | 5 |",
			"deep\n- 1\n  - 2\n    - 3\n      - 4\n        - 5\n          - 6\n            - 7\n              - 8\n                - 9\n                  - 10",
			"> 1\n>> 2\n>>> 3\n>>>> 4\n>>>>> 5\n>>>>>> 6\n>>>>>>> 7\n>>>>>>>> 8\n>>>>>>>>> 9\n>>>>>>>>>> 10"
		};
		static const RTheme themes[] = { R_THEME_DARK, R_THEME_LIGHT, R_THEME_GLASS_DARK, R_THEME_GLASS_LIGHT };
		size_t d, t;
		for (t = 0; t < sizeof(themes) / sizeof(themes[0]); t++) {
			Rtheme_apply(themes[t]);
			for (d = 0; d < sizeof(documents) / sizeof(documents[0]); d++) {
				label = markdown_label(documents[d]);
				CHECK(height_of(label) > 0.0f);
				Rwindow_set_size(g_window, 120, 300);
				Rdraw_frame();
				Rwindow_set_size(g_window, 400, 300);
				Rdraw_frame();
			}
		}
		Rtheme_apply(R_THEME_DARK);
	}
}

static void test_links(void) {
	Rid label = markdown_label("Go to [the site](https://example.com/page) or [save](action:file.save), or stay.");
	float x, y, w, h;

	Rwidget_get_rect(label, &x, &y, &w, &h);
	Rlabel_set_link_callback(label, on_link, NULL);
	g_links = 0;
	/* the first link starts after "Go to ": about 40 units in */
	Rwindow_inject_mouse_move(g_window, x + 4.0f, y + h * 0.5f);
	CHECK(Rwindow_get_hovered_widget(g_window) == label);
	test_click_at(g_window, x + 4.0f, y + h * 0.5f);
	CHECK(g_links == 0);
	{
		/* walks along the line until a click follows a link */
		float at;
		for (at = 10.0f; at < 200.0f && g_links == 0; at += 6.0f) test_click_at(g_window, x + at, y + h * 0.5f);
		CHECK(g_links == 1);
		CHECK_STR(g_link, "https://example.com/page");
		for (; at < 380.0f && strcmp(g_link, "action:file.save") != 0; at += 6.0f) test_click_at(g_window, x + at, y + h * 0.5f);
		CHECK_STR(g_link, "action:file.save");
		/* without a callback, an action link fires its action */
		Rlabel_set_link_callback(label, NULL, NULL);
		Raction_bind("file.save", on_action, NULL);
		g_actions = 0;
		test_click_at(g_window, x + at - 6.0f, y + h * 0.5f);
		CHECK(g_actions == 1);
		CHECK_STR(g_action, "file.save");
		Raction_unbind("file.save");
	}
	/* a label without links lets the mouse through, as labels do */
	label = markdown_label("no link *here*");
	Rwidget_get_rect(label, &x, &y, &w, &h);
	Rwindow_inject_mouse_move(g_window, x + 4.0f, y + h * 0.5f);
	CHECK(Rwindow_get_hovered_widget(g_window) != label);
}

static void test_fonts(void) {
	Rid font = Rfont_get_default();
	Rid bold = Rfont_get_variant(font, R_FONT_BOLD);
	Rid italic = Rfont_get_variant(font, R_FONT_ITALIC);
	Rid both = Rfont_get_variant(font, R_FONT_BOLD_ITALIC);
	float regular_width, bold_width, height, bold_height;

	CHECK(Rfont_get_variant(font, R_FONT_REGULAR) == font);
	CHECK(Rfont_is_valid(bold) && Rfont_is_valid(italic) && Rfont_is_valid(both));
	CHECK(bold != font && italic != font && both != bold && both != italic);
	/* asked again: the same ones; the bold of the italic is the bold italic */
	CHECK(Rfont_get_variant(font, R_FONT_BOLD) == bold);
	CHECK(Rfont_get_variant(italic, R_FONT_BOLD) == both);
	CHECK(Rfont_get_variant(bold, R_FONT_REGULAR) == bold);
	/* the styles Raven derives take the same room: text does not move when its style changes */
	Rfont_measure_text(font, 14.0f, "The quick brown fox", &regular_width, &height);
	Rfont_measure_text(bold, 14.0f, "The quick brown fox", &bold_width, &bold_height);
	CHECK_NEAR(regular_width, bold_width);
	CHECK_NEAR(height, bold_height);

	/* a real font for a style */
	Rfont_set_variant(font, R_FONT_BOLD, Rfont_get_monospace());
	CHECK(Rfont_get_variant(font, R_FONT_BOLD) == Rfont_get_monospace());
	CHECK(!Rfont_is_valid(bold));
	Rfont_set_variant(font, R_FONT_BOLD, R_INVALID_ID);
	bold = Rfont_get_variant(font, R_FONT_BOLD);
	CHECK(Rfont_is_valid(bold) && bold != Rfont_get_monospace());

	EXPECT_ERRORS_BEGIN();
	Rfont_destroy(italic);
	Rfont_set_variant(font, R_FONT_REGULAR, bold);
	Rfont_set_variant(font, R_FONT_BOLD, font);
	Rfont_set_variant(italic, R_FONT_BOLD, font);
	CHECK(Rfont_get_variant(font, (RFontStyle)0) == R_INVALID_ID);
	EXPECT_ERRORS_END(5);
	CHECK(Rfont_is_valid(italic));
}

static void test_editor(void) {
	Rid editor = Rwidget_create(R_WIDGET_TEXT_EDITOR);
	Rwindow_set_content(g_window, editor);
	CHECK(!Rtext_editor_get_markdown(editor));
	Rtext_editor_set_markdown(editor, R_TRUE);
	CHECK(Rtext_editor_get_markdown(editor));
	CHECK_STR(Rtext_editor_get_language(editor), "Markdown");
	CHECK_STR(Rlanguage_for_file("notes/README.md"), "Markdown");
	Rtext_editor_set_text(editor,
		"# Title\n"
		"plain **strong** and *soft* `code` [link](url) ~~gone~~ 2 * 3\n"
		"- item\n"
		"```c\n"
		"int *a = *b;\n"
		"```\n"
		"after *the* block\n");
	Rdraw_frame();
	CHECK(Rtext_editor_get_token(editor, 0, 0) == R_TOKEN_HEADING);
	CHECK(Rtext_editor_get_token(editor, 0, 4) == R_TOKEN_HEADING);
	CHECK(Rtext_editor_get_token(editor, 1, 0) == R_TOKEN_TEXT);
	CHECK(Rtext_editor_get_token(editor, 1, 6) == R_TOKEN_STRONG);
	CHECK(Rtext_editor_get_token(editor, 1, 10) == R_TOKEN_STRONG);
	CHECK(Rtext_editor_get_token(editor, 1, 16) == R_TOKEN_TEXT);
	CHECK(Rtext_editor_get_token(editor, 1, 21) == R_TOKEN_EMPHASIS);
	CHECK(Rtext_editor_get_token(editor, 1, 28) == R_TOKEN_STRING);
	CHECK(Rtext_editor_get_token(editor, 1, 35) == R_TOKEN_LINK);
	CHECK(Rtext_editor_get_token(editor, 1, 47) == R_TOKEN_COMMENT);
	/* a star alone is a star */
	CHECK(Rtext_editor_get_token(editor, 1, 58) == R_TOKEN_TEXT);
	CHECK(Rtext_editor_get_token(editor, 2, 0) == R_TOKEN_KEYWORD);
	CHECK(Rtext_editor_get_token(editor, 2, 3) == R_TOKEN_TEXT);
	CHECK(Rtext_editor_get_token(editor, 3, 0) == R_TOKEN_PUNCTUATION);
	/* inside a block of code nothing is formatted */
	CHECK(Rtext_editor_get_token(editor, 4, 5) == R_TOKEN_STRING);
	CHECK(Rtext_editor_get_token(editor, 5, 1) == R_TOKEN_PUNCTUATION);
	CHECK(Rtext_editor_get_token(editor, 6, 7) == R_TOKEN_EMPHASIS);
	/* typing a fence changes what follows */
	Rtext_editor_set_text(editor, "```\n*a*\n");
	Rdraw_frame();
	CHECK(Rtext_editor_get_token(editor, 1, 1) == R_TOKEN_STRING);
	Rtext_editor_set_markdown(editor, R_FALSE);
	CHECK_STR(Rtext_editor_get_language(editor), "");
	Rwidget_set_attribute(editor, "Markdown", "true");
	CHECK(Rtext_editor_get_markdown(editor));
	Rdraw_frame();
}

/*
 * Marks that close nothing ("*a *a *a ..."): each one looked for a closer to
 * the end of the text, so a long line took a time that grows with its square
 * (seconds for these 90 KB, on every frame in the editor). The text is
 * still the text, and both stay fast.
 */
static void test_unclosed_marks(void) {
	static const char* const units[] = { "**a ", "__a ", "~~a ", "*a ", "_a " };
	const int repeat = 5000;
	size_t u, length = 0, unit_length;
	char* text = (char*)malloc((size_t)repeat * 5 * 4 + 1);
	Rid label, editor;
	double start;
	int r;
	for (u = 0; u < sizeof(units) / sizeof(units[0]); u++) {
		unit_length = strlen(units[u]);
		for (r = 0; r < repeat; r++) {
			memcpy(text + length, units[u], unit_length);
			length += unit_length;
		}
	}
	text[length - 1] = 0;

	start = Rget_time();
	label = markdown_label(text);
	CHECK_STR(Rlabel_get_plain_text(label), text);
	editor = Rwidget_create(R_WIDGET_TEXT_EDITOR);
	Rtext_editor_set_markdown(editor, R_TRUE);
	Rwindow_set_content(g_window, editor);
	Rtext_editor_set_text(editor, text);
	Rdraw_frame();
	Rtext_editor_insert_text(editor, "x");
	Rdraw_frame();
	CHECK(Rtext_editor_get_token(editor, 0, 1) == R_TOKEN_TEXT);
	CHECK(Rtext_editor_get_token(editor, 0, (int)length - 3) == R_TOKEN_TEXT);
	CHECK(Rget_time() - start < 2.0);
	free(text);
}

int main(void) {
	if (!test_init(0)) {
		printf("test_markdown: cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("Markdown", 400, 300, R_WINDOW_DEFAULT);
	RUN(test_inline);
	RUN(test_blocks);
	RUN(test_links);
	RUN(test_fonts);
	RUN(test_editor);
	RUN(test_unclosed_marks);
	Rwindow_destroy(g_window);
	Rshutdown();
	return test_report("test_markdown");
}
