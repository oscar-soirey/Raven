/**
 * Raven tests - formatted text: Markdown in labels and in the text editor,
 * and the styles of the fonts they use.
 */

#include "test.h"

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

/* --------------------------------------------------------------------------
 *  Formulas: a $...$ in a line, a $$...$$ of its own, drawn by the application
 * ----------------------------------------------------------------------- */

/* What the resolver saw, and what it answers. The picture is a texture the test
 * makes once; the box it gives is one em tall unless the test says otherwise. */
static int g_math_calls;
static char g_math_source[256];
static float g_math_size;
static float g_math_width = 3.0f;
static float g_math_tall = 1.0f;
static int g_math_fail;
static int g_math_display;
static Rid g_math_texture;

static int on_math(Rid label, const char* source, int length, float size, int display, RMathBox* box, void* user_data) {
	(void)label;
	(void)user_data;
	g_math_calls++;
	if (length > (int)sizeof(g_math_source) - 1) length = (int)sizeof(g_math_source) - 1;
	if (length > 0) memcpy(g_math_source, source, (size_t)length);
	g_math_source[length > 0 ? length : 0] = 0;
	g_math_size = size;
	g_math_display = display;
	if (g_math_fail) return R_FALSE;
	box->texture = g_math_texture;
	box->width = g_math_width;
	/* (the baseline sits four fifths of the way down the box, as an engine reports it) */
	box->ascent = g_math_tall * 0.8f;
	box->descent = g_math_tall * 0.2f;
	return R_TRUE;
}

static void math_reset(void) {
	g_math_calls = 0;
	g_math_source[0] = 0;
	g_math_size = 0.0f;
	g_math_width = 3.0f;
	g_math_tall = 1.0f;
	g_math_fail = 0;
	g_math_display = R_FALSE;
}

/* A Markdown label whose formulas `on_math` draws, laid out and drawn. */
static Rid math_label(const char* text) {
	Rid label = markdown_label(text);
	Rlabel_set_math_callback(label, on_math, NULL);
	Rdraw_frame();
	return label;
}

static void test_math_syntax(void) {
	Rid label;

	math_reset();
	label = math_label("The area is $x^2$ exactly.");
	CHECK(g_math_calls == 1);
	CHECK_STR(g_math_source, "x^2");
	CHECK_NEAR(g_math_size, 13.0);
	CHECK(g_math_display == R_FALSE);
	/* (copying gives the marks back, so it re-renders elsewhere) */
	CHECK_STR(Rlabel_get_plain_text(label), "The area is $x^2$ exactly.");

	/* money and a pair of dollars in prose are not formulas */
	math_reset();
	math_label("it costs $5 and $10, and 3 dollars");
	CHECK(g_math_calls == 0);
	math_reset();
	math_label("a $ b $ c");
	CHECK(g_math_calls == 0);
	math_reset();
	math_label("a $x $ c");
	CHECK(g_math_calls == 0);
	/* a code span and a fence win over the marks: a formula shown as code stays code */
	math_reset();
	math_label("a `$x^2$` b");
	CHECK(g_math_calls == 0);
	math_reset();
	math_label("```\n$x^2$\n```");
	CHECK(g_math_calls == 0);
	/* an escaped dollar is a dollar */
	math_reset();
	label = math_label("a \\$x\\$ b");
	CHECK(g_math_calls == 0);
	CHECK_STR(Rlabel_get_plain_text(label), "a $x$ b");
	/* an open formula is text until its closer arrives */
	math_reset();
	math_label("a $x^2 b");
	CHECK(g_math_calls == 0);
	/* the other pair of delimiters is prose, not marks (and copies as the prose it reads as) */
	math_reset();
	label = math_label("\\(x^2\\) and \\[y\\]");
	CHECK(g_math_calls == 0);
	CHECK_STR(Rlabel_get_plain_text(label), "(x^2) and [y]");
	/* a formula keeps no emphasis of the text, and a TeX star is not a mark */
	math_reset();
	math_label("$x_1 * y_2$");
	CHECK(g_math_calls == 1);
	CHECK_STR(g_math_source, "x_1 * y_2");
	/* a formula inside a link label still has its link */
	math_reset();
	label = math_label("[see $x$](https://example.com)");
	CHECK(g_math_calls == 1);
	CHECK_STR(g_math_source, "x");
	CHECK_STR(Rlabel_get_plain_text(label), "see $x$");
}

static void test_math_display(void) {
	Rid label;
	float line, tall;

	math_reset();
	line = height_of(markdown_label("prose"));
	label = math_label("prose\n\n$$\\int_0^1 x^2 dx$$");
	CHECK(g_math_calls == 1);
	CHECK_STR(g_math_source, "\\int_0^1 x^2 dx");
	CHECK(g_math_display == R_TRUE);
	CHECK_STR(Rlabel_get_plain_text(label), "prose\n$$\\int_0^1 x^2 dx$$");
	/* a display formula has vertical space of its own above it */
	tall = height_of(label);
	CHECK(tall > line + 8.0f);
	/* surrounded by words it only breaks the lines it is on, and keeps its marks when copied */
	math_reset();
	label = math_label("before $$x^2$$ after");
	CHECK(g_math_calls == 1);
	CHECK(g_math_display == R_TRUE);
	CHECK_STR(Rlabel_get_plain_text(label), "before \n$$x^2$$\n after");
	/* $$ inside the paragraph, not alone, is not a block: three lines, not a paragraph */
	CHECK(height_of(label) > line * 2.0f);
}

static void test_math_layout(void) {
	Rid plain_label, label;
	float line, with_formula;

	line = height_of(markdown_label("aaa bbb"));

	/* a tall formula raises the line, and the words keep its baseline */
	math_reset();
	g_math_tall = 2.2f;
	label = math_label("aaa $x$ bbb");
	with_formula = height_of(label);
	CHECK(g_math_calls == 1);
	CHECK(with_formula > line + 8.0f);
	Rwidget_destroy(label);

	/* a formula that does not fit is moved to the next line whole */
	math_reset();
	g_math_width = 3.0f;
	plain_label = math_label("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa $x$");
	math_reset();
	g_math_width = 30.0f;
	Rlabel_set_math_callback(plain_label, NULL, NULL);
	Rlabel_set_math_callback(plain_label, on_math, NULL);
	Rdraw_frame();
	with_formula = height_of(plain_label);
	math_reset();
	g_math_width = 3.0f;
	Rlabel_set_math_callback(plain_label, NULL, NULL);
	Rlabel_set_math_callback(plain_label, on_math, NULL);
	Rdraw_frame();
	CHECK(with_formula > height_of(plain_label) + 8.0f);
	Rwidget_destroy(plain_label);

	/* a formula wider than the column is asked for again, smaller, at half at the floor */
	math_reset();
	g_math_width = 60.0f;
	label = math_label("$x^2$");
	CHECK(g_math_calls == 2);
	CHECK(g_math_size > 6.0f && g_math_size < 13.0f);
	Rwidget_destroy(label);
	math_reset();
	g_math_width = 400.0f;
	label = math_label("$x^2$");
	CHECK(g_math_calls == 2);
	CHECK_NEAR(g_math_size, 6.5);
	Rwidget_destroy(label);

	/* the pictures are kept: a second layout asks for nothing, and an invalidation asks again */
	math_reset();
	label = math_label("$x$");
	CHECK(g_math_calls == 1);
	Rdraw_frame();
	Rdraw_frame();
	CHECK(g_math_calls == 1);
	Rlabel_invalidate_math(label);
	Rdraw_frame();
	CHECK(g_math_calls == 2);
	/* a new text is a new parse, and the formulas in it are asked for again */
	Rlabel_set_text(label, "$y$");
	Rdraw_frame();
	CHECK(g_math_calls == 3);
	CHECK_STR(g_math_source, "y");
	Rwidget_destroy(label);
}

static void test_math_failure(void) {
	Rid label, failed;
	float line;

	/* a formula the application cannot draw is shown as its source, in the flow */
	math_reset();
	g_math_fail = 1;
	failed = math_label("the root $\\notacommand{x}$ of it");
	CHECK(g_math_calls == 1);
	CHECK_STR(Rlabel_get_plain_text(failed), "the root $\\notacommand{x}$ of it");
	line = height_of(failed);
	/* (it reads as the sentence it is: no room is reserved for a picture) */
	CHECK(line > 10.0f && line < 60.0f);

	/* the same formula, drawn: the same label grows, and asks again when told to */
	g_math_fail = 0;
	g_math_tall = 2.2f;
	Rlabel_invalidate_math(failed);
	Rdraw_frame();
	CHECK(g_math_calls == 2);
	CHECK(height_of(failed) > line + 4.0f);
	/* a label that is not Markdown does not read the marks at all */
	label = markdown_label("not Markdown $x$");
	Rlabel_set_math_callback(label, on_math, NULL);
	Rlabel_set_markdown(label, R_FALSE);
	Rdraw_frame();
	CHECK(g_math_calls == 2);
	CHECK_STR(Rlabel_get_plain_text(label), "not Markdown $x$");
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

int main(void) {
	if (!test_init(0)) {
		printf("test_markdown: cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("Markdown", 400, 300, R_WINDOW_DEFAULT);
	/* a picture for the formulas: white, one pixel per texel, never drawn headless */
	{
		static unsigned char pixels[8 * 8 * 4];
		memset(pixels, 255, sizeof(pixels));
		g_math_texture = Rtexture_create_from_pixels(pixels, 8, 8);
	}
	RUN(test_inline);
	RUN(test_blocks);
	RUN(test_links);
	RUN(test_math_syntax);
	RUN(test_math_display);
	RUN(test_math_layout);
	RUN(test_math_failure);
	RUN(test_fonts);
	RUN(test_editor);
	Rwindow_destroy(g_window);
	Rshutdown();
	return test_report("test_markdown");
}
