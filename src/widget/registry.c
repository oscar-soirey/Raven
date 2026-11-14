/**
 * Raven - widget/registry.c
 * Table of the widget classes. A new widget type is listed here once.
 */

#include "../widgets/common.h"

static const rvn_widget_class_t* const g_classes[] = {
	/* panels */
	&rvn_class_vbox,
	&rvn_class_hbox,
	&rvn_class_grid,
	&rvn_class_uniform_grid,
	&rvn_class_overlay,
	&rvn_class_border,
	&rvn_class_size_box,
	&rvn_class_spacer,
	&rvn_class_scroll_box,
	&rvn_class_splitter,
	&rvn_class_wrap_box,
	&rvn_class_canvas,
	&rvn_class_switcher,
	&rvn_class_tab_view,
	&rvn_class_expander,
	/* controls */
	&rvn_class_label,
	&rvn_class_button,
	&rvn_class_checkbox,
	&rvn_class_slider,
	&rvn_class_progress_bar,
	&rvn_class_image,
	&rvn_class_text_input,
	&rvn_class_spin_box,
	&rvn_class_combo_box,
	&rvn_class_separator,
	&rvn_class_list_view,
	&rvn_class_tree_view,
	&rvn_class_menu_bar,
	&rvn_class_menu,
	&rvn_class_menu_item,
	&rvn_class_viewport,
	&rvn_class_custom,
	&rvn_class_text_editor,
	&rvn_class_header_bar,
	&rvn_class_file_browser,
	&rvn_class_dock_area,
	&rvn_class_island,
	&rvn_class_background_blur,
	&rvn_class_color_picker,
	&rvn_class_timeline,
	&rvn_class_node_graph,
	&rvn_class_tile_view,
	&rvn_class_block_editor,
	&rvn_class_table,
	&rvn_class_date_picker,
	&rvn_class_time_picker,
	&rvn_class_toast,
	&rvn_class_breadcrumb,
	&rvn_class_rating,
	&rvn_class_pagination,
	&rvn_class_plot,
	&rvn_class_pdf_view
};

/** Other tags accepted in XML, for people coming from Slate / UMG. */
static const struct { const char* alias; RWidgetType type; } g_aliases[] = {
	{ "VerticalBox", R_WIDGET_VBOX },
	{ "HorizontalBox", R_WIDGET_HBOX },
	{ "GridPanel", R_WIDGET_GRID },
	{ "UniformGridPanel", R_WIDGET_UNIFORM_GRID },
	{ "Box", R_WIDGET_SIZE_BOX },
	{ "CanvasPanel", R_WIDGET_CANVAS },
	{ "WidgetSwitcher", R_WIDGET_SWITCHER },
	{ "Tabs", R_WIDGET_TAB_VIEW },
	{ "ExpandableArea", R_WIDGET_EXPANDER },
	{ "Text", R_WIDGET_LABEL },
	{ "TextBlock", R_WIDGET_LABEL },
	{ "CheckBox", R_WIDGET_CHECKBOX },
	{ "EditableText", R_WIDGET_TEXT_INPUT },
	{ "TextBox", R_WIDGET_TEXT_INPUT },
	{ "TextArea", R_WIDGET_TEXT_EDITOR },
	{ "MultiLineEditableText", R_WIDGET_TEXT_EDITOR },
	{ "TitleBar", R_WIDGET_HEADER_BAR },
	{ "Notch", R_WIDGET_ISLAND },
	{ "DynamicIsland", R_WIDGET_ISLAND },
	{ "DataGrid", R_WIDGET_TABLE },
	{ "TableView", R_WIDGET_TABLE },
	{ "Snackbar", R_WIDGET_TOAST },
	{ "Chart", R_WIDGET_PLOT },
	{ "LinePlot", R_WIDGET_PLOT },
	{ "XYPlot", R_WIDGET_PLOT },
	{ "PdfViewer", R_WIDGET_PDF_VIEW },
	{ "PDFView", R_WIDGET_PDF_VIEW }
};

const rvn_widget_class_t* rvn_widget_class_by_type(RWidgetType type) {
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(g_classes); i++) {
		if (g_classes[i]->type == type) return g_classes[i];
	}
	return NULL;
}

const rvn_widget_class_t* rvn_widget_class_by_name(const char* name) {
	size_t i;
	if (!name) return NULL;
	for (i = 0; i < RVN_COUNT_OF(g_classes); i++) {
		if (rvn_stricmp(g_classes[i]->name, name) == 0) return g_classes[i];
	}
	for (i = 0; i < RVN_COUNT_OF(g_aliases); i++) {
		if (rvn_stricmp(g_aliases[i].alias, name) == 0) return rvn_widget_class_by_type(g_aliases[i].type);
	}
	return NULL;
}
