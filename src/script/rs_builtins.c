/**
 * Raven Script - script/rs_builtins.c
 * What the language knows by itself: the native API of Raven (app, ui,
 * window), the events of the widgets, the methods of the values.
 *
 * The natives are declared here (name, signature, documentation) so that the
 * type checker, the command line tool and the editor know them; the library
 * runs them (script/script_raven.c).
 */

#include "rs_internal.h"

/* ---------------------------------------------------------------------------
 *  Native API
 * ------------------------------------------------------------------------- */

static const rs_native g_natives[] = {
	{ "app.log", "(message: string): void", "Writes a message to the log of the application (the standard output, or the function given to Rscript_set_log_callback)." },
	{ "app.quit", "(): void", "Quits the application (Rquit(0))." },
	{ "app.fire", "(action: string): void", "Fires an action by its name, as a widget would: a function bound with Raction_bind, a script function without parameters or a built-in action (\"show:details\", \"theme.dark\"...)." },
	{ "ui.setText", "(id: string, text: string): void", "Sets the text of the widget named `id` (its Text attribute: a label, a button, a text input...)." },
	{ "ui.getText", "(id: string): string", "The text of the widget named `id` (a label, a text input, a check box, the item chosen in a combo box or a list)." },
	{ "ui.setVisible", "(id: string, visible: bool): void", "Shows or hides (Collapsed) the widget named `id`." },
	{ "ui.isVisible", "(id: string): bool", "Whether the widget named `id` is visible." },
	{ "ui.setEnabled", "(id: string, enabled: bool): void", "Enables or disables the widget named `id`." },
	{ "ui.isEnabled", "(id: string): bool", "Whether the widget named `id` is enabled." },
	{ "ui.setValue", "(id: string, value: number): void", "Sets the value of the widget named `id`: a slider, a spin box, a rating, a progress bar, the item chosen in a combo box or a list, the page of a tab view, a pagination or a carousel." },
	{ "ui.getValue", "(id: string): number", "The value of the widget named `id` (see ui.setValue)." },
	{ "ui.setChecked", "(id: string, checked: bool): void", "Checks or unchecks the check box (or the expander, opened or closed) named `id`." },
	{ "ui.isChecked", "(id: string): bool", "Whether the check box named `id` is checked (an expander: open)." },
	{ "ui.setAttribute", "(id: string, name: string, value: string): void", "Sets any XML attribute of the widget named `id`, as written in a document (ui.setAttribute(\"title\", \"Foreground\", \"#FF0000\"))." },
	{ "ui.focus", "(id: string): void", "Gives the keyboard focus to the widget named `id`." },
	{ "ui.exists", "(id: string): bool", "Whether a widget is named `id` in the window." },
	{ "window.close", "(): void", "Closes the window of the widget that fired the event (the window of the document while it loads)." },
	{ "window.minimize", "(): void", "Minimizes the window." },
	{ "window.maximize", "(): void", "Maximizes the window, or gives it back its size when it already is." },
	{ "window.restore", "(): void", "Gives the window back its size." },
	{ "window.setTitle", "(title: string): void", "Changes the title of the window." }
};

int rs_native_count(void) {
	return (int)RS_COUNT_OF(g_natives);
}

const rs_native* rs_native_at(int index) {
	return index >= 0 && index < (int)RS_COUNT_OF(g_natives) ? &g_natives[index] : NULL;
}

const rs_native* rs_native_find(const char* name, rs_sig* sig) {
	size_t i;
	for (i = 0; i < RS_COUNT_OF(g_natives); i++) {
		if (strcmp(g_natives[i].name, name) == 0) {
			if (sig) {
				/* the names of the parameters point into a static copy per native */
				static char storage[RS_COUNT_OF(g_natives)][128];
				rs_sig_parse(g_natives[i].signature, sig, storage[i], sizeof(storage[i]), NULL, 0);
			}
			return &g_natives[i];
		}
	}
	return NULL;
}

/* ---------------------------------------------------------------------------
 *  Events of the widgets
 * ------------------------------------------------------------------------- */

static const rs_event_sig g_events[] = {
	{ "Checkbox|CheckBox", "OnChanged", "Checkbox.OnChanged", 1, { RS_T_BOOL }, { "checked" }, "when the box is checked or unchecked" },
	{ "Slider", "OnChanged", "Slider.OnChanged", 1, { RS_T_NUMBER }, { "value" }, "when the value changes" },
	{ "SpinBox", "OnChanged", "SpinBox.OnChanged", 1, { RS_T_NUMBER }, { "value" }, "when the value changes" },
	{ "Rating", "OnChanged", "Rating.OnChanged", 1, { RS_T_NUMBER }, { "value" }, "when the user changes the score" },
	{ "TextInput|TextBox|EditableText", "OnCommitted", "TextInput.OnCommitted", 1, { RS_T_STRING }, { "text" }, "when Enter is pressed" },
	{ "ComboBox", "OnChanged", "ComboBox.OnChanged", 2, { RS_T_NUMBER, RS_T_STRING }, { "index", "text" }, "when another item is chosen (-1: none)" },
	{ "ListView", "OnActivated", "ListView.OnActivated", 2, { RS_T_NUMBER, RS_T_STRING }, { "index", "text" }, "on a double click or Enter on an item" },
	{ "TabView|Tabs", "OnChanged", "TabView.OnChanged", 1, { RS_T_NUMBER }, { "index" }, "when another tab is shown" },
	{ "Pagination", "OnChanged", "Pagination.OnChanged", 1, { RS_T_NUMBER }, { "page" }, "when the user goes to another page" },
	{ "Carousel", "OnChanged", "Carousel.OnChanged", 1, { RS_T_NUMBER }, { "page" }, "when another page is shown" },
	{ "Expander|ExpandableArea", "OnChanged", "Expander.OnChanged", 1, { RS_T_BOOL }, { "expanded" }, "when it opens or closes" },
	{ "Island|Notch|DynamicIsland", "OnChanged", "Island.OnChanged", 1, { RS_T_BOOL }, { "expanded" }, "when it expands or folds" },
	{ "ColorPicker", "OnChanged", "ColorPicker.OnChanged", 1, { RS_T_STRING }, { "color" }, "when the color changes (\"#RRGGBB\" or \"#RRGGBBAA\")" },
	{ "DatePicker", "OnChanged", "DatePicker.OnChanged", 3, { RS_T_NUMBER, RS_T_NUMBER, RS_T_NUMBER }, { "year", "month", "day" }, "when the user chooses a date" },
	{ "TimePicker", "OnChanged", "TimePicker.OnChanged", 3, { RS_T_NUMBER, RS_T_NUMBER, RS_T_NUMBER }, { "hour", "minute", "second" }, "when the user changes the time" }
};

static int rs_tag_matches(const char* tags, const char* tag) {
	const char* p = tags;
	size_t length = strlen(tag);
	while (*p) {
		const char* end = strchr(p, '|');
		size_t part = end ? (size_t)(end - p) : strlen(p);
		if (part == length) {
			size_t i;
			for (i = 0; i < length; i++) {
				char a = p[i], b = tag[i];
				if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
				if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
				if (a != b) break;
			}
			if (i == length) return 1;
		}
		if (!end) break;
		p = end + 1;
	}
	return 0;
}

const rs_event_sig* rs_event_find(const char* tag) {
	size_t i;
	if (!tag) return NULL;
	for (i = 0; i < RS_COUNT_OF(g_events); i++) {
		if (rs_tag_matches(g_events[i].tags, tag)) return &g_events[i];
	}
	return NULL;
}

int rs_event_count(void) {
	return (int)RS_COUNT_OF(g_events);
}

const rs_event_sig* rs_event_at(int index) {
	return index >= 0 && index < (int)RS_COUNT_OF(g_events) ? &g_events[index] : NULL;
}

int rs_is_action_attribute(const char* name) {
	static const char* const names[] = { "Action", "OnClicked", "OnChanged", "OnCommitted", "OnActivated" };
	size_t i;
	for (i = 0; i < RS_COUNT_OF(names); i++) {
		if (rs_stricmp(names[i], name) == 0) return 1;
	}
	return 0;
}

/* ---------------------------------------------------------------------------
 *  Methods
 * ------------------------------------------------------------------------- */

static const rs_method_info g_methods[RS_M_COUNT_] = {
	{ "toString", 0, "The value as text (numbers: the shortest text that reads back as the same number)." },
	{ "length", 1, "Number of items of an array, of characters of a string." },
	{ "push", 0, "Adds an item at the end of the array." },
	{ "pop", 0, "Removes the last item of the array and returns it (an error when the array is empty)." },
	{ "toUpperCase", 0, "The text in capital letters." },
	{ "toLowerCase", 0, "The text in small letters." },
	{ "trim", 0, "The text without the white space at its ends." },
	{ "contains", 0, "Whether the text contains another." },
	{ "startsWith", 0, "Whether the text starts with another." },
	{ "endsWith", 0, "Whether the text ends with another." },
	{ "indexOf", 0, "Position (in characters) of the first occurrence of a text, -1 when there is none." },
	{ "substring", 0, "The characters from `start` to `end` (excluded); positions outside the text are brought back inside." },
	{ "split", 0, "The parts of the text between the separators." },
	{ "toFixed", 0, "The number with `digits` digits after the decimal point (0 to 20)." }
};

const rs_method_info* rs_method_info_of(int method) {
	return method >= 0 && method < RS_M_COUNT_ ? &g_methods[method] : NULL;
}

static void rs_sig_set(rs_sig* sig, rs_type ret, int count, rs_type a, rs_type b, const char* name_a, const char* name_b) {
	memset(sig, 0, sizeof(*sig));
	sig->ret = ret;
	sig->count = count;
	sig->params[0] = a;
	sig->params[1] = b;
	sig->names[0] = name_a;
	sig->names[1] = name_b;
}

int rs_method_lookup(rs_type receiver, const char* name, rs_sig* sig) {
	rs_type number = rs_type_make(RS_T_NUMBER, 0), string = rs_type_make(RS_T_STRING, 0);
	rs_type boolean = rs_type_make(RS_T_BOOL, 0), none = rs_type_make(RS_T_VOID, 0);
	rs_sig local;
	if (!sig) sig = &local;
	if (rs_type_is_error(receiver)) return -1;
	if (strcmp(name, "toString") == 0 && (receiver.depth == 0 && (receiver.base == RS_T_NUMBER || receiver.base == RS_T_BOOL || receiver.base == RS_T_STRING))) {
		rs_sig_set(sig, string, 0, none, none, NULL, NULL);
		return RS_M_TO_STRING;
	}
	if (receiver.depth > 0) {
		rs_type element = rs_type_element(receiver);
		if (strcmp(name, "length") == 0) { rs_sig_set(sig, number, 0, none, none, NULL, NULL); return RS_M_LENGTH; }
		if (strcmp(name, "push") == 0) { rs_sig_set(sig, none, 1, element, none, "item", NULL); return RS_M_PUSH; }
		if (strcmp(name, "pop") == 0) { rs_sig_set(sig, element, 0, none, none, NULL, NULL); return RS_M_POP; }
		return -1;
	}
	if (receiver.base == RS_T_NUMBER) {
		if (strcmp(name, "toFixed") == 0) { rs_sig_set(sig, string, 1, number, none, "digits", NULL); return RS_M_TO_FIXED; }
		return -1;
	}
	if (receiver.base != RS_T_STRING) return -1;
	if (strcmp(name, "length") == 0) { rs_sig_set(sig, number, 0, none, none, NULL, NULL); return RS_M_LENGTH; }
	if (strcmp(name, "toUpperCase") == 0) { rs_sig_set(sig, string, 0, none, none, NULL, NULL); return RS_M_TO_UPPER; }
	if (strcmp(name, "toLowerCase") == 0) { rs_sig_set(sig, string, 0, none, none, NULL, NULL); return RS_M_TO_LOWER; }
	if (strcmp(name, "trim") == 0) { rs_sig_set(sig, string, 0, none, none, NULL, NULL); return RS_M_TRIM; }
	if (strcmp(name, "contains") == 0) { rs_sig_set(sig, boolean, 1, string, none, "text", NULL); return RS_M_CONTAINS; }
	if (strcmp(name, "startsWith") == 0) { rs_sig_set(sig, boolean, 1, string, none, "text", NULL); return RS_M_STARTS_WITH; }
	if (strcmp(name, "endsWith") == 0) { rs_sig_set(sig, boolean, 1, string, none, "text", NULL); return RS_M_ENDS_WITH; }
	if (strcmp(name, "indexOf") == 0) { rs_sig_set(sig, number, 1, string, none, "text", NULL); return RS_M_INDEX_OF; }
	if (strcmp(name, "substring") == 0) { rs_sig_set(sig, string, 2, number, number, "start", "end"); return RS_M_SUBSTRING; }
	if (strcmp(name, "split") == 0) { rs_sig_set(sig, rs_type_make(RS_T_STRING, 1), 1, string, none, "separator", NULL); return RS_M_SPLIT; }
	return -1;
}

/* ---------------------------------------------------------------------------
 *  Intrinsics
 * ------------------------------------------------------------------------- */

static const rs_intrinsic_info g_intrinsics[RS_I_COUNT_] = {
	{ "number.parse", "(text: string): number", "Reads a number from a text (\"42\", \"-3.5\", \"1e3\", \"0x1F\"); an error (RS104) when the text is not a number." }
};

const rs_intrinsic_info* rs_intrinsic_info_of(int intrinsic) {
	return intrinsic >= 0 && intrinsic < RS_I_COUNT_ ? &g_intrinsics[intrinsic] : NULL;
}

int rs_intrinsic_find(const char* name, rs_sig* sig) {
	int i;
	for (i = 0; i < RS_I_COUNT_; i++) {
		if (strcmp(g_intrinsics[i].name, name) == 0) {
			if (sig) {
				static char storage[RS_I_COUNT_][64];
				rs_sig_parse(g_intrinsics[i].signature, sig, storage[i], sizeof(storage[i]), NULL, 0);
			}
			return i;
		}
	}
	return -1;
}

/* ---------------------------------------------------------------------------
 *  What the editor shows of the language
 * ------------------------------------------------------------------------- */

char* rs_language_json(void) {
	rs_buf out = { 0 };
	size_t i;
	int j;
	rs_buf_printf(&out, "{\"version\":");
	rs_buf_json_string(&out, RS_LANGUAGE_VERSION);
	rs_buf_puts(&out, ",\"natives\":[");
	for (i = 0; i < RS_COUNT_OF(g_natives); i++) {
		if (i) rs_buf_putc(&out, ',');
		rs_buf_puts(&out, "{\"name\":");
		rs_buf_json_string(&out, g_natives[i].name);
		rs_buf_puts(&out, ",\"signature\":");
		rs_buf_json_string(&out, g_natives[i].signature);
		rs_buf_puts(&out, ",\"doc\":");
		rs_buf_json_string(&out, g_natives[i].doc);
		rs_buf_putc(&out, '}');
	}
	for (j = 0; j < RS_I_COUNT_; j++) {
		rs_buf_puts(&out, ",{\"name\":");
		rs_buf_json_string(&out, g_intrinsics[j].name);
		rs_buf_puts(&out, ",\"signature\":");
		rs_buf_json_string(&out, g_intrinsics[j].signature);
		rs_buf_puts(&out, ",\"doc\":");
		rs_buf_json_string(&out, g_intrinsics[j].doc);
		rs_buf_putc(&out, '}');
	}
	rs_buf_puts(&out, "],\"events\":[");
	for (i = 0; i < RS_COUNT_OF(g_events); i++) {
		const rs_event_sig* event = &g_events[i];
		if (i) rs_buf_putc(&out, ',');
		rs_buf_puts(&out, "{\"tags\":");
		rs_buf_json_string(&out, event->tags);
		rs_buf_puts(&out, ",\"event\":");
		rs_buf_json_string(&out, event->event);
		rs_buf_puts(&out, ",\"description\":");
		rs_buf_json_string(&out, event->description);
		rs_buf_puts(&out, ",\"parameters\":[");
		for (j = 0; j < event->count; j++) {
			if (j) rs_buf_putc(&out, ',');
			rs_buf_puts(&out, "{\"name\":");
			rs_buf_json_string(&out, event->names[j]);
			rs_buf_puts(&out, ",\"type\":");
			rs_buf_json_string(&out, rs_type_name(rs_type_make(event->types[j], 0)));
			rs_buf_putc(&out, '}');
		}
		rs_buf_puts(&out, "]}");
	}
	rs_buf_puts(&out, "]}");
	return rs_buf_take(&out);
}
