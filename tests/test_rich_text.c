/**
 * Raven tests - RichTextEditor: styled text edited.
 *
 * The document (insertions and deletions across runs and paragraphs, looks
 * on part of a selection, runs that look the same merged), Markdown read and
 * written back to the same document, HTML written, HTML pasted and cleaned,
 * the keys (Ctrl+B, I, U, E, K, Shift+X; headings and lists), Markdown typed
 * ("# ", "- ", **bold**...), lists (Tab, Shift+Tab, Enter, Backspace), the
 * history and the selection it gives back, the composition of an input
 * method, the clipboard, the toolbar, links followed, XML, accessibility,
 * and a document of 5 000 paragraphs that stays quick to type in and scroll.
 */

#include "test.h"

#include <stdlib.h>
#include <time.h>

static Rid g_window;
static Rid g_editor;
static int g_changes;
static int g_selections;
static char g_followed[256];

#define W 700.0f
#define H 480.0f

static void on_changed(Rid editor, void* user_data) {
	(void)editor;
	(void)user_data;
	g_changes++;
}

static void on_selection(Rid editor, void* user_data) {
	(void)editor;
	(void)user_data;
	g_selections++;
}

static void on_link(Rid editor, const char* url, void* user_data) {
	(void)editor;
	(void)user_data;
	snprintf(g_followed, sizeof(g_followed), "%s", url);
}

static Rid open_editor(const char* markdown) {
	g_editor = Rrich_text_editor_create();
	Rrich_text_editor_set_markdown(g_editor, markdown);
	Rrich_text_editor_set_changed_callback(g_editor, on_changed, NULL);
	Rrich_text_editor_set_selection_callback(g_editor, on_selection, NULL);
	Rrich_text_editor_set_link_callback(g_editor, on_link, NULL);
	Rwindow_set_content(g_window, g_editor);
	Rwidget_focus(g_editor);
	Rwindow_inject_mouse_move(g_window, -50.0f, -50.0f);
	Rdraw_frame();
	g_changes = 0;
	g_selections = 0;
	g_followed[0] = '\0';
	return g_editor;
}

static void key(RKey k, Ruint modifiers) {
	test_key(g_window, k, modifiers);
}

static void type(const char* text) {
	Rwindow_inject_text(g_window, text);
}

static void select_range(int ap, int ao, int cp, int co) {
	Rrich_text_editor_set_selection(g_editor, ap, ao, cp, co);
}

static const char* md(void) {
	return Rrich_text_editor_get_markdown(g_editor);
}

static char* copy(const char* text) {
	size_t length = strlen(text);
	char* out = (char*)malloc(length + 1);
	memcpy(out, text, length + 1);
	return out;
}

/* Markdown read, written, read again: the same document. */
static void check_round_trip(const char* source, const char* expected) {
	char* first;
	Rrich_text_editor_set_markdown(g_editor, source);
	first = copy(md());
	if (expected) CHECK_STR(first, expected);
	Rrich_text_editor_set_markdown(g_editor, first);
	CHECK_STR(md(), first);
	/* and through HTML */
	{
		char* html = copy(Rrich_text_editor_get_html(g_editor));
		Rrich_text_editor_set_html(g_editor, html);
		CHECK_STR(md(), first);
		if (strcmp(md(), first) != 0) printf("    html: %s\n", html);
		free(html);
	}
	free(first);
}

/* ---- the document ---- */

static void test_model(void) {
	open_editor("Hello **bold** world");
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 1);
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "Hello bold world");
	CHECK(Rrich_text_editor_get_style_at(g_editor, 0, 0) == 0);
	CHECK(Rrich_text_editor_get_style_at(g_editor, 0, 6) == R_RICH_TEXT_BOLD);
	CHECK(Rrich_text_editor_get_style_at(g_editor, 0, 9) == R_RICH_TEXT_BOLD);
	CHECK(Rrich_text_editor_get_style_at(g_editor, 0, 10) == 0);

	/* typed inside the bold run: bold; at its end: bold too (the character before) */
	select_range(0, 8, 0, 8);
	Rrich_text_editor_insert_text(g_editor, "XX");
	CHECK_STR(md(), "Hello **boXXld** world\n");
	CHECK(g_changes == 1);

	/* deleted across runs */
	select_range(0, 3, 0, 9);
	Rrich_text_editor_insert_text(g_editor, "p");
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "HelpXld world");
	/* (the text typed over a selection takes the looks of its first character: plain) */
	CHECK_STR(md(), "Help**Xld** world\n");

	/* looks on part of a selection, then over all of it: merged into one run */
	Rrich_text_editor_set_markdown(g_editor, "one two three");
	select_range(0, 0, 0, 3);
	Rrich_text_editor_toggle_style(g_editor, R_RICH_TEXT_ITALIC);
	select_range(0, 4, 0, 7);
	Rrich_text_editor_toggle_style(g_editor, R_RICH_TEXT_ITALIC);
	CHECK_STR(md(), "*one* *two* three\n");
	select_range(0, 0, 0, 7);
	CHECK(Rrich_text_editor_get_style(g_editor) == 0);
	Rrich_text_editor_toggle_style(g_editor, R_RICH_TEXT_ITALIC);
	CHECK_STR(md(), "*one two* three\n");
	CHECK(Rrich_text_editor_get_style(g_editor) == R_RICH_TEXT_ITALIC);
	/* toggled again: off over all of it */
	Rrich_text_editor_toggle_style(g_editor, R_RICH_TEXT_ITALIC);
	CHECK_STR(md(), "one two three\n");
	/* bold on a part, then over more of it: one run */
	select_range(0, 2, 0, 5);
	Rrich_text_editor_set_style(g_editor, R_RICH_TEXT_BOLD, R_TRUE);
	select_range(0, 4, 0, 9);
	Rrich_text_editor_set_style(g_editor, R_RICH_TEXT_BOLD, R_TRUE);
	CHECK_STR(md(), "on**e two t**hree\n");

	/* across paragraphs: deleted, joined */
	Rrich_text_editor_set_markdown(g_editor, "# Title\n\nfirst *para*\n\nsecond **para**\n\nthird");
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 4);
	select_range(1, 6, 2, 7);
	Rrich_text_editor_insert_text(g_editor, "");
	key(R_KEY_DELETE, 0);
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 3);
	CHECK_STR(md(), "# Title\n\nfirst **para**\n\nthird\n");
	/* a style over several paragraphs */
	select_range(0, 2, 2, 3);
	Rrich_text_editor_set_style(g_editor, R_RICH_TEXT_STRIKETHROUGH, R_TRUE);
	CHECK_STR(md(), "# Ti~~tle~~\n\n~~first **para**~~\n\n~~thi~~rd\n");
	CHECK(Rrich_text_editor_get_style(g_editor) == R_RICH_TEXT_STRIKETHROUGH);

	/* text with lines: paragraphs */
	Rrich_text_editor_set_markdown(g_editor, "ab");
	select_range(0, 1, 0, 1);
	Rrich_text_editor_insert_text(g_editor, "1\n2\n3");
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 3);
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "a1\n2\n3b");
	{
		int ap, ao, cp, co;
		Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
		CHECK(cp == 2 && co == 1 && ap == 2 && ao == 1);
	}
	/* plain text in and out */
	Rrich_text_editor_set_text(g_editor, "plain *not bold*\nline two");
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 2);
	CHECK_STR(md(), "plain \\*not bold\\*\n\nline two\n");
	CHECK_STR(Rrich_text_editor_get_paragraph_text(g_editor, 1), "line two");
	CHECK_STR(Rrich_text_editor_get_paragraph_text(g_editor, 9), "");
	/* an empty document is an empty text */
	Rrich_text_editor_set_markdown(g_editor, "");
	CHECK_STR(md(), "");
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 1);
}

static void test_markdown(void) {
	open_editor("");
	check_round_trip("# One\n\n## Two\n\n### Three\n\ntext", "# One\n\n## Two\n\n### Three\n\ntext\n");
	check_round_trip("**bold** *italic* ***both*** ~~struck~~ <u>under</u> `code` [link](https://example.com/a_b)",
		"**bold** *italic* ***both*** ~~struck~~ <u>under</u> `code` [link](https://example.com/a_b)\n");
	check_round_trip("a **bold *and italic* run** end", "a **bold *and italic* run** end\n");
	check_round_trip("[**bold link** and more](http://x.org)", "[**bold link** and more](http://x.org)\n");
	check_round_trip("- one\n- two\n  - nested\n    - deeper\n- three", "- one\n- two\n  - nested\n    - deeper\n- three\n");
	check_round_trip("1. one\n2. two\n   1. inner\n   2. inner two\n3. three", "1. one\n2. two\n   1. inner\n   2. inner two\n3. three\n");
	check_round_trip("> quoted\n>\n> **second**", "> quoted\n>\n> **second**\n");
	check_round_trip("```\nint x = 1;\n\n  indented\n```", "```\nint x = 1;\n\n  indented\n```\n");
	check_round_trip("text\n\n&nbsp;\n\nafter an empty paragraph", "text\n\n&nbsp;\n\nafter an empty paragraph\n");
	/* what would be read as marks is escaped */
	check_round_trip("\\# not a title, 2\\*3\\*4, \\[x\\], \\_y\\_, a\\|b, 1\\. no list", NULL);
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 1);
	CHECK(Rrich_text_editor_get_paragraph_block(g_editor, 0) == R_RICH_TEXT_PARAGRAPH);
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "# not a title, 2*3*4, [x], _y_, a|b, 1. no list");
	/* a line break in a paragraph, a list item, a quote */
	check_round_trip("one\\\ntwo\n\n- item\\\n  more\n\n> q\\\n> r", "one\\\ntwo\n\n- item\\\n  more\n\n> q\\\n> r\n");
	CHECK_STR(Rrich_text_editor_get_paragraph_text(g_editor, 0), "one\ntwo");
	/* code with backquotes; a fence longer than the ones inside */
	check_round_trip("``a ` b``", "``a ` b``\n");
	check_round_trip("````\n```\n````", "````\n```\n````\n");
	/* looks against spaces are written outside the marks */
	Rrich_text_editor_set_text(g_editor, "a bold word");
	select_range(0, 1, 0, 7);
	Rrich_text_editor_toggle_style(g_editor, R_RICH_TEXT_BOLD);
	CHECK_STR(md(), "a **bold** word\n");
	/* the blocks */
	Rrich_text_editor_set_markdown(g_editor, "# T\n\n- a\n  1. b\n\n> c\n\n```\nd\n```");
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 5);
	CHECK(Rrich_text_editor_get_paragraph_block(g_editor, 0) == R_RICH_TEXT_HEADING_1);
	CHECK(Rrich_text_editor_get_paragraph_block(g_editor, 1) == R_RICH_TEXT_BULLET_LIST);
	CHECK(Rrich_text_editor_get_paragraph_block(g_editor, 2) == R_RICH_TEXT_NUMBERED_LIST);
	CHECK(Rrich_text_editor_get_paragraph_indent(g_editor, 2) == 1);
	CHECK(Rrich_text_editor_get_paragraph_block(g_editor, 3) == R_RICH_TEXT_QUOTE);
	CHECK(Rrich_text_editor_get_paragraph_block(g_editor, 4) == R_RICH_TEXT_CODE_BLOCK);
	/* what the document cannot hold: a rule goes, a table is a paragraph per row, titles 4 to 6 are of the third level */
	Rrich_text_editor_set_markdown(g_editor, "a\n\n---\n\n| x | y |\n|---|---|\n| 1 | 2 |\n\n#### deep");
	CHECK_STR(md(), "a\n\nx \\| y\n\n1 \\| 2\n\n### deep\n");
}

static void test_html(void) {
	open_editor("# Title\n\nSome **bold**, *italic* & <u>under</u> [a \"link\"](https://e.org/?a=1&b=2)\n\n"
		"- one\n  - two\n- three\n\n1. first\n\n> quote\n\n```\nx < y\n```");
	CHECK_STR(Rrich_text_editor_get_html(g_editor),
		"<h1>Title</h1>\n"
		"<p>Some <strong>bold</strong>, <em>italic</em> &amp; <u>under</u> <a href=\"https://e.org/?a=1&amp;b=2\">a \"link\"</a></p>\n"
		"<ul>\n<li>one<ul>\n<li>two</li></ul></li>\n<li>three</li></ul>\n"
		"<ol>\n<li>first</li></ol>\n"
		"<blockquote><p>quote</p></blockquote>\n"
		"<pre><code>x &lt; y</code></pre>\n");
}

static void test_html_paste(void) {
	open_editor("");
	/* what is not part of the document goes: scripts, styles, attributes, links that run code */
	Rrich_text_editor_insert_html(g_editor,
		"<html><head><title>t</title><style>p { color: red }</style><script>alert(1)</script></head><body>"
		"<!--StartFragment--><h2 onclick=\"evil()\">Head</h2>"
		"<p class=\"x\" style=\"color:red\">Hello <b>bold</b> <i>it</i> <span style=\"font-weight:700\">heavy</span> "
		"<span style=\"font-style:italic;text-decoration:underline\">both</span> <del>gone</del> <kbd>key</kbd></p>"
		"<p><a href=\"javascript:alert(1)\">bad</a> <a href=\" JaVa&#x09;script:x\">bad2</a> <a href=\"https://ok.org\">good</a></p>"
		"<ul><li>a<ul><li>b</li></ul></li><li>c</li></ul>"
		"<ol><li><p>n1</p></li><li>n2</li></ol>"
		"<blockquote>q1<br>q2</blockquote>"
		"<pre>line 1\n  line 2\n</pre>"
		"<iframe src=\"x\">inside</iframe><img src=\"x\" onerror=\"y\">"
		"<p>a&nbsp;&amp;&lt;&#233;&#x263A;</p>"
		"<!--EndFragment--></body></html>");
	CHECK_STR(md(),
		"## Head\n\n"
		"Hello **bold** *it* **heavy** *<u>both</u>* ~~gone~~ `key`\n\n"
		"bad bad2 [good](https://ok.org)\n\n"
		"- a\n  - b\n- c\n\n"
		"1. n1\n2. n2\n\n"
		"> q1\\\n> q2\n\n"
		"```\nline 1\n  line 2\n```\n\n"
		"a\xC2\xA0\\&\\<\xC3\xA9\xE2\x98\xBA\n");
	/* Google Docs: everything in a <b> that is not bold */
	Rrich_text_editor_set_markdown(g_editor, "");
	Rrich_text_editor_insert_html(g_editor, "<b style=\"font-weight:normal;\" id=\"docs-internal-guid-1\"><p><span style=\"font-weight:700\">B</span>"
		"<span style=\"font-weight:400\">n</span></p></b>");
	CHECK_STR(md(), "**B**n\n");
	/* pasted in the middle of a paragraph: the first and last paragraphs join it */
	Rrich_text_editor_set_markdown(g_editor, "start end");
	select_range(0, 6, 0, 6);
	Rrich_text_editor_insert_html(g_editor, "<p><b>x</b></p><p>y</p>");
	CHECK_STR(md(), "start **x**\n\nyend\n");
	{
		int ap, ao, cp, co;
		Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
		CHECK(cp == 1 && co == 1);
	}
	/* one undo takes the paste back */
	CHECK(Rrich_text_editor_undo(g_editor));
	CHECK_STR(md(), "start end\n");
}

/* ---- keys ---- */

static void test_keys(void) {
	open_editor("");
	type("plain ");
	key(R_KEY_B, R_MOD_CTRL);
	type("bold");
	key(R_KEY_B, R_MOD_CTRL);
	type(" back");
	CHECK_STR(md(), "plain **bold** back\n");
	/* Shift + arrows select; Ctrl+I, U, E, Shift+X on the selection */
	key(R_KEY_HOME, 0);
	key(R_KEY_RIGHT, R_MOD_SHIFT);
	key(R_KEY_RIGHT, R_MOD_SHIFT);
	key(R_KEY_I, R_MOD_CTRL);
	CHECK_STR(md(), "*pl*ain **bold** back\n");
	key(R_KEY_U, R_MOD_CTRL);
	CHECK(Rrich_text_editor_get_style(g_editor) == (R_RICH_TEXT_ITALIC | R_RICH_TEXT_UNDERLINE));
	key(R_KEY_X, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK(Rrich_text_editor_get_style(g_editor) == (R_RICH_TEXT_ITALIC | R_RICH_TEXT_UNDERLINE | R_RICH_TEXT_STRIKETHROUGH));
	key(R_KEY_Z, R_MOD_CTRL);
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(md(), "*pl*ain **bold** back\n");
	/* a word with Ctrl+Shift+Right, made code */
	key(R_KEY_END, 0);
	key(R_KEY_LEFT, R_MOD_CTRL | R_MOD_SHIFT);
	key(R_KEY_E, R_MOD_CTRL);
	CHECK_STR(md(), "*pl*ain **bold** `back`\n");
	/* the kinds of paragraphs */
	key(R_KEY_2, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_HEADING_2);
	key(R_KEY_8, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_BULLET_LIST);
	key(R_KEY_8, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_PARAGRAPH);
	key(R_KEY_7, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_NUMBERED_LIST);
	key(R_KEY_9, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_QUOTE);
	key(R_KEY_0, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_PARAGRAPH);

	/* Ctrl+K: the bar of the link; what is typed goes there; Enter makes the link */
	Rrich_text_editor_set_markdown(g_editor, "see the docs");
	select_range(0, 8, 0, 12);
	key(R_KEY_K, R_MOD_CTRL);
	Rdraw_frame();
	type("https://raven.dev");
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "see the docs");
	key(R_KEY_ENTER, 0);
	CHECK_STR(md(), "see the [docs](https://raven.dev)\n");
	CHECK(Rrich_text_editor_get_style(g_editor) & R_RICH_TEXT_LINK);
	CHECK_STR(Rrich_text_editor_get_link(g_editor), "https://raven.dev");
	/* with the caret in it: the whole link edited; Escape cancels */
	select_range(0, 10, 0, 10);
	key(R_KEY_K, R_MOD_CTRL);
	type("x");
	key(R_KEY_ESCAPE, 0);
	CHECK_STR(md(), "see the [docs](https://raven.dev)\n");
	key(R_KEY_K, R_MOD_CTRL);
	key(R_KEY_A, R_MOD_CTRL);
	type("https://other.dev");
	key(R_KEY_ENTER, 0);
	CHECK_STR(md(), "see the [docs](https://other.dev)\n");
	/* an empty address removes it */
	key(R_KEY_K, R_MOD_CTRL);
	key(R_KEY_A, R_MOD_CTRL);
	key(R_KEY_BACKSPACE, 0);
	key(R_KEY_ENTER, 0);
	CHECK_STR(md(), "see the docs\n");
	/* typed after the end of a link: not part of it */
	Rrich_text_editor_set_markdown(g_editor, "[a](u)");
	select_range(0, 1, 0, 1);
	type("b");
	CHECK_STR(md(), "[a](u)b\n");
	/* the API: set_link with no selection and no link types the address */
	Rrich_text_editor_set_link(g_editor, "https://n.io");
	CHECK_STR(md(), "[a](u)b[https\\://n.io](https://n.io)\n");

	/* moves: words, ends, up and down */
	Rrich_text_editor_set_markdown(g_editor, "alpha beta\n\ngamma");
	select_range(0, 0, 0, 0);
	key(R_KEY_RIGHT, R_MOD_CTRL);
	{
		int ap, ao, cp, co;
		Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
		CHECK(cp == 0 && co == 6);
		key(R_KEY_DOWN, 0);
		Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
		CHECK(cp == 1);
		key(R_KEY_END, R_MOD_CTRL | R_MOD_SHIFT);
		Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
		CHECK(ap == 1 && cp == 1 && co == 5);
		key(R_KEY_A, R_MOD_CTRL);
		CHECK_STR(Rrich_text_editor_get_selected_text(g_editor), "alpha beta\ngamma");
	}
	/* Backspace and Delete by word */
	select_range(0, 10, 0, 10);
	key(R_KEY_BACKSPACE, R_MOD_ALT);
	CHECK_STR(Rrich_text_editor_get_paragraph_text(g_editor, 0), "alpha ");
	key(R_KEY_DELETE, 0);
	CHECK_STR(Rrich_text_editor_get_paragraph_text(g_editor, 0), "alpha gamma");
	/* Shift+Enter: a line in the paragraph */
	key(R_KEY_ENTER, R_MOD_SHIFT);
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 1);
	CHECK_STR(md(), "alpha \\\ngamma\n");
}

static void test_markdown_typed(void) {
	open_editor("");
	type("# Title");
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_HEADING_1);
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "Title");
	/* Enter after a title: a paragraph */
	key(R_KEY_ENTER, 0);
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_PARAGRAPH);
	type("Some **bold** and *it* and `co` and ~~st~~ and __b2__ and snake_case_name.");
	CHECK_STR(md(), "# Title\n\nSome **bold** and *it* and `co` and ~~st~~ and **b2** and snake\\_case\\_name.\n");
	key(R_KEY_ENTER, 0);
	type("- item");
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_BULLET_LIST);
	key(R_KEY_ENTER, 0);
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_BULLET_LIST);
	/* an empty item: Enter ends the list */
	key(R_KEY_ENTER, 0);
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_PARAGRAPH);
	type("1. first");
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_NUMBERED_LIST);
	key(R_KEY_ENTER, 0);
	key(R_KEY_ENTER, 0);
	type("> wise");
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_QUOTE);
	key(R_KEY_ENTER, 0);
	key(R_KEY_ENTER, 0);
	type("```");
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_CODE_BLOCK);
	type("int **x**;");
	key(R_KEY_ENTER, 0);
	type("y");
	key(R_KEY_ENTER, 0);
	/* the empty last line of a block of code ends it */
	key(R_KEY_ENTER, 0);
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_PARAGRAPH);
	type("end");
	CHECK_STR(md(), "# Title\n\nSome **bold** and *it* and `co` and ~~st~~ and **b2** and snake\\_case\\_name.\n\n- item\n\n"
		"1. first\n\n> wise\n\n```\nint **x**;\ny\n```\n\nend\n");

	/* Ctrl+Z gives back what was typed */
	Rrich_text_editor_set_markdown(g_editor, "");
	type("## ");
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_HEADING_2);
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_PARAGRAPH);
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "## ");
	Rrich_text_editor_set_markdown(g_editor, "");
	type("**b**");
	CHECK_STR(md(), "**b**\n");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "**b**");
	/* off: as typed */
	Rrich_text_editor_set_markdown_shortcuts(g_editor, R_FALSE);
	Rrich_text_editor_set_markdown(g_editor, "");
	type("# no *x*");
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "# no *x*");
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_PARAGRAPH);
	Rrich_text_editor_set_markdown_shortcuts(g_editor, R_TRUE);
	/* not in a word that is not closed, not against spaces */
	Rrich_text_editor_set_markdown(g_editor, "");
	type("2 * 3 * 4 and a* b*");
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "2 * 3 * 4 and a* b*");
}

static void test_lists(void) {
	open_editor("- a\n- b\n- c");
	select_range(1, 1, 1, 1);
	key(R_KEY_TAB, 0);
	CHECK(Rrich_text_editor_get_paragraph_indent(g_editor, 1) == 1);
	/* at most one level deeper than the item above */
	key(R_KEY_TAB, 0);
	CHECK(Rrich_text_editor_get_paragraph_indent(g_editor, 1) == 1);
	CHECK_STR(md(), "- a\n  - b\n- c\n");
	/* the first item cannot go deeper */
	select_range(0, 0, 0, 0);
	key(R_KEY_TAB, 0);
	CHECK(Rrich_text_editor_get_paragraph_indent(g_editor, 0) == 0);
	/* several items at once */
	select_range(1, 0, 2, 1);
	key(R_KEY_TAB, 0);
	CHECK(Rrich_text_editor_get_paragraph_indent(g_editor, 1) == 1);
	CHECK(Rrich_text_editor_get_paragraph_indent(g_editor, 2) == 1);
	key(R_KEY_TAB, R_MOD_SHIFT);
	CHECK(Rrich_text_editor_get_paragraph_indent(g_editor, 1) == 0);
	CHECK(Rrich_text_editor_get_paragraph_indent(g_editor, 2) == 0);
	CHECK_STR(md(), "- a\n- b\n- c\n");
	/* Enter on an empty nested item goes out a level, then out of the list */
	Rrich_text_editor_set_markdown(g_editor, "- a\n  - b");
	select_range(1, 1, 1, 1);
	key(R_KEY_ENTER, 0);
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 3);
	CHECK(Rrich_text_editor_get_paragraph_indent(g_editor, 2) == 1);
	key(R_KEY_ENTER, 0);
	CHECK(Rrich_text_editor_get_paragraph_block(g_editor, 2) == R_RICH_TEXT_BULLET_LIST);
	CHECK(Rrich_text_editor_get_paragraph_indent(g_editor, 2) == 0);
	key(R_KEY_ENTER, 0);
	CHECK(Rrich_text_editor_get_paragraph_block(g_editor, 2) == R_RICH_TEXT_PARAGRAPH);
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 3);
	/* Backspace at the start of an item: out a level, then a paragraph, then joined to the one above */
	Rrich_text_editor_set_markdown(g_editor, "- a\n  - b");
	select_range(1, 0, 1, 0);
	key(R_KEY_BACKSPACE, 0);
	CHECK(Rrich_text_editor_get_paragraph_indent(g_editor, 1) == 0);
	key(R_KEY_BACKSPACE, 0);
	CHECK(Rrich_text_editor_get_paragraph_block(g_editor, 1) == R_RICH_TEXT_PARAGRAPH);
	key(R_KEY_BACKSPACE, 0);
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 1);
	CHECK_STR(md(), "- ab\n");
	/* Shift+Tab on an item of the first level: a paragraph */
	key(R_KEY_TAB, R_MOD_SHIFT);
	CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_PARAGRAPH);
	/* numbers: per level, started again after a paragraph */
	Rrich_text_editor_set_markdown(g_editor, "1. a\n2. b\n   1. c\n3. d\n\nx\n\n1. e");
	CHECK_STR(md(), "1. a\n2. b\n   1. c\n3. d\n\nx\n\n1. e\n");
	/* Enter in an item splits it: two items */
	select_range(0, 1, 0, 1);
	type("z");
	select_range(0, 1, 0, 1);
	key(R_KEY_ENTER, 0);
	CHECK_STR(md(), "1. a\n2. z\n3. b\n   1. c\n4. d\n\nx\n\n1. e\n");
	/* Enter at the start of a title leaves a paragraph above it */
	Rrich_text_editor_set_markdown(g_editor, "# T");
	key(R_KEY_ENTER, 0);
	CHECK_STR(md(), "&nbsp;\n\n# T\n");
}

static void test_history(void) {
	int ap, ao, cp, co;
	open_editor("one two");
	select_range(0, 4, 0, 7);
	Rrich_text_editor_toggle_style(g_editor, R_RICH_TEXT_BOLD);
	select_range(0, 7, 0, 7);
	type("abc");
	CHECK_STR(md(), "one **twoabc**\n");
	/* the characters typed in a row: one step, the selection before them given back */
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(md(), "one **two**\n");
	Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
	CHECK(ap == 0 && ao == 7 && cp == 0 && co == 7);
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(md(), "one two\n");
	Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
	CHECK(ap == 0 && ao == 4 && cp == 0 && co == 7);
	CHECK(!Rrich_text_editor_undo(g_editor));
	CHECK(Rrich_text_editor_can_redo(g_editor));
	key(R_KEY_Y, R_MOD_CTRL);
	CHECK_STR(md(), "one **two**\n");
	key(R_KEY_Z, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK_STR(md(), "one **twoabc**\n");
	Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
	CHECK(cp == 0 && co == 10);
	CHECK(!Rrich_text_editor_can_redo(g_editor));
	/* paragraphs split and joined come back */
	key(R_KEY_ENTER, 0);
	type("x");
	key(R_KEY_ENTER, 0);
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 3);
	key(R_KEY_Z, R_MOD_CTRL);
	key(R_KEY_Z, R_MOD_CTRL);
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 1);
	CHECK_STR(md(), "one **twoabc**\n");
	/* a selection removed comes back selected */
	Rrich_text_editor_set_markdown(g_editor, "a\n\nb\n\nc");
	select_range(0, 1, 2, 0);
	key(R_KEY_BACKSPACE, 0);
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "ac");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "a\nb\nc");
	Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
	CHECK(ap == 0 && ao == 1 && cp == 2 && co == 0);
	/* the history goes with a new document; read-only: no undo */
	Rrich_text_editor_set_markdown(g_editor, "z");
	CHECK(!Rrich_text_editor_can_undo(g_editor));
	type("q");
	Rrich_text_editor_set_read_only(g_editor, R_TRUE);
	CHECK(!Rrich_text_editor_undo(g_editor));
	type("r");
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "qz");
	Rrich_text_editor_set_read_only(g_editor, R_FALSE);
}

static void test_composition(void) {
	int x, y, w, h, s = -1, e = -1;
	open_editor("**ab**");
	select_range(0, 1, 0, 1);
	Rdraw_frame();
	g_changes = 0;
	/* the input method composes: the document does not change, the composition is shown at the caret */
	Rwindow_inject_composition(g_window, "ni", -1, -1);
	Rdraw_frame();
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "ab");
	CHECK(g_changes == 0);
	CHECK_STR(Rwindow_get_text_composition(g_window, &s, &e), "ni");
	CHECK(Rwindow_get_text_input_rect(g_window, &x, &y, &w, &h));
	CHECK(w > 0 && h > 0);
	/* what it commits is typed, with the looks of the caret, as one step */
	Rwindow_inject_text(g_window, "\xE4\xBD\xA0");
	CHECK(Rwindow_get_text_composition(g_window, NULL, NULL) == NULL);
	CHECK_STR(md(), "**a\xE4\xBD\xA0" "b**\n");
	CHECK(g_changes == 1);
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(md(), "**ab**\n");
	/* a composition in place of a selection */
	select_range(0, 0, 0, 2);
	Rwindow_inject_composition(g_window, "k", -1, -1);
	Rdraw_frame();
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "ab");
	Rwindow_inject_text(g_window, "K");
	CHECK_STR(md(), "**K**\n");
}

static void test_clipboard(void) {
	size_t size = 0;
	char* data;
	open_editor("# T\n\nsome **bold** text\n\n- item");
	select_range(1, 5, 2, 2);
	key(R_KEY_C, R_MOD_CTRL);
	CHECK_STR(Rclipboard_get_text(), "bold text\nit");
	data = Rclipboard_get_data("text/html", &size);
	CHECK(data != NULL);
	if (data) CHECK_STR(data, "<p><strong>bold</strong> text</p>\n<ul>\n<li>it</li></ul>\n");
	Rmemory_free(data);
	data = Rclipboard_get_data("text/markdown", &size);
	CHECK(data != NULL);
	if (data) CHECK_STR(data, "**bold** text\n\n- it\n");
	Rmemory_free(data);
	/* cut, then pasted elsewhere: HTML comes first */
	key(R_KEY_X, R_MOD_CTRL);
	CHECK_STR(md(), "# T\n\nsome em\n");
	select_range(0, 1, 0, 1);
	key(R_KEY_V, R_MOD_CTRL);
	CHECK_STR(md(), "# T**bold** text\n\n- it\n\nsome em\n");
	/* plain text only */
	Rclipboard_set_text("p1\np2");
	Rrich_text_editor_set_markdown(g_editor, "**xy**");
	select_range(0, 1, 0, 1);
	key(R_KEY_V, R_MOD_CTRL);
	CHECK_STR(md(), "**xp1**\n\n**p2y**\n");
	/* into a line of code: no looks */
	Rrich_text_editor_set_markdown(g_editor, "```\ncode\n```");
	select_range(0, 2, 0, 2);
	Rrich_text_editor_insert_html(g_editor, "<b>B</b>");
	CHECK_STR(md(), "```\ncoBde\n```\n");
}

static void test_toolbar(void) {
	float x, y, w, h, bx, by;
	open_editor("word");
	CHECK(Rrich_text_editor_get_toolbar(g_editor));
	Rwidget_get_rect(g_editor, &x, &y, &w, &h);
	select_range(0, 0, 0, 4);
	Rdraw_frame();
	/* the first button (bold): the toolbar is 1 + 5 from the corner; buttons of twice the text size */
	bx = x + 1.0f + 6.0f + 10.0f;
	by = y + 1.0f + 5.0f + 10.0f;
	test_click_at(g_window, bx, by);
	Rdraw_frame();
	CHECK_STR(md(), "**word**\n");
	/* the button lit: it turns bold off */
	test_click_at(g_window, bx, by);
	CHECK_STR(md(), "word\n");
	/* a title of the first level (after five looks, the link and a gap) */
	{
		float button = floorf(Rtheme_get_metric(R_METRIC_FONT_SIZE) * 2.0f + 0.5f);
		float hx = x + 1.0f + 6.0f + 6.0f * (button + 2.0f) + 11.0f + button * 0.5f;
		test_click_at(g_window, hx, by);
		CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_HEADING_1);
		test_click_at(g_window, hx, by);
		CHECK(Rrich_text_editor_get_block(g_editor) == R_RICH_TEXT_PARAGRAPH);
	}
	/* without the toolbar the text starts higher, and clicking there selects text */
	Rrich_text_editor_set_toolbar(g_editor, R_FALSE);
	Rdraw_frame();
	CHECK(!Rrich_text_editor_get_toolbar(g_editor));
	test_click_at(g_window, bx, by);
	CHECK_STR(md(), "word\n");
	/* read-only: the buttons do nothing */
	Rrich_text_editor_set_toolbar(g_editor, R_TRUE);
	Rrich_text_editor_set_read_only(g_editor, R_TRUE);
	Rdraw_frame();
	select_range(0, 0, 0, 4);
	test_click_at(g_window, bx, by);
	CHECK_STR(md(), "word\n");
	Rrich_text_editor_set_read_only(g_editor, R_FALSE);
}

static void test_links_followed(void) {
	float x, y, w, h;
	int ap, ao, cp, co;
	open_editor("[link](https://raven.dev/a)");
	Rrich_text_editor_set_toolbar(g_editor, R_FALSE);
	Rdraw_frame();
	Rwidget_get_rect(g_editor, &x, &y, &w, &h);
	/* (no key held from the tests before) */
	Rwindow_inject_key(g_window, R_KEY_LEFT_CTRL, R_FALSE, 0);
	/* a click places the caret; Ctrl+click follows */
	test_click_at(g_window, x + 1.0f + 12.0f + 8.0f, y + 1.0f + 12.0f + 8.0f);
	CHECK(g_followed[0] == '\0');
	Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
	CHECK(cp == 0 && co > 0 && co < 4);
	/* (one click each: Rtest_click_at says how many) */
	Rtest_click_at(g_editor, 1.0f + 12.0f + 8.0f, 1.0f + 12.0f + 8.0f, R_MOUSE_LEFT, 1, R_MOD_CTRL);
	CHECK_STR(g_followed, "https://raven.dev/a");
	/* read-only: a click follows */
	g_followed[0] = '\0';
	Rrich_text_editor_set_read_only(g_editor, R_TRUE);
	Rtest_click_at(g_editor, 1.0f + 12.0f + 8.0f, 1.0f + 12.0f + 8.0f, R_MOUSE_LEFT, 1, 0);
	CHECK_STR(g_followed, "https://raven.dev/a");
	/* not beside the link */
	g_followed[0] = '\0';
	Rtest_click_at(g_editor, w - 40.0f, 1.0f + 12.0f + 8.0f, R_MOUSE_LEFT, 1, 0);
	CHECK(g_followed[0] == '\0');
	Rrich_text_editor_set_read_only(g_editor, R_FALSE);
}

static void test_mouse(void) {
	float x, y, w, h;
	int ap, ao, cp, co;
	open_editor("first words here\n\nsecond");
	Rrich_text_editor_set_toolbar(g_editor, R_FALSE);
	Rdraw_frame();
	Rwidget_get_rect(g_editor, &x, &y, &w, &h);
	/* a double click selects a word, a triple click the paragraph */
	Rtest_click_at(g_editor, 1.0f + 12.0f + 4.0f, 1.0f + 12.0f + 6.0f, R_MOUSE_LEFT, 2, 0);
	CHECK_STR(Rrich_text_editor_get_selected_text(g_editor), "first");
	Rtest_click_at(g_editor, 1.0f + 12.0f + 4.0f, 1.0f + 12.0f + 6.0f, R_MOUSE_LEFT, 3, 0);
	CHECK_STR(Rrich_text_editor_get_selected_text(g_editor), "first words here");
	/* a click far below: the end of the text */
	test_click_at(g_window, x + w * 0.5f, y + h - 20.0f);
	Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
	CHECK(cp == 1 && co == 6 && ap == 1 && ao == 6);
	/* a drag selects */
	Rwindow_inject_mouse_move(g_window, x + 13.0f, y + 19.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, x + w * 0.8f, y + h - 20.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
	CHECK(ap == 0 && ao == 0 && cp == 1 && co == 6);
	CHECK(g_selections > 0);
}

static void test_xml_and_access(void) {
	static const char* xml =
		"<RichTextEditor Name=\"r\" Placeholder=\"Write\" Toolbar=\"false\" MarkdownShortcuts=\"false\" TextSize=\"15\">\n"
		"    # Notes\n"
		"    - **eggs**\n"
		"</RichTextEditor>";
	static const char* html = "<RichText Html=\"&lt;h2&gt;H&lt;/h2&gt;&lt;p&gt;&lt;i&gt;i&lt;/i&gt;&lt;/p&gt;\" ReadOnly=\"true\"/>";
	Rid editor = Rwidget_create_from_xml(xml, strlen(xml));
	Rid other;
	CHECK(Rwidget_get_type(editor) == R_WIDGET_RICH_TEXT_EDITOR);
	Rwindow_set_content(g_window, editor);
	Rdraw_frame();
	CHECK_STR(Rrich_text_editor_get_markdown(editor), "# Notes\n\n- **eggs**\n");
	CHECK(!Rrich_text_editor_get_toolbar(editor));
	/* accessibility: a document, its text as the value */
	CHECK_STR(Rtest_get_text(editor), "Notes\neggs");
	CHECK(strstr(Rtest_dump_tree(editor), "document") != NULL);
	other = Rwidget_create_from_xml(html, strlen(html));
	CHECK(Rwidget_get_type(other) == R_WIDGET_RICH_TEXT_EDITOR);
	Rwindow_set_content(g_window, other);
	Rdraw_frame();
	CHECK_STR(Rrich_text_editor_get_markdown(other), "## H\n\n*i*\n");
	CHECK(Rrich_text_editor_is_read_only(other));
	/* an empty document shows its placeholder; nothing to check but that it draws */
	Rrich_text_editor_set_markdown(other, "");
	Rrich_text_editor_set_placeholder(other, "Empty");
	Rdraw_frame();
	/* bad blocks are refused */
	EXPECT_ERRORS_BEGIN();
	Rrich_text_editor_set_block(other, (RRichTextBlock)12);
	EXPECT_ERRORS_END(1);
}

static void test_spelling_and_theme(void) {
	/* with spell checking on (no checker in the headless build: nothing is underlined, nothing breaks) */
	open_editor("Teh *quick* fox\n\n```\nnot checked\n```");
	Rrich_text_editor_set_spell_check(g_editor, R_TRUE);
	Rrich_text_editor_set_spell_language(g_editor, "en_US");
	Rdraw_frame();
	Rtheme_set_dark(R_FALSE);
	Rdraw_frame();
	Rtheme_set_dark(R_TRUE);
	Rdraw_frame();
	CHECK_STR(Rrich_text_editor_get_text(g_editor), "Teh quick fox\nnot checked");
}

static double now(void) {
	return Rget_time();
}

static void test_large(void) {
	int i, ap, ao, cp, co;
	double start, typing, frames;
	size_t cap = 5000 * 64, length = 0;
	char* text = (char*)malloc(cap);
	text[0] = '\0';
	for (i = 0; i < 5000; i++) {
		if (i % 50 == 0) length += (size_t)snprintf(text + length, cap - length, "## Part %d\n\n", i / 50);
		else if (i % 7 == 0) length += (size_t)snprintf(text + length, cap - length, "- item **%d** with *words*\n\n", i);
		else length += (size_t)snprintf(text + length, cap - length, "Paragraph %d has some [text](https://x.org/%d) in it.\n\n", i, i);
	}
	open_editor("");
	start = now();
	Rrich_text_editor_set_markdown(g_editor, text);
	Rdraw_frame();
	printf("  5000 paragraphs read and laid out: %.1f ms\n", (now() - start) * 1000.0);
	CHECK(Rrich_text_editor_get_paragraph_count(g_editor) == 5000);
	/* typing in the middle: one paragraph laid out again per character */
	select_range(2500, 3, 2500, 3);
	Rdraw_frame();
	start = now();
	for (i = 0; i < 50; i++) {
		type("x");
		Rdraw_frame();
	}
	typing = (now() - start) * 1000.0 / 50.0;
	printf("  a character typed and drawn: %.2f ms\n", typing);
	CHECK(typing < 25.0);
	Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
	CHECK(cp == 2500 && co == 53);
	/* scrolling: only the paragraphs on screen are drawn */
	start = now();
	for (i = 0; i < 50; i++) {
		Rwindow_inject_mouse_move(g_window, W * 0.5f, H * 0.5f);
		Rwindow_inject_mouse_wheel(g_window, 0.0f, -10.0f);
		Rdraw_frame();
	}
	frames = (now() - start) * 1000.0 / 50.0;
	printf("  a frame scrolled: %.2f ms\n", frames);
	CHECK(frames < 25.0);
	/* to the end and back */
	key(R_KEY_END, R_MOD_CTRL);
	Rdraw_frame();
	Rrich_text_editor_get_selection(g_editor, &ap, &ao, &cp, &co);
	CHECK(cp == 4999);
	start = now();
	key(R_KEY_Z, R_MOD_CTRL);
	Rdraw_frame();
	printf("  undo: %.2f ms\n", (now() - start) * 1000.0);
	/* (typed in two steps: 48 characters, then 2) */
	CHECK(strncmp(Rrich_text_editor_get_paragraph_text(g_editor, 2500), "Parxxxxxx", 9) == 0);
	CHECK(strstr(Rrich_text_editor_get_paragraph_text(g_editor, 2500), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx") == NULL);
	free(text);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("rich text", (int)W, (int)H, R_WINDOW_DEFAULT);

	RUN(test_model);
	RUN(test_markdown);
	RUN(test_html);
	RUN(test_html_paste);
	RUN(test_keys);
	RUN(test_markdown_typed);
	RUN(test_lists);
	RUN(test_history);
	RUN(test_composition);
	RUN(test_clipboard);
	RUN(test_toolbar);
	RUN(test_links_followed);
	RUN(test_mouse);
	RUN(test_xml_and_access);
	RUN(test_spelling_and_theme);
	RUN(test_large);

	Rdraw_frame();
	Rshutdown();
	return test_report("test_rich_text");
}
