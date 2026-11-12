/**
 * Raven example - components
 * Tags of your own, and documents built from C.
 *
 *   <Widget Name="Card" ...>   declares a tag, with parameters; then
 *   <Card Title="Sales">       is used like any widget of the library
 *                              (components.xml, widgets.xml)
 *   <Repeat>, <ForEach>, If=   repeat and choose
 *   <Include Source="...">     declarations kept in their own file
 *
 * From C, the "Add a card" button builds a piece of interface as a document
 * (Rxml_document_..., Rxml_node_...) instead of a text: it adds elements and
 * attributes, asks the library for the widgets, and shows the XML it wrote.
 * Rxml_get_registered_widget_name lists the declared tags.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window, cards, status, source;
	int added;
} app;

static void action_add(Rid sender, const char* action, void* user_data) {
	static const char* const titles[] = { "Builds", "Tickets", "Reviews", "Deploys" };
	char value[32], label[32];
	Rid document, card, stat, button, widget;
	int i;
	(void)sender;
	(void)action;
	(void)user_data;

	/* <Card Title="..." Badge="new" Appear="Drop"> <Stat .../> <Stat .../> <Button Into="footer"/> </Card> */
	document = Rxml_document_create("Card");
	card = Rxml_document_get_root(document);
	Rxml_node_set_attribute(card, "Title", titles[app.added % 4]);
	Rxml_node_set_attribute(card, "Badge", "new");
	Rxml_node_set_attribute(card, "Appear", "Drop");
	for (i = 0; i < 2; i++) {
		stat = Rxml_node_add_child(card, "Stat", -1);
		snprintf(label, sizeof(label), i == 0 ? "Open" : "Closed today");
		snprintf(value, sizeof(value), "%d", 3 + (app.added * 7 + i * 11) % 40);
		Rxml_node_set_attribute(stat, "Label", label);
		Rxml_node_set_attribute(stat, "Value", value);
		Rxml_node_set_attribute(stat, "Trend", i == 0 ? "-1" : "1");
	}
	button = Rxml_node_add_child(card, "Button", -1);
	Rxml_node_set_attribute(button, "Text", "Remove");
	Rxml_node_set_attribute(button, "Style", "Flat");
	Rxml_node_set_attribute(button, "Into", "footer");
	Rxml_node_set_attribute(button, "OnClicked", "card.remove");

	/* the document becomes widgets: <Card> is expanded as it would be from a file */
	widget = Rxml_document_create_widget(document);
	if (widget != R_INVALID_ID) {
		Rwidget_add_child(app.cards, widget);
		app.added++;
		/* and it can be written back as text */
		Rtext_editor_set_text(app.source, Rxml_document_to_string(document));
		snprintf(value, sizeof(value), "%d", app.added);
		Rlabel_set_text(app.status, app.added == 1 ? "1 card added from C." : "Several cards added from C; \"Remove\" takes one away.");
	}
	Rxml_document_destroy(document);
}

/* The button is inside the card it removes: climb to the child of the list of cards. */
static void action_remove(Rid sender, const char* action, void* user_data) {
	Rid widget = sender;
	(void)action;
	(void)user_data;
	while (widget != R_INVALID_ID && Rwidget_get_parent(widget) != app.cards) widget = Rwidget_get_parent(widget);
	if (widget != R_INVALID_ID) Rwidget_destroy(widget);
}

static void action_details(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	Rlabel_set_text(app.status, "A button given to a card with Into=\"footer\" was clicked.");
}

int main(void) {
	char text[256];
	int i, length;

	if (!example_init("components")) return 1;

	Raction_bind("cards.add", action_add, NULL);
	Raction_bind("card.remove", action_remove, NULL);
	Raction_bind("card.details", action_details, NULL);

	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/components.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.cards = Rwidget_find(app.window, "cards");
	app.status = Rwidget_find(app.window, "status");
	app.source = Rwidget_find(app.window, "source");

	/* the tags the documents declared are known to the library from now on */
	length = snprintf(text, sizeof(text), "Declared tags:");
	for (i = 0; i < Rxml_get_registered_widget_count() && length < (int)sizeof(text) - 40; i++) {
		length += snprintf(text + length, sizeof(text) - (size_t)length, " <%s>", Rxml_get_registered_widget_name(i));
	}
	Rlabel_set_text(app.status, text);
	Rtext_editor_set_text(app.source, "<!-- press \"Add a card\" -->");

	example_run(app.window, NULL, NULL);
	return 0;
}
