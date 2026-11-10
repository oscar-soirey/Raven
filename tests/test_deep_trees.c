/**
 * Raven tests - trees far deeper than any interface is.
 *
 * Destroying a widget tree must not depend on the call stack, and XML nested
 * past what the library accepts is refused (R_ERROR_XML), not followed down.
 * The stack of this program is made small, so that a walk going down one
 * call per level overflows it: here where the system allows it, by the
 * linker on Windows (see CMakeLists.txt).
 */

#include "test.h"

#include <stdlib.h>

#if !defined(_WIN32)
#include <sys/resource.h>
#endif

#define TEST_STACK_BYTES (512 * 1024)
/* (a call takes at least 32 bytes: followed by a recursion, this chain needs several times the stack) */
#define TEST_WIDGET_DEPTH 100000
/* The chain is made of pieces: see deep_chain. */
#define TEST_WIDGET_PIECE 250
/* What the reader accepts under the root (RVN_XML_MAX_DEPTH). */
#define TEST_XML_DEPTH 256

static void small_stack(void) {
#if !defined(_WIN32)
	struct rlimit limit;
	if (getrlimit(RLIMIT_STACK, &limit) == 0 && (limit.rlim_cur == RLIM_INFINITY || limit.rlim_cur > TEST_STACK_BYTES)) {
		limit.rlim_cur = TEST_STACK_BYTES;
		setrlimit(RLIMIT_STACK, &limit);
	}
#endif
}

/* ---------------------------------------------------------------------------
 *  Widgets
 * ------------------------------------------------------------------------- */

/*
 * A chain of boxes, each the only child of the one before. Adding a widget
 * updates every ancestor: built one box at a time, a chain this long would
 * take minutes. It is built in short pieces, each added whole to the end.
 */
static Rid deep_chain(int depth, Rid* deepest) {
	Rid root = Rwidget_create(R_WIDGET_VBOX);
	Rid end = root;
	int made = 0;
	while (made < depth) {
		Rid piece = Rwidget_create(R_WIDGET_VBOX);
		Rid piece_end = piece;
		int i;
		made++;
		for (i = 1; i < TEST_WIDGET_PIECE && made < depth; i++, made++) {
			Rid child = Rwidget_create(R_WIDGET_VBOX);
			Rwidget_add_child(piece_end, child);
			piece_end = child;
		}
		Rwidget_add_child(end, piece);
		end = piece_end;
	}
	if (deepest) *deepest = end;
	return root;
}

static void test_destroy_deep_chain(void) {
	Rid deepest = R_INVALID_ID;
	Rid root = deep_chain(TEST_WIDGET_DEPTH, &deepest);
	CHECK(Rwidget_is_valid(root));
	CHECK(Rwidget_is_valid(deepest));
	CHECK(Rwidget_get_parent(Rwidget_get_parent(deepest)) != R_INVALID_ID);
	Rwidget_destroy(root);
	CHECK(!Rwidget_is_valid(root));
	CHECK(!Rwidget_is_valid(deepest));
}

static int g_destroyed[8];
static int g_destroyed_count = 0;

static void on_destroyed(Rid widget, void* user_data) {
	(void)widget;
	if (g_destroyed_count < 8) g_destroyed[g_destroyed_count] = (int)(intptr_t)user_data;
	g_destroyed_count++;
}

/* A widget is told before its children, and the children in their order. */
static void test_destroy_order(void) {
	Rid a = Rwidget_create(R_WIDGET_VBOX);
	Rid b = Rwidget_create(R_WIDGET_VBOX);
	Rid c = Rwidget_create(R_WIDGET_VBOX);
	Rid d = Rwidget_create(R_WIDGET_VBOX);
	Rid e = Rwidget_create(R_WIDGET_VBOX);
	Rwidget_add_child(a, b);
	Rwidget_add_child(a, c);
	Rwidget_add_child(b, d);
	Rwidget_add_child(b, e);
	Rwidget_set_destroy_callback(a, on_destroyed, (void*)(intptr_t)1);
	Rwidget_set_destroy_callback(b, on_destroyed, (void*)(intptr_t)2);
	Rwidget_set_destroy_callback(d, on_destroyed, (void*)(intptr_t)3);
	Rwidget_set_destroy_callback(e, on_destroyed, (void*)(intptr_t)4);
	Rwidget_set_destroy_callback(c, on_destroyed, (void*)(intptr_t)5);
	g_destroyed_count = 0;
	Rwidget_destroy(a);
	CHECK(g_destroyed_count == 5);
	CHECK(g_destroyed[0] == 1 && g_destroyed[1] == 2 && g_destroyed[2] == 3 && g_destroyed[3] == 4 && g_destroyed[4] == 5);
	CHECK(!Rwidget_is_valid(a) && !Rwidget_is_valid(b) && !Rwidget_is_valid(c) && !Rwidget_is_valid(d) && !Rwidget_is_valid(e));
}

/* ---------------------------------------------------------------------------
 *  XML
 * ------------------------------------------------------------------------- */

/* "<VBox>" `depth` times, then as many "</VBox>". */
static char* nested_xml(int depth, size_t* size) {
	static const char open[] = "<VBox>", close[] = "</VBox>";
	size_t length = (size_t)depth * (sizeof(open) - 1 + sizeof(close) - 1);
	char* text = (char*)malloc(length + 1);
	char* cursor = text;
	int i;
	for (i = 0; i < depth; i++, cursor += sizeof(open) - 1) memcpy(cursor, open, sizeof(open) - 1);
	for (i = 0; i < depth; i++, cursor += sizeof(close) - 1) memcpy(cursor, close, sizeof(close) - 1);
	*cursor = '\0';
	*size = length;
	return text;
}

static void test_xml_too_deep(void) {
	size_t size = 0;
	char* text = nested_xml(100000, &size);
	const char* detail = NULL;
	RXmlError error;
	Rid document;

	/* loading: a clean error */
	EXPECT_ERRORS_BEGIN();
	CHECK(Rwidget_create_from_xml(text, size) == R_INVALID_ID);
	EXPECT_ERRORS_END(1);
	CHECK(Rerror_get_last(&detail, NULL) == R_ERROR_XML);
	CHECK(detail && strstr(detail, "nested too deeply") != NULL);

	/* a document to edit: the same, given to the caller */
	document = Rxml_document_parse(text, size, &error);
	CHECK(document == R_INVALID_ID);
	CHECK(strstr(error.message, "nested too deeply") != NULL);
	free(text);

	/* one level past the limit is refused, the limit itself is read */
	text = nested_xml(TEST_XML_DEPTH + 2, &size);
	document = Rxml_document_parse(text, size, &error);
	CHECK(document == R_INVALID_ID);
	free(text);
	text = nested_xml(TEST_XML_DEPTH + 1, &size);
	document = Rxml_document_parse(text, size, &error);
	CHECK(Rxml_document_is_valid(document));
	Rxml_document_destroy(document);
	free(text);
	CHECK(Rerror_get_last(NULL, NULL) == R_NO_ERROR);
}

/* A document built node by node stops where a text would be refused. */
static void test_xml_document_depth(void) {
	Rid document = Rxml_document_create("VBox");
	Rid root = Rxml_document_get_root(document);
	Rid node = root, last = root, before_last = root, branch, copy, again;
	const char* detail = NULL;
	const char* written;
	int added = 0;

	EXPECT_ERRORS_BEGIN();
	while (added < 100000) {
		Rid child = Rxml_node_add_child(node, "VBox", -1);
		if (child == R_INVALID_ID) break;
		before_last = node;
		last = child;
		node = child;
		added++;
	}
	EXPECT_ERRORS_END(1);
	CHECK(added == TEST_XML_DEPTH);
	CHECK(Rerror_get_last(&detail, NULL) == R_ERROR_XML);
	CHECK(detail && strstr(detail, "nested too deeply") != NULL);

	/* nothing else gets under the deepest node */
	EXPECT_ERRORS_BEGIN();
	CHECK(Rxml_node_add_comment(last, "no", -1) == R_INVALID_ID);
	CHECK(Rxml_node_add_xml(last, "<Label/>", 8, -1) == R_INVALID_ID);
	CHECK(Rxml_node_add_xml(before_last, "<HBox><Label/></HBox>", 21, -1) == R_INVALID_ID);
	EXPECT_ERRORS_END(3);
	CHECK(Rxml_node_get_child_count(last) == 0);
	CHECK(Rxml_node_get_child_count(before_last) == 1);
	CHECK(Rerror_get_last(NULL, NULL) == R_ERROR_XML);

	/* a branch moves where it fits, and only there */
	branch = Rxml_node_add_xml(root, "<HBox><Label/></HBox>", 21, -1);
	CHECK(Rxml_node_is_valid(branch));
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rxml_node_move(branch, before_last, -1));
	EXPECT_ERRORS_END(1);
	CHECK(Rerror_get_last(NULL, NULL) == R_ERROR_XML);
	CHECK(Rxml_node_get_parent(branch) == root);
	CHECK(Rxml_node_move(branch, Rxml_node_get_parent(before_last), -1));
	CHECK(Rxml_node_get_parent(branch) == Rxml_node_get_parent(before_last));

	/* what was built is copied, written and read back */
	copy = Rxml_node_duplicate(Rxml_node_get_child(root, 0));
	CHECK(Rxml_node_is_valid(copy));
	written = Rxml_document_to_string(document);
	again = Rxml_document_parse(written, strlen(written), NULL);
	CHECK(Rxml_document_is_valid(again));
	Rxml_document_destroy(again);
	Rxml_document_destroy(document);
	CHECK(!Rxml_node_is_valid(last));
	CHECK(Rerror_get_last(NULL, NULL) == R_NO_ERROR);
}

/* Operators in front of a value, by the thousand: an expression that is refused. */
static void test_expression_too_deep(void) {
	static const char head[] = "<Widget Name=\"DeepNot\" On=\"true\"><Label Name=\"deep\" Text=\"{";
	static const char tail[] = "On}\"/></Widget>";
	static const char plain[] =
		"<Widget Name=\"PlainNot\" On=\"true\" N=\"3\">"
		"<VBox><Label Name=\"not\" Text=\"{!!On}\"/><Label Name=\"minus\" Text=\"{--N}\"/><Label Name=\"one\" Text=\"{!On}/{-N}\"/></VBox>"
		"</Widget>";
	int operators = 100000, i;
	size_t size = sizeof(head) - 1 + (size_t)operators + sizeof(tail) - 1;
	char* text = (char*)malloc(size + 1);
	const char* detail = NULL;
	Rid widget;

	memcpy(text, head, sizeof(head) - 1);
	for (i = 0; i < operators; i++) text[sizeof(head) - 1 + i] = (i & 1) ? '-' : '!';
	memcpy(text + sizeof(head) - 1 + operators, tail, sizeof(tail));
	CHECK(Rxml_register_widgets(text, size));
	free(text);

	EXPECT_ERRORS_BEGIN();
	widget = Rwidget_create_from_xml("<DeepNot/>", 10);
	EXPECT_ERRORS_END(1);
	CHECK(Rerror_get_last(&detail, NULL) == R_ERROR_XML);
	CHECK(detail && strstr(detail, "nested too deeply") != NULL);
	/* (like any expression that is wrong: reported, and the text is empty) */
	CHECK(Rwidget_is_valid(widget));
	CHECK_STR(Rlabel_get_text(widget), "");
	Rwidget_destroy(widget);

	/* the same operators, in a number anyone writes, mean what they did */
	CHECK(Rxml_register_widgets(plain, sizeof(plain) - 1));
	widget = Rwidget_create_from_xml("<PlainNot/>", 11);
	CHECK(Rwidget_is_valid(widget));
	CHECK_STR(Rlabel_get_text(Rwidget_find(widget, "not")), "true");
	CHECK_STR(Rlabel_get_text(Rwidget_find(widget, "minus")), "3");
	CHECK_STR(Rlabel_get_text(Rwidget_find(widget, "one")), "false/-3");
	Rwidget_destroy(widget);
	CHECK(Rerror_get_last(NULL, NULL) == R_NO_ERROR);
}

int main(void) {
	small_stack();
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	RUN(test_destroy_deep_chain);
	RUN(test_destroy_order);
	RUN(test_xml_too_deep);
	RUN(test_xml_document_depth);
	RUN(test_expression_too_deep);

	/* what is left when the library stops is destroyed the same way */
	deep_chain(TEST_WIDGET_DEPTH, NULL);
	Rshutdown();
	return test_report("test_deep_trees");
}
