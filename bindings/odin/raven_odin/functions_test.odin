package raven_odin

import "core:testing"

// The generated procedures of functions.odin, headless: one of each kind of rule. The file itself is
// checked by `bindgen.py --check` (ctest bindings_up_to_date) and by compiling.

@(private = "file")
XML :: `<Window Title="Test" Width="200" Height="120">
	<VBox>
		<Label Name="title" Text="x"/>
		<Button Name="ok" Text="Ok"/>
	</VBox>
</Window>`

@(test)
generated_procedures_follow_the_rules :: proc(t: ^testing.T) {
	testing.expect(t, init(headless = true), "init")
	defer shutdown()

	window, built := window_create_from_xml(XML)
	testing.expect(t, built, "window")
	title := widget_find(window, "title")
	testing.expect(t, title != Widget(INVALID), "title")

	// a boolean in and out (int in C)
	testing.expect(t, widget_is_enabled(title), "enabled by default")
	widget_set_enabled(title, false)
	testing.expect(t, !widget_is_enabled(title), "disabled")
	widget_set_enabled(title, true)
	testing.expect(t, widget_is_enabled(title), "enabled again")

	// an enumeration in and out
	widget_set_visibility(title, .Collapsed)
	testing.expect_value(t, widget_get_visibility(title), Visibility.Collapsed)

	// texts: a string in and a boolean out, and a text that may be NULL, which takes nil
	widget_set_class(title, "primary")
	testing.expect(t, widget_has_class(title, "primary"), "the class is set")
	testing.expect(t, !widget_has_class(title, "other"), "another class is not")
	widget_set_class(title, nil)
	testing.expect(t, !widget_has_class(title, "primary"), "nil removes the classes")

	// numbers written through pointers come back as results
	width, height := window_get_framebuffer_size(window)
	testing.expect(t, width > 0 && height > 0, "the size of the frame buffer")

	// a handle that is returned comes with ok
	content := window_get_content(window)
	parent, has_parent := widget_get_parent(title)
	testing.expect(t, has_parent, "the label has a parent")
	_, root_has_parent := widget_get_parent(content)
	testing.expect(t, !root_has_parent, "the root has none: ok is false for R_INVALID_ID")
	testing.expect(t, widget_get_child_count(parent) >= 2, "a number of C is an i32")

	// numbers in and out, and several numbers written through pointers
	slider := widget_add_child(parent, widget_create(.Slider))
	slider_set_range(slider, 0, 10)
	slider_set_value(slider, 4)
	testing.expect_value(t, slider_get_value(slider), f32(4))
	minimum, maximum, _ := slider_get_range(slider)
	testing.expect_value(t, minimum, f32(0))
	testing.expect_value(t, maximum, f32(10))

	// handles of another kind than a widget: the items of a tree
	tree := widget_add_child(parent, widget_create(.Tree_View))
	root := tree_view_add_item(tree, Tree_Item(INVALID), "root")
	child := tree_view_add_item(tree, root, "child")
	testing.expect_value(t, tree_view_get_item_count(tree), i32(1)) // the top-level items
	testing.expect_value(t, tree_item_get_child_count(root), i32(1))
	testing.expect(t, tree_item_is_valid(child), "the child is valid")
	tree_item_set_expanded(root, true)
	testing.expect(t, tree_item_is_expanded(root), "expanded")

	// a text that is returned, and one that may be NULL
	document, _, parsed := xml_document_parse(`<Window Title="Hello"/>`)
	testing.expect(t, parsed, "the XML parses")
	window_node := xml_document_get_root(document)
	title_attribute := xml_node_get_attribute(window_node, "Title")
	testing.expect_value(t, string(title_attribute), "Hello")
	missing := xml_node_get_attribute(window_node, "Missing")
	testing.expect(t, missing == nil, "NULL for an attribute the node does not have")
	xml_document_destroy(document)
}
