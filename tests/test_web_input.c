/**
 * Raven tests - test_web_input.c
 * The bookkeeping of the text field of the browser platform
 * (platform/web_input.c), apart from any browser: UTF-16 offsets of the DOM
 * against the UTF-8 bytes of Raven, the part of the text the textarea holds
 * around the selection, and the attributes of each kind of field. Runs
 * everywhere.
 */

#include "test.h"
#include "../src/platform/web_input.h"

/* "é" is 2 bytes and 1 unit, "日" 3 bytes and 1 unit, "😀" 4 bytes and 2 units */
#define E_ACUTE "\xC3\xA9"
#define NICHI "\xE6\x97\xA5"
#define GRIN "\xF0\x9F\x98\x80"

static void test_utf16_offsets(void) {
	const char* text = "a" E_ACUTE NICHI GRIN "b";   /* a(0) é(1..2) 日(3..5) 😀(6..9) b(10), 11 bytes */
	int length = (int)strlen(text);
	CHECK(rvn_utf16_count(text, length) == 6);
	CHECK(rvn_utf16_count(text, 3) == 2);
	CHECK(rvn_utf16_count(text, 6) == 3);
	CHECK(rvn_utf16_count(text, 10) == 5);
	CHECK(rvn_utf16_count(NULL, 4) == 0);
	CHECK(rvn_utf16_count(text, 0) == 0);
	CHECK(rvn_utf16_to_utf8(text, length, 0) == 0);
	CHECK(rvn_utf16_to_utf8(text, length, 1) == 1);
	CHECK(rvn_utf16_to_utf8(text, length, 2) == 3);
	CHECK(rvn_utf16_to_utf8(text, length, 3) == 6);
	/* the middle of the surrogate pair: back to the start of the emoji */
	CHECK(rvn_utf16_to_utf8(text, length, 4) == 6);
	CHECK(rvn_utf16_to_utf8(text, length, 5) == 10);
	CHECK(rvn_utf16_to_utf8(text, length, 6) == 11);
	CHECK(rvn_utf16_to_utf8(text, length, 99) == 11);
	CHECK(rvn_utf16_to_utf8(text, length, -3) == 0);
	/* a character cut short at the end of the text counts once */
	CHECK(rvn_utf16_count("a\xE6\x97", 3) == 2);
	CHECK(rvn_utf16_to_utf8("a\xE6\x97", 3, 2) == 3);
}

static void test_surrounding(void) {
	rvn_web_surrounding_t part;
	char text[64];
	/* all of a short text, the selection in UTF-16 units */
	rvn_web_surrounding("a" GRIN "bc", 5, 7, 512, &part);
	CHECK(part.offset == 0 && part.length == 7);
	CHECK(part.start16 == 3 && part.end16 == 5);
	/* either order, offsets inside a character go back to its start */
	rvn_web_surrounding("a" GRIN "bc", 7, 3, 512, &part);
	CHECK(part.offset == 0 && part.length == 7);
	CHECK(part.start16 == 1 && part.end16 == 5);
	/* a long text: max units on each side of the selection */
	memset(text, 'x', 40);
	text[40] = '\0';
	rvn_web_surrounding(text, 20, 22, 5, &part);
	CHECK(part.offset == 15 && part.length == 12);
	CHECK(part.start16 == 5 && part.end16 == 7);
	/* at the start and at the end */
	rvn_web_surrounding(text, 0, 0, 5, &part);
	CHECK(part.offset == 0 && part.length == 5 && part.start16 == 0 && part.end16 == 0);
	rvn_web_surrounding(text, 40, 40, 5, &part);
	CHECK(part.offset == 35 && part.length == 5 && part.start16 == 5 && part.end16 == 5);
	/* the whole selection, longer than the room */
	rvn_web_surrounding(text, 2, 38, 1, &part);
	CHECK(part.offset == 1 && part.length == 38 && part.start16 == 1 && part.end16 == 37);
	/* an emoji that does not fit whole is left out, never cut in two */
	rvn_web_surrounding("ab" GRIN "|" GRIN "cd", 7, 7, 3, &part);
	CHECK(part.offset == 2 && part.length == 10);
	CHECK(part.start16 == 3 && part.end16 == 3);
	rvn_web_surrounding("ab" GRIN "|" GRIN "cd", 7, 7, 2, &part);
	CHECK(part.offset == 6 && part.length == 5);
	CHECK(part.start16 == 1 && part.end16 == 1);
	rvn_web_surrounding("ab" GRIN "|" GRIN "cd", 7, 7, 1, &part);
	CHECK(part.offset == 6 && part.length == 1);
	CHECK(part.start16 == 1 && part.end16 == 1);
	/* nothing */
	rvn_web_surrounding("", 0, 0, 512, &part);
	CHECK(part.offset == 0 && part.length == 0 && part.start16 == 0 && part.end16 == 0);
	rvn_web_surrounding(NULL, 3, 3, 512, &part);
	CHECK(part.offset == 0 && part.length == 0);
}

static void test_field_hints(void) {
	rvn_web_field_hints_t hints;
	RTextInputInfo info;
	memset(&info, 0, sizeof(info));
	info.type = R_TEXT_INPUT_TYPE_TEXT;
	info.return_key = R_RETURN_KEY_DEFAULT;

	rvn_web_field_hints(R_FALSE, RVN_WEB_KIND_TEXT_INPUT, &info, &hints);
	CHECK(hints.mode == RVN_WEB_FIELD_NONE);

	/* prose: suggestions and corrections */
	rvn_web_field_hints(R_TRUE, RVN_WEB_KIND_TEXT_INPUT, &info, &hints);
	CHECK(hints.mode == RVN_WEB_FIELD_MIRROR);
	CHECK(hints.spellcheck);
	CHECK_STR(hints.inputmode, "text");
	CHECK_STR(hints.autocapitalize, "sentences");
	CHECK_STR(hints.enterkeyhint, "");

	info.type = R_TEXT_INPUT_TYPE_EMAIL;
	info.return_key = R_RETURN_KEY_NEXT;
	rvn_web_field_hints(R_TRUE, RVN_WEB_KIND_TEXT_INPUT, &info, &hints);
	CHECK(hints.mode == RVN_WEB_FIELD_MIRROR && !hints.spellcheck);
	CHECK_STR(hints.inputmode, "email");
	CHECK_STR(hints.autocomplete, "email");
	CHECK_STR(hints.autocapitalize, "off");
	CHECK_STR(hints.enterkeyhint, "next");

	info.type = R_TEXT_INPUT_TYPE_URL;
	info.return_key = R_RETURN_KEY_GO;
	rvn_web_field_hints(R_TRUE, RVN_WEB_KIND_TEXT_INPUT, &info, &hints);
	CHECK_STR(hints.inputmode, "url");
	CHECK_STR(hints.enterkeyhint, "go");
	info.type = R_TEXT_INPUT_TYPE_NUMBER;
	rvn_web_field_hints(R_TRUE, RVN_WEB_KIND_TEXT_INPUT, &info, &hints);
	CHECK_STR(hints.inputmode, "numeric");
	info.type = R_TEXT_INPUT_TYPE_DECIMAL;
	rvn_web_field_hints(R_TRUE, RVN_WEB_KIND_TEXT_INPUT, &info, &hints);
	CHECK_STR(hints.inputmode, "decimal");
	info.type = R_TEXT_INPUT_TYPE_PHONE;
	rvn_web_field_hints(R_TRUE, RVN_WEB_KIND_TEXT_INPUT, &info, &hints);
	CHECK_STR(hints.inputmode, "tel");
	CHECK_STR(hints.autocomplete, "tel");
	info.type = R_TEXT_INPUT_TYPE_SEARCH;
	info.return_key = R_RETURN_KEY_DEFAULT;
	rvn_web_field_hints(R_TRUE, RVN_WEB_KIND_TEXT_INPUT, &info, &hints);
	CHECK_STR(hints.inputmode, "search");
	CHECK_STR(hints.enterkeyhint, "search");
	CHECK(hints.spellcheck);

	/* a password: the password input, nothing learned */
	info.type = R_TEXT_INPUT_TYPE_TEXT;
	info.password = R_TRUE;
	info.read_only = R_TRUE;
	rvn_web_field_hints(R_TRUE, RVN_WEB_KIND_TEXT_INPUT, &info, &hints);
	CHECK(hints.mode == RVN_WEB_FIELD_SECURE);
	CHECK(!hints.spellcheck && hints.read_only);
	CHECK_STR(hints.autocomplete, "current-password");

	/* without the text services of the core: keys, no corrections */
	rvn_web_field_hints(R_TRUE, RVN_WEB_KIND_TEXT_EDITOR, NULL, &hints);
	CHECK(hints.mode == RVN_WEB_FIELD_KEYS && !hints.spellcheck);
	CHECK_STR(hints.enterkeyhint, "enter");
	rvn_web_field_hints(R_TRUE, RVN_WEB_KIND_SPIN_BOX, NULL, &hints);
	CHECK(hints.mode == RVN_WEB_FIELD_KEYS);
	CHECK_STR(hints.inputmode, "decimal");
	rvn_web_field_hints(R_TRUE, RVN_WEB_KIND_TEXT_INPUT, NULL, &hints);
	CHECK(hints.mode == RVN_WEB_FIELD_KEYS);
}

int main(void) {
	RUN(test_utf16_offsets);
	RUN(test_surrounding);
	RUN(test_field_hints);
	return test_report("test_web_input");
}
