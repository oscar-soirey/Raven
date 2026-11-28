/**
 * Raven - core/mermaid.c
 * Mermaid diagrams: the text read into a model (see core/mermaid.h), the
 * scene that the layouts fill, its summary for screen readers, and the scene
 * written as SVG.
 *
 * The text is read line by line (a flowchart also splits its lines at the
 * semicolons outside of texts). Each kind of diagram has its own small
 * reader; they share the names of the nodes (a table of hashes), the
 * styles (style, classDef, class, :::), the comments, the directives and
 * the accessible title and description. The first error stops the reading:
 * its line and the reason are kept, and the scene then shows them.
 */

#include "mermaid.h"

#include <ctype.h>

/* ---------------------------------------------------------------------------
 *  Small texts
 * ------------------------------------------------------------------------- */

static int rvn_mm_space(char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

static int rvn_mm_alnum(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

/* A character of a name: letters, digits, '_', and every byte of a character that is not ASCII. */
static int rvn_mm_id_char(char c) {
	return rvn_mm_alnum(c) || c == '_' || (unsigned char)c >= 0x80u;
}

static const char* rvn_mm_skip(const char* p, const char* end) {
	while (p < end && rvn_mm_space(*p)) p++;
	return p;
}

static void rvn_mm_trim(const char** start, const char** end) {
	while (*start < *end && rvn_mm_space(**start)) (*start)++;
	while (*end > *start && rvn_mm_space((*end)[-1])) (*end)--;
}

/* The word at p, compared without case, followed by a space or the end. */
static int rvn_mm_word(const char* p, const char* end, const char* word) {
	size_t n = strlen(word);
	if ((size_t)(end - p) < n || rvn_strnicmp(p, word, n) != 0) return R_FALSE;
	return p + n == end || rvn_mm_space(p[n]);
}

static int rvn_mm_starts(const char* p, const char* end, const char* prefix) {
	size_t n = strlen(prefix);
	return (size_t)(end - p) >= n && memcmp(p, prefix, n) == 0;
}

static const char* rvn_mm_find(const char* p, const char* end, const char* needle) {
	size_t n = strlen(needle);
	while ((size_t)(end - p) >= n) {
		if (memcmp(p, needle, n) == 0) return p;
		p++;
	}
	return NULL;
}

/*
 * A label as it is shown: the quotes around it removed, <br> as a line
 * break, the entities of Mermaid (#quot; #35;) and of HTML (&amp;) read,
 * the backquotes of a Markdown string dropped. At most RVN_MM_MAX_LABEL bytes.
 */
static char* rvn_mm_label(const char* start, const char* end) {
	rvn_text_t out = { NULL, 0, 0 };
	const char* p;
	rvn_mm_trim(&start, &end);
	if (end - start >= 2 && *start == '"' && end[-1] == '"') {
		start++;
		end--;
	}
	if (end - start >= 2 && *start == '`' && end[-1] == '`') {
		start++;
		end--;
	}
	for (p = start; p < end && out.length < RVN_MM_MAX_LABEL;) {
		if (*p == '<' && (rvn_mm_starts(p, end, "<br>") || rvn_mm_starts(p, end, "<br/>") || rvn_mm_starts(p, end, "<br />") ||
			rvn_mm_starts(p, end, "<BR>"))) {
			while (p < end && *p != '>') p++;
			p++;
			rvn_text_append(&out, "\n", 1);
			continue;
		}
		if (*p == '\\' && p + 1 < end && p[1] == 'n') {
			rvn_text_append(&out, "\n", 1);
			p += 2;
			continue;
		}
		if ((*p == '#' || *p == '&') && p + 2 < end) {
			const char* semi = p + 1;
			while (semi < end && semi - p < 12 && *semi != ';') semi++;
			if (semi < end && *semi == ';') {
				char name[12];
				size_t n = (size_t)(semi - p - 1);
				const char* replace = NULL;
				char utf8[4];
				memcpy(name, p + 1, n);
				name[n] = '\0';
				if (strcmp(name, "quot") == 0) replace = "\"";
				else if (strcmp(name, "amp") == 0) replace = "&";
				else if (strcmp(name, "lt") == 0) replace = "<";
				else if (strcmp(name, "gt") == 0) replace = ">";
				else if (strcmp(name, "nbsp") == 0) replace = " ";
				else if (strcmp(name, "apos") == 0) replace = "'";
				else if (n > 0 && (isdigit((unsigned char)name[0]) || (name[0] == '#' && n > 1))) {
					long code = name[0] == '#' ? strtol(name + 1, NULL, 10) : strtol(name, NULL, 10);
					if (code > 0 && code <= 0x10FFFF) {
						int k = rvn_utf8_encode((uint32_t)code, utf8);
						rvn_text_append(&out, utf8, (size_t)k);
						p = semi + 1;
						continue;
					}
				}
				if (replace) {
					rvn_text_add(&out, replace);
					p = semi + 1;
					continue;
				}
			}
		}
		rvn_text_append(&out, p, 1);
		p++;
	}
	if (!out.data) rvn_text_append(&out, "", 0);
	/* a character cut at the limit is taken away */
	while (out.length > 0 && ((unsigned char)out.data[out.length - 1] & 0xC0u) == 0x80u && out.length >= RVN_MM_MAX_LABEL) out.data[--out.length] = '\0';
	return out.data;
}

/* ---------------------------------------------------------------------------
 *  Colors
 * ------------------------------------------------------------------------- */

static int rvn_mm_hex_digit(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

int rvn_mm_parse_color(const char* text, int length, rvn_color_t* color) {
	static const struct { const char* name; uint32_t rgb; } names[] = {
		{ "black", 0x000000 }, { "white", 0xFFFFFF }, { "red", 0xFF0000 }, { "green", 0x008000 }, { "blue", 0x0000FF },
		{ "yellow", 0xFFFF00 }, { "orange", 0xFFA500 }, { "purple", 0x800080 }, { "pink", 0xFFC0CB }, { "gray", 0x808080 },
		{ "grey", 0x808080 }, { "lightgray", 0xD3D3D3 }, { "lightgrey", 0xD3D3D3 }, { "darkgray", 0xA9A9A9 }, { "silver", 0xC0C0C0 },
		{ "lightblue", 0xADD8E6 }, { "lightgreen", 0x90EE90 }, { "lightyellow", 0xFFFFE0 }, { "lightpink", 0xFFB6C1 },
		{ "darkblue", 0x00008B }, { "darkgreen", 0x006400 }, { "darkred", 0x8B0000 }, { "navy", 0x000080 }, { "teal", 0x008080 },
		{ "olive", 0x808000 }, { "maroon", 0x800000 }, { "lime", 0x00FF00 }, { "aqua", 0x00FFFF }, { "cyan", 0x00FFFF },
		{ "magenta", 0xFF00FF }, { "fuchsia", 0xFF00FF }, { "gold", 0xFFD700 }, { "coral", 0xFF7F50 }, { "salmon", 0xFA8072 },
		{ "tomato", 0xFF6347 }, { "violet", 0xEE82EE }, { "indigo", 0x4B0082 }, { "brown", 0xA52A2A }, { "beige", 0xF5F5DC },
		{ "ivory", 0xFFFFF0 }, { "khaki", 0xF0E68C }, { "lavender", 0xE6E6FA }, { "skyblue", 0x87CEEB }, { "steelblue", 0x4682B4 },
		{ "aliceblue", 0xF0F8FF }, { "honeydew", 0xF0FFF0 }, { "mintcream", 0xF5FFFA }, { "wheat", 0xF5DEB3 }, { "tan", 0xD2B48C },
		{ "chocolate", 0xD2691E }, { "crimson", 0xDC143C }, { "orchid", 0xDA70D6 }, { "plum", 0xDDA0DD }, { "turquoise", 0x40E0D0 }
	};
	const char* end;
	size_t i;
	if (!text) return R_FALSE;
	if (length < 0) length = (int)strlen(text);
	end = text + length;
	rvn_mm_trim(&text, &end);
	length = (int)(end - text);
	if (length <= 0) return R_FALSE;
	if (text[0] == '#') {
		int digits[8], n = length - 1, k;
		if (n != 3 && n != 4 && n != 6 && n != 8) return R_FALSE;
		for (k = 0; k < n; k++) {
			digits[k] = rvn_mm_hex_digit(text[1 + k]);
			if (digits[k] < 0) return R_FALSE;
		}
		if (n <= 4) {
			*color = rvn_rgba8((uint32_t)(digits[0] * 17), (uint32_t)(digits[1] * 17), (uint32_t)(digits[2] * 17), n == 4 ? (uint32_t)(digits[3] * 17) : 255u);
		} else {
			*color = rvn_rgba8((uint32_t)(digits[0] * 16 + digits[1]), (uint32_t)(digits[2] * 16 + digits[3]), (uint32_t)(digits[4] * 16 + digits[5]),
				n == 8 ? (uint32_t)(digits[6] * 16 + digits[7]) : 255u);
		}
		return R_TRUE;
	}
	if (length > 4 && rvn_strnicmp(text, "rgb", 3) == 0) {
		float values[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		const char* p = text + 3;
		int count = 0;
		if (p < end && (*p == 'a' || *p == 'A')) p++;
		if (p >= end || *p != '(') return R_FALSE;
		p++;
		while (p < end && count < 4) {
			char* stop;
			char number[32];
			size_t n = 0;
			p = rvn_mm_skip(p, end);
			while (p + n < end && n < sizeof(number) - 1 && (isdigit((unsigned char)p[n]) || p[n] == '.' || p[n] == '-')) n++;
			if (n == 0) break;
			memcpy(number, p, n);
			number[n] = '\0';
			values[count++] = strtof(number, &stop);
			p += n;
			p = rvn_mm_skip(p, end);
			if (p < end && *p == '%') {
				values[count - 1] *= count <= 3 ? 2.55f : 0.01f;
				p++;
			}
			p = rvn_mm_skip(p, end);
			if (p < end && (*p == ',' || *p == '/')) p++;
		}
		if (count < 3) return R_FALSE;
		*color = rvn_rgba(values[0] / 255.0f, values[1] / 255.0f, values[2] / 255.0f, values[3] > 1.0f ? values[3] / 255.0f : values[3]);
		return R_TRUE;
	}
	for (i = 0; i < RVN_COUNT_OF(names); i++) {
		if (strlen(names[i].name) == (size_t)length && rvn_strnicmp(text, names[i].name, (size_t)length) == 0) {
			*color = rvn_hex(names[i].rgb, 255);
			return R_TRUE;
		}
	}
	if (length == 11 && rvn_strnicmp(text, "transparent", 11) == 0) {
		*color = rvn_rgba8(0, 0, 0, 1);
		return R_TRUE;
	}
	return R_FALSE;
}

/* "fill:#f9f,stroke:#333,stroke-width:4px": the properties given are written over `style`. */
static void rvn_mm_parse_style(const char* p, const char* end, rvn_mm_style_t* style) {
	while (p < end) {
		const char* item_end = p;
		const char* colon;
		int depth = 0;
		/* (the commas inside rgb(...) do not separate) */
		while (item_end < end && (*item_end != ',' || depth > 0)) {
			if (*item_end == '(') depth++;
			else if (*item_end == ')' && depth > 0) depth--;
			item_end++;
		}
		colon = p;
		while (colon < item_end && *colon != ':') colon++;
		if (colon < item_end) {
			const char* key = p;
			const char* key_end = colon;
			const char* value = colon + 1;
			const char* value_end = item_end;
			rvn_mm_trim(&key, &key_end);
			rvn_mm_trim(&value, &value_end);
			if (value_end > value && value_end[-1] == ';') value_end--;
			if (key_end - key == 4 && rvn_strnicmp(key, "fill", 4) == 0) rvn_mm_parse_color(value, (int)(value_end - value), &style->fill);
			else if (key_end - key == 6 && rvn_strnicmp(key, "stroke", 6) == 0) rvn_mm_parse_color(value, (int)(value_end - value), &style->stroke);
			else if (key_end - key == 5 && rvn_strnicmp(key, "color", 5) == 0) rvn_mm_parse_color(value, (int)(value_end - value), &style->color);
			else if (key_end - key == 12 && rvn_strnicmp(key, "stroke-width", 12) == 0) {
				float width = strtof(value, NULL);
				if (width > 0.0f) style->stroke_width = rvn_minf(width, 12.0f);
			} else if (key_end - key == 16 && rvn_strnicmp(key, "stroke-dasharray", 16) == 0) {
				style->dashed = (uint8_t)(strtof(value, NULL) > 0.0f);
			}
		}
		p = item_end < end ? item_end + 1 : end;
	}
}

/* `over` given its values over `style`. */
static void rvn_mm_style_merge(rvn_mm_style_t* style, const rvn_mm_style_t* over) {
	if (rvn_color_a(over->fill)) style->fill = over->fill;
	if (rvn_color_a(over->stroke)) style->stroke = over->stroke;
	if (rvn_color_a(over->color)) style->color = over->color;
	if (over->stroke_width > 0.0f) style->stroke_width = over->stroke_width;
	if (over->dashed) style->dashed = over->dashed;
}

/* ---------------------------------------------------------------------------
 *  The reader
 * ------------------------------------------------------------------------- */

typedef struct rvn_mm_class_t {
	char* name;
	rvn_mm_style_t style;
} rvn_mm_class_t;

/** A class or a style given to a node, applied once everything is read. */
typedef struct rvn_mm_apply_t {
	int node;
	/** Index in the classes, or -1 for `style`. */
	char* name;
	rvn_mm_style_t style;
} rvn_mm_apply_t;

typedef struct rvn_mm_reader_t {
	rvn_mm_t* mm;
	/* the names of the nodes: an open addressing table of indices + 1 */
	int* table;
	uint32_t table_size;
	/* the line being read */
	int line;
	const char* line_start;
	const char* line_end;
	/* clusters open (subgraphs, composite states, boxes, blocks) */
	int stack[64];
	int depth;
	/* sequence diagrams: what each open `end` closes (0 block, 1 box) */
	uint8_t stack_kind[64];
	rvn_mm_class_t* classes;
	uint32_t class_count, class_cap;
	rvn_mm_apply_t* applies;
	uint32_t apply_count, apply_cap;
	/* accDescr { ... } over several lines */
	int in_description;
	rvn_text_t description;
	/* a note of a state diagram over several lines */
	int note_node;
	rvn_text_t note;
	int counter;
} rvn_mm_reader_t;

static void rvn_mm_error(rvn_mm_reader_t* r, const char* format, ...) {
	rvn_mm_t* mm = r->mm;
	char message[256];
	va_list args;
	if (mm->error) return;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);
	mm->error = rvn_strdup(message);
	mm->error_line = r->line > 0 ? r->line : 1;
	if (r->line_start && r->line_end > r->line_start) {
		const char* s = r->line_start;
		const char* e = r->line_end;
		rvn_mm_trim(&s, &e);
		if (e - s > 160) e = s + 160;
		mm->error_text = rvn_strndup(s, (size_t)(e - s));
	}
}

/* A short excerpt of a text, for the messages of errors. */
static void rvn_mm_excerpt(const char* p, const char* end, char* out, size_t size) {
	size_t n = 0;
	while (p < end && n + 1 < size && n < 24) out[n++] = *p++;
	out[n] = '\0';
	if (p < end && n >= 3) memcpy(out + n - 3, "...", 4);
}

static uint32_t rvn_mm_hash(const char* text, size_t length) {
	uint32_t h = 2166136261u;
	size_t i;
	for (i = 0; i < length; i++) h = (h ^ (uint8_t)text[i]) * 16777619u;
	return h;
}

static void rvn_mm_table_put(rvn_mm_reader_t* r, int index) {
	const char* id = r->mm->nodes[index].id;
	uint32_t slot = rvn_mm_hash(id, strlen(id)) & (r->table_size - 1u);
	while (r->table[slot]) slot = (slot + 1u) & (r->table_size - 1u);
	r->table[slot] = index + 1;
}

static int rvn_mm_find_node(rvn_mm_reader_t* r, const char* id, size_t length) {
	uint32_t slot;
	if (!r->table) return -1;
	slot = rvn_mm_hash(id, length) & (r->table_size - 1u);
	while (r->table[slot]) {
		const char* name = r->mm->nodes[r->table[slot] - 1].id;
		if (strlen(name) == length && memcmp(name, id, length) == 0) return r->table[slot] - 1;
		slot = (slot + 1u) & (r->table_size - 1u);
	}
	return -1;
}

static int rvn_mm_in_cluster(const rvn_mm_t* mm, int cluster, int ancestor);

/* The node of a name, made the first time; it goes into the cluster open, unless it is in one inside it already. */
static int rvn_mm_node(rvn_mm_reader_t* r, const char* id, size_t length) {
	rvn_mm_t* mm = r->mm;
	int index = rvn_mm_find_node(r, id, length);
	int cluster = r->depth > 0 ? r->stack[r->depth - 1] : -1;
	if (index < 0) {
		rvn_mm_node_t* node;
		if (mm->node_count >= RVN_MM_MAX_NODES) {
			rvn_mm_error(r, "too many nodes (more than %d)", RVN_MM_MAX_NODES);
			return -1;
		}
		RVN_ARRAY_RESERVE(mm->nodes, mm->node_cap, mm->node_count + 1);
		node = &mm->nodes[mm->node_count];
		memset(node, 0, sizeof(*node));
		node->id = rvn_strndup(id, length);
		node->label = rvn_strndup(id, length);
		node->shape = RVN_MM_RECT;
		node->cluster = cluster;
		index = (int)mm->node_count++;
		/* the table stays at most half full */
		if (mm->node_count * 2u > r->table_size) {
			uint32_t i;
			rvn_free(r->table);
			r->table_size = r->table_size ? r->table_size * 2u : 64u;
			r->table = (int*)rvn_calloc(sizeof(int) * r->table_size);
			for (i = 0; i < mm->node_count; i++) rvn_mm_table_put(r, (int)i);
		} else {
			rvn_mm_table_put(r, index);
		}
	} else if (cluster >= 0 && mm->nodes[index].cluster != cluster && rvn_mm_in_cluster(mm, cluster, mm->nodes[index].cluster)) {
		/* named again in a subgraph inside the one it was in: it moves there */
		mm->nodes[index].cluster = cluster;
	}
	return index;
}

/* R_TRUE when `cluster` is `ancestor` or inside it (-1: the whole diagram). */
static int rvn_mm_in_cluster(const rvn_mm_t* mm, int cluster, int ancestor) {
	int guard = 0;
	if (ancestor < 0) return R_TRUE;
	while (cluster >= 0 && guard++ < 1000) {
		if (cluster == ancestor) return R_TRUE;
		cluster = mm->clusters[cluster].parent;
	}
	return R_FALSE;
}

static int rvn_mm_add_edge(rvn_mm_reader_t* r, int from, int to) {
	rvn_mm_t* mm = r->mm;
	rvn_mm_edge_t* edge;
	if (mm->edge_count >= RVN_MM_MAX_EDGES) {
		rvn_mm_error(r, "too many links (more than %d)", RVN_MM_MAX_EDGES);
		return -1;
	}
	RVN_ARRAY_RESERVE(mm->edges, mm->edge_cap, mm->edge_count + 1);
	edge = &mm->edges[mm->edge_count];
	memset(edge, 0, sizeof(*edge));
	edge->from = from;
	edge->to = to;
	edge->head_end = RVN_MM_HEAD_ARROW;
	edge->length = 1;
	return (int)mm->edge_count++;
}

static int rvn_mm_add_cluster(rvn_mm_reader_t* r, char* id, char* title) {
	rvn_mm_t* mm = r->mm;
	rvn_mm_cluster_t* cluster;
	if (r->depth >= (int)RVN_COUNT_OF(r->stack)) {
		rvn_mm_error(r, "too many subgraphs inside each other");
		rvn_free(id);
		rvn_free(title);
		return -1;
	}
	RVN_ARRAY_RESERVE(mm->clusters, mm->cluster_cap, mm->cluster_count + 1);
	cluster = &mm->clusters[mm->cluster_count];
	memset(cluster, 0, sizeof(*cluster));
	cluster->id = id;
	cluster->title = title;
	cluster->parent = r->depth > 0 ? r->stack[r->depth - 1] : -1;
	r->stack_kind[r->depth] = 0;
	r->stack[r->depth++] = (int)mm->cluster_count;
	return (int)mm->cluster_count++;
}

static rvn_mm_event_t* rvn_mm_add_event(rvn_mm_reader_t* r, int kind) {
	rvn_mm_t* mm = r->mm;
	rvn_mm_event_t* event;
	if (mm->event_count >= RVN_MM_MAX_EDGES) {
		rvn_mm_error(r, "too many messages (more than %d)", RVN_MM_MAX_EDGES);
		return NULL;
	}
	RVN_ARRAY_RESERVE(mm->events, mm->event_cap, mm->event_count + 1);
	event = &mm->events[mm->event_count++];
	memset(event, 0, sizeof(*event));
	event->kind = (uint8_t)kind;
	event->from = event->to = -1;
	event->line_number = r->line;
	return event;
}

static void rvn_mm_add_class(rvn_mm_reader_t* r, const char* name, size_t length, const rvn_mm_style_t* style) {
	uint32_t i;
	for (i = 0; i < r->class_count; i++) {
		if (strlen(r->classes[i].name) == length && memcmp(r->classes[i].name, name, length) == 0) {
			rvn_mm_style_merge(&r->classes[i].style, style);
			return;
		}
	}
	if (r->class_count >= 1000u) return;
	RVN_ARRAY_RESERVE(r->classes, r->class_cap, r->class_count + 1);
	r->classes[r->class_count].name = rvn_strndup(name, length);
	r->classes[r->class_count].style = *style;
	r->class_count++;
}

static void rvn_mm_add_apply(rvn_mm_reader_t* r, int node, const char* name, size_t length, const rvn_mm_style_t* style) {
	rvn_mm_apply_t* apply;
	if (node < 0 || r->apply_count >= 20000u) return;
	RVN_ARRAY_RESERVE(r->applies, r->apply_cap, r->apply_count + 1);
	apply = &r->applies[r->apply_count++];
	memset(apply, 0, sizeof(*apply));
	apply->node = node;
	apply->name = name ? rvn_strndup(name, length) : NULL;
	if (style) apply->style = *style;
}

static const rvn_mm_class_t* rvn_mm_class(const rvn_mm_reader_t* r, const char* name) {
	uint32_t i;
	for (i = 0; i < r->class_count; i++) {
		if (strcmp(r->classes[i].name, name) == 0) return &r->classes[i];
	}
	return NULL;
}

/* classDef default first, then the classes, then the styles, in the order written. */
static void rvn_mm_apply_styles(rvn_mm_reader_t* r) {
	rvn_mm_t* mm = r->mm;
	const rvn_mm_class_t* fallback = rvn_mm_class(r, "default");
	uint32_t i;
	if (fallback) {
		for (i = 0; i < mm->node_count; i++) rvn_mm_style_merge(&mm->nodes[i].style, &fallback->style);
	}
	for (i = 0; i < r->apply_count; i++) {
		const rvn_mm_apply_t* apply = &r->applies[i];
		if (apply->node < 0 || (uint32_t)apply->node >= mm->node_count) continue;
		if (apply->name) {
			const rvn_mm_class_t* c = rvn_mm_class(r, apply->name);
			if (c) rvn_mm_style_merge(&mm->nodes[apply->node].style, &c->style);
		}
	}
	for (i = 0; i < r->apply_count; i++) {
		const rvn_mm_apply_t* apply = &r->applies[i];
		if (apply->node < 0 || (uint32_t)apply->node >= mm->node_count || apply->name) continue;
		rvn_mm_style_merge(&mm->nodes[apply->node].style, &apply->style);
	}
}

/* A name at p: letters, digits, '_', other characters than ASCII, and '-' between two of them (my-node). */
static const char* rvn_mm_read_id(const char* p, const char* end) {
	const char* start = p;
	while (p < end) {
		if (rvn_mm_id_char(*p)) p++;
		else if (*p == '-' && p > start && p + 1 < end && rvn_mm_id_char(p[1]) && p[1] != '-') p++;
		else break;
	}
	return p;
}

/* "classDef a,b fill:#f00", "class n1,n2 name", "style n1 fill:#f00" (the keyword already read). */
static int rvn_mm_style_statement(rvn_mm_reader_t* r, const char* p, const char* end, int keyword) {
	const char* names = rvn_mm_skip(p, end);
	const char* names_end = names;
	const char* rest;
	while (names_end < end && !rvn_mm_space(*names_end)) names_end++;
	rest = rvn_mm_skip(names_end, end);
	if (names_end == names) {
		rvn_mm_error(r, "a name is missing after \"%s\"", keyword == 0 ? "classDef" : (keyword == 1 ? "class" : "style"));
		return R_FALSE;
	}
	if (end > rest && end[-1] == ';') end--;
	if (keyword == 0) {
		/* classDef */
		rvn_mm_style_t style;
		const char* name = names;
		memset(&style, 0, sizeof(style));
		rvn_mm_parse_style(rest, end, &style);
		while (name < names_end) {
			const char* stop = name;
			while (stop < names_end && *stop != ',') stop++;
			if (stop > name) rvn_mm_add_class(r, name, (size_t)(stop - name), &style);
			name = stop + 1;
		}
	} else if (keyword == 1) {
		/* class a,b name */
		const char* name = names;
		const char* class_end = rest;
		while (class_end < end && !rvn_mm_space(*class_end) && *class_end != ';') class_end++;
		if (class_end == rest) {
			rvn_mm_error(r, "a class name is missing");
			return R_FALSE;
		}
		while (name < names_end) {
			const char* stop = name;
			while (stop < names_end && *stop != ',') stop++;
			if (stop > name) rvn_mm_add_apply(r, rvn_mm_node(r, name, (size_t)(stop - name)), rest, (size_t)(class_end - rest), NULL);
			name = stop + 1;
		}
	} else {
		rvn_mm_style_t style;
		memset(&style, 0, sizeof(style));
		rvn_mm_parse_style(rest, end, &style);
		rvn_mm_add_apply(r, rvn_mm_node(r, names, (size_t)(names_end - names)), NULL, 0, &style);
	}
	return !r->mm->error;
}

/* accTitle, accDescr (on one line or in { }): what is the same for every kind. R_TRUE when the line was one. */
static int rvn_mm_common_statement(rvn_mm_reader_t* r, const char* p, const char* end) {
	rvn_mm_t* mm = r->mm;
	if (r->in_description) {
		const char* close = memchr(p, '}', (size_t)(end - p));
		const char* stop = close ? close : end;
		if (r->description.length) rvn_text_append(&r->description, "\n", 1);
		rvn_mm_trim(&p, &stop);
		rvn_text_append(&r->description, p, (size_t)(stop - p));
		if (close) {
			r->in_description = R_FALSE;
			rvn_free(mm->acc_description);
			mm->acc_description = rvn_strdup(r->description.data ? r->description.data : "");
		}
		return R_TRUE;
	}
	if (rvn_mm_starts(p, end, "accTitle")) {
		const char* colon = memchr(p, ':', (size_t)(end - p));
		if (colon) {
			rvn_free(mm->acc_title);
			mm->acc_title = rvn_mm_label(colon + 1, end);
			return R_TRUE;
		}
	}
	if (rvn_mm_starts(p, end, "accDescr")) {
		const char* q = rvn_mm_skip(p + 8, end);
		if (q < end && *q == ':') {
			rvn_free(mm->acc_description);
			mm->acc_description = rvn_mm_label(q + 1, end);
			return R_TRUE;
		}
		if (q < end && *q == '{') {
			r->in_description = R_TRUE;
			r->description.length = 0;
			return rvn_mm_common_statement(r, q + 1, end) || R_TRUE;
		}
	}
	return R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Flowcharts
 * ------------------------------------------------------------------------- */

typedef struct rvn_mm_link_t {
	uint8_t line, head_start, head_end, length;
	const char* text;
	const char* text_end;
} rvn_mm_link_t;

static char rvn_mm_direction(const char* p, const char* end) {
	if (end - p < 2) {
		if (end - p == 1 && (*p == '>' )) return 'L';
		if (end - p == 1 && (*p == '<' )) return 'R';
		if (end - p == 1 && (*p == 'v' )) return 'T';
		if (end - p == 1 && (*p == '^' )) return 'B';
		return 0;
	}
	if (end - p != 2) return 0;
	if (rvn_strnicmp(p, "TD", 2) == 0 || rvn_strnicmp(p, "TB", 2) == 0) return 'T';
	if (rvn_strnicmp(p, "BT", 2) == 0) return 'B';
	if (rvn_strnicmp(p, "LR", 2) == 0) return 'L';
	if (rvn_strnicmp(p, "RL", 2) == 0) return 'R';
	return 0;
}

/* The head at the end of a link: '>' an arrow, 'x' a cross, 'o' a circle (these two only before a space or a name of a node). */
static int rvn_mm_link_end(const char* p, const char* end, uint8_t* head) {
	if (p >= end) return R_FALSE;
	if (*p == '>') {
		*head = RVN_MM_HEAD_ARROW;
		return R_TRUE;
	}
	if ((*p == 'x' || *p == 'o') && (p + 1 >= end || rvn_mm_space(p[1]) || p[1] == '|' || !rvn_mm_id_char(p[1]))) {
		*head = *p == 'x' ? RVN_MM_HEAD_CROSS : RVN_MM_HEAD_CIRCLE;
		return R_TRUE;
	}
	return R_FALSE;
}

/*
 * A link at p: --> --- -.-> -.- ==> === ~~~, longer ones, the heads <, x, o,
 * a text inside (-- text -->, -. text .->, == text ==>) or after (|text|).
 * Returns where it ends, or NULL when there is none.
 */
static const char* rvn_mm_read_link(const char* p, const char* end, rvn_mm_link_t* link) {
	char mark;
	int n = 0;
	memset(link, 0, sizeof(*link));
	link->length = 1;
	if (p < end && *p == '<' && p + 1 < end && (p[1] == '-' || p[1] == '=')) {
		link->head_start = RVN_MM_HEAD_ARROW;
		p++;
	} else if (p + 2 < end && (*p == 'x' || *p == 'o') && ((p[1] == '-' && (p[2] == '-' || p[2] == '.')) || (p[1] == '=' && p[2] == '='))) {
		link->head_start = *p == 'x' ? RVN_MM_HEAD_CROSS : RVN_MM_HEAD_CIRCLE;
		p++;
	}
	if (p >= end) return NULL;
	mark = *p;
	if (mark == '~') {
		while (p + n < end && p[n] == '~') n++;
		if (n < 3) return NULL;
		link->line = RVN_MM_LINE_INVISIBLE;
		link->length = (uint8_t)rvn_clampi(n - 2, 1, 8);
		p += n;
	} else if (mark == '-' && p + 1 < end && p[1] == '.') {
		/* dotted: -.-> -..-> -.- or -. text .-> */
		int dots = 0;
		const char* q = p + 1;
		link->line = RVN_MM_LINE_DOTTED;
		while (q < end && *q == '.') {
			dots++;
			q++;
		}
		if (q < end && *q == '-') {
			q++;
			if (rvn_mm_link_end(q, end, &link->head_end)) q++;
			link->length = (uint8_t)rvn_clampi(dots, 1, 8);
			p = q;
		} else {
			/* a text, up to ".-" */
			const char* close = rvn_mm_find(q, end, ".-");
			const char* r;
			if (!close) return NULL;
			link->text = q;
			link->text_end = close;
			r = close;
			while (r < end && *r == '.') r++;
			if (r >= end || *r != '-') return NULL;
			r++;
			if (rvn_mm_link_end(r, end, &link->head_end)) r++;
			p = r;
		}
	} else if (mark == '-' || mark == '=') {
		const char* q;
		link->line = mark == '=' ? RVN_MM_LINE_THICK : RVN_MM_LINE_SOLID;
		while (p + n < end && p[n] == mark) n++;
		if (n < 2) return NULL;
		q = p + n;
		if (rvn_mm_link_end(q, end, &link->head_end)) {
			link->length = (uint8_t)rvn_clampi(n - 1, 1, 8);
			p = q + 1;
		} else if (n >= 3) {
			link->length = (uint8_t)rvn_clampi(n - 2, 1, 8);
			p = q;
		} else {
			/* -- text --> : the text runs to the next "--" (or "==") */
			char pair[3];
			const char* close;
			int m = 0;
			pair[0] = mark;
			pair[1] = mark;
			pair[2] = '\0';
			close = rvn_mm_find(q, end, pair);
			if (!close) return NULL;
			link->text = q;
			link->text_end = close;
			while (close + m < end && close[m] == mark) m++;
			q = close + m;
			if (rvn_mm_link_end(q, end, &link->head_end)) {
				link->length = (uint8_t)rvn_clampi(m - 1, 1, 8);
				q++;
			} else {
				if (m < 3) return NULL;
				link->length = (uint8_t)rvn_clampi(m - 2, 1, 8);
			}
			p = q;
		}
	} else {
		return NULL;
	}
	if (!link->head_end && link->head_start) {
		/* x--> is fine; <--- alone is read as a link toward the left node */
	}
	/* |text| */
	{
		const char* q = rvn_mm_skip(p, end);
		if (q < end && *q == '|') {
			const char* close = memchr(q + 1, '|', (size_t)(end - q - 1));
			if (!close) return NULL;
			link->text = q + 1;
			link->text_end = close;
			p = close + 1;
		}
	}
	return p;
}

/* A shape around a text at p: [text] (text) {text}... Returns where it ends, NULL when malformed, p when there is none. */
static const char* rvn_mm_read_shape(rvn_mm_reader_t* r, const char* p, const char* end, uint8_t* shape, const char** text, const char** text_end) {
	static const struct { const char* open; const char* close; const char* other; uint8_t shape, other_shape; } shapes[] = {
		{ "(((", ")))", NULL, RVN_MM_DOUBLE_CIRCLE, 0 },
		{ "((", "))", NULL, RVN_MM_CIRCLE, 0 },
		{ "([", "])", NULL, RVN_MM_STADIUM, 0 },
		{ "[[", "]]", NULL, RVN_MM_SUBROUTINE, 0 },
		{ "[(", ")]", NULL, RVN_MM_CYLINDER, 0 },
		{ "[/", "/]", "\\]", RVN_MM_PARALLELOGRAM, RVN_MM_TRAPEZOID },
		{ "[\\", "\\]", "/]", RVN_MM_PARALLELOGRAM_ALT, RVN_MM_TRAPEZOID_ALT },
		{ "{{", "}}", NULL, RVN_MM_HEXAGON, 0 },
		{ "(", ")", NULL, RVN_MM_ROUND, 0 },
		{ "[", "]", NULL, RVN_MM_RECT, 0 },
		{ "{", "}", NULL, RVN_MM_RHOMBUS, 0 },
		{ ">", "]", NULL, RVN_MM_ASYMMETRIC, 0 }
	};
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(shapes); i++) {
		const char* q;
		const char* close = NULL;
		const char* other = NULL;
		size_t open_length = strlen(shapes[i].open);
		if (!rvn_mm_starts(p, end, shapes[i].open)) continue;
		q = p + open_length;
		/* a quoted text may hold the closing marks */
		{
			const char* t = rvn_mm_skip(q, end);
			if (t < end && *t == '"') {
				const char* quote = memchr(t + 1, '"', (size_t)(end - t - 1));
				if (quote) {
					const char* after = rvn_mm_skip(quote + 1, end);
					if (rvn_mm_starts(after, end, shapes[i].close)) close = after;
					else if (shapes[i].other && rvn_mm_starts(after, end, shapes[i].other)) other = after;
				}
			}
		}
		if (!close && !other) {
			close = rvn_mm_find(q, end, shapes[i].close);
			if (shapes[i].other) {
				other = rvn_mm_find(q, end, shapes[i].other);
				if (close && other) {
					if (other < close) close = NULL;
					else other = NULL;
				}
			}
		}
		if (!close && !other) {
			rvn_mm_error(r, "\"%s\" is not closed by \"%s\"", shapes[i].open, shapes[i].close);
			return NULL;
		}
		*text = q;
		if (close) {
			*text_end = close;
			*shape = shapes[i].shape;
			return close + strlen(shapes[i].close);
		}
		*text_end = other;
		*shape = shapes[i].other_shape;
		return other + strlen(shapes[i].other);
	}
	return p;
}

/* A node at p: its name, its shape, its classes (:::name). Returns where it ends, NULL when there is none. */
static const char* rvn_mm_read_node(rvn_mm_reader_t* r, const char* p, const char* end, int* index) {
	const char* id_end = rvn_mm_read_id(p, end);
	const char* q;
	uint8_t shape = RVN_MM_RECT;
	const char* text = NULL;
	const char* text_end = NULL;
	if (id_end == p) return NULL;
	*index = rvn_mm_node(r, p, (size_t)(id_end - p));
	if (*index < 0) return NULL;
	q = rvn_mm_read_shape(r, id_end, end, &shape, &text, &text_end);
	if (!q) return NULL;
	if (q != id_end) {
		rvn_mm_node_t* node = &r->mm->nodes[*index];
		rvn_free(node->label);
		node->label = rvn_mm_label(text, text_end);
		node->shape = shape;
		node->declared = R_TRUE;
	}
	while (rvn_mm_starts(q, end, ":::")) {
		const char* name = q + 3;
		const char* name_end = name;
		while (name_end < end && (rvn_mm_id_char(*name_end) || *name_end == '-')) name_end++;
		if (name_end > name) rvn_mm_add_apply(r, *index, name, (size_t)(name_end - name), NULL);
		q = name_end;
	}
	return q;
}

/* Nodes joined by '&'. */
static const char* rvn_mm_read_group(rvn_mm_reader_t* r, const char* p, const char* end, int* nodes, int* count, int max) {
	*count = 0;
	for (;;) {
		int index;
		const char* q;
		p = rvn_mm_skip(p, end);
		q = rvn_mm_read_node(r, p, end, &index);
		if (!q) return NULL;
		if (*count < max) nodes[(*count)++] = index;
		p = rvn_mm_skip(q, end);
		if (p < end && *p == '&') {
			p++;
			continue;
		}
		return p;
	}
}

static void rvn_mm_flowchart_statement(rvn_mm_reader_t* r, const char* p, const char* end) {
	rvn_mm_t* mm = r->mm;
	int left[64], right[64], left_count = 0, right_count = 0;
	char excerpt[32];
	const char* p0;
	rvn_mm_trim(&p, &end);
	if (p >= end) return;
	if (rvn_mm_word(p, end, "end")) {
		if (r->depth <= 0) rvn_mm_error(r, "\"end\" without a subgraph to close");
		else r->depth--;
		return;
	}
	if (rvn_mm_word(p, end, "subgraph")) {
		const char* q = rvn_mm_skip(p + 8, end);
		const char* id_end = rvn_mm_read_id(q, end);
		char* id;
		char* title;
		const char* t = rvn_mm_skip(id_end, end);
		if (q >= end) {
			char name[32];
			snprintf(name, sizeof(name), "subgraph%d", r->counter++);
			id = rvn_strdup(name);
			title = rvn_strdup("");
		} else if (id_end > q && t < end && *t == '[') {
			/* subgraph id [Title] */
			const char* close = rvn_mm_find(t, end, "]");
			id = rvn_strndup(q, (size_t)(id_end - q));
			title = rvn_mm_label(t + 1, close ? close : end);
		} else if (id_end > q && t >= end) {
			id = rvn_strndup(q, (size_t)(id_end - q));
			title = rvn_strndup(q, (size_t)(id_end - q));
		} else {
			/* subgraph "A title" or subgraph A title */
			title = rvn_mm_label(q, end);
			id = rvn_strdup(title);
		}
		rvn_mm_add_cluster(r, id, title);
		return;
	}
	if (rvn_mm_word(p, end, "direction")) {
		const char* q = rvn_mm_skip(p + 9, end);
		/* (a subgraph keeps the direction of the whole graph) */
		if (!rvn_mm_direction(q, end)) rvn_mm_error(r, "unknown direction (TB, TD, BT, LR or RL)");
		return;
	}
	if (rvn_mm_word(p, end, "classDef")) {
		rvn_mm_style_statement(r, p + 8, end, 0);
		return;
	}
	if (rvn_mm_word(p, end, "class")) {
		rvn_mm_style_statement(r, p + 5, end, 1);
		return;
	}
	if (rvn_mm_word(p, end, "style")) {
		rvn_mm_style_statement(r, p + 5, end, 2);
		return;
	}
	/* linkStyle, click and callbacks: understood, not drawn */
	if (rvn_mm_word(p, end, "linkStyle") || rvn_mm_word(p, end, "click") || rvn_mm_word(p, end, "callback")) return;
	if (rvn_mm_common_statement(r, p, end)) return;

	/* nodes and links: a & b --> c --> d & e */
	p0 = p;
	p = rvn_mm_read_group(r, p, end, left, &left_count, 64);
	if (!p) {
		if (!mm->error) {
			rvn_mm_excerpt(p0, end, excerpt, sizeof(excerpt));
			rvn_mm_error(r, "a node was expected at \"%s\"", excerpt);
		}
		return;
	}
	while (p < end && !mm->error) {
		rvn_mm_link_t link;
		const char* q = rvn_mm_read_link(p, end, &link);
		int i, j;
		if (!q) {
			rvn_mm_excerpt(p, end, excerpt, sizeof(excerpt));
			rvn_mm_error(r, "a link was expected at \"%s\"", excerpt);
			return;
		}
		q = rvn_mm_read_group(r, q, end, right, &right_count, 64);
		if (!q) {
			if (!mm->error) rvn_mm_error(r, "a node is missing after the link");
			return;
		}
		for (i = 0; i < left_count && !mm->error; i++) {
			for (j = 0; j < right_count && !mm->error; j++) {
				int e = rvn_mm_add_edge(r, left[i], right[j]);
				if (e < 0) break;
				mm->edges[e].line = link.line;
				mm->edges[e].head_start = link.head_start;
				mm->edges[e].head_end = link.head_end;
				mm->edges[e].length = link.length;
				if (link.text && link.text_end > link.text) {
					char* label = rvn_mm_label(link.text, link.text_end);
					if (label[0]) mm->edges[e].label = label;
					else rvn_free(label);
				}
			}
		}
		memcpy(left, right, sizeof(int) * (size_t)right_count);
		left_count = right_count;
		p = rvn_mm_skip(q, end);
	}
}

/* ---------------------------------------------------------------------------
 *  State diagrams
 * ------------------------------------------------------------------------- */

/* The start or the end point of the composite state open, made the first time. */
static int rvn_mm_state_point(rvn_mm_reader_t* r, int start) {
	char name[48];
	int cluster = r->depth > 0 ? r->stack[r->depth - 1] : -1;
	int index;
	snprintf(name, sizeof(name), "[*]%s%d", start ? "start" : "end", cluster);
	index = rvn_mm_node(r, name, strlen(name));
	if (index >= 0) {
		rvn_mm_node_t* node = &r->mm->nodes[index];
		node->shape = (uint8_t)(start ? RVN_MM_START : RVN_MM_END);
		node->declared = R_TRUE;
		node->label[0] = '\0';
		node->cluster = cluster;
	}
	return index;
}

/* A state at p ("[*]" or a name, with :::classes). */
static const char* rvn_mm_read_state(rvn_mm_reader_t* r, const char* p, const char* end, int start, int* index) {
	const char* q;
	p = rvn_mm_skip(p, end);
	if (rvn_mm_starts(p, end, "[*]")) {
		*index = rvn_mm_state_point(r, start);
		return *index < 0 ? NULL : p + 3;
	}
	q = rvn_mm_read_id(p, end);
	while (q < end && (*q == '.' || *q == '-') && q + 1 < end && rvn_mm_id_char(q[1])) q = rvn_mm_read_id(q + 1, end);
	if (q == p) return NULL;
	*index = rvn_mm_node(r, p, (size_t)(q - p));
	if (*index < 0) return NULL;
	r->mm->nodes[*index].shape = r->mm->nodes[*index].shape == RVN_MM_RECT ? (uint8_t)RVN_MM_ROUND : r->mm->nodes[*index].shape;
	while (rvn_mm_starts(q, end, ":::")) {
		const char* name = q + 3;
		const char* name_end = name;
		while (name_end < end && (rvn_mm_id_char(*name_end) || *name_end == '-')) name_end++;
		if (name_end > name) rvn_mm_add_apply(r, *index, name, (size_t)(name_end - name), NULL);
		q = name_end;
	}
	return q;
}

static void rvn_mm_add_note(rvn_mm_reader_t* r, int state, int left, const char* text, size_t length) {
	char name[32];
	int note;
	int e;
	snprintf(name, sizeof(name), "note%d", r->counter++);
	note = rvn_mm_node(r, name, strlen(name));
	if (note < 0 || state < 0) return;
	rvn_free(r->mm->nodes[note].label);
	r->mm->nodes[note].label = rvn_mm_label(text, text + length);
	r->mm->nodes[note].shape = RVN_MM_NOTE;
	r->mm->nodes[note].declared = R_TRUE;
	r->mm->nodes[note].cluster = r->mm->nodes[state].cluster;
	/* the link that holds a note beside its state: no length, no head (core/mermaid_layout.c) */
	e = rvn_mm_add_edge(r, left ? note : state, left ? state : note);
	if (e >= 0) {
		r->mm->edges[e].line = RVN_MM_LINE_DOTTED;
		r->mm->edges[e].head_end = RVN_MM_HEAD_NONE;
		r->mm->edges[e].length = 0;
	}
}

static void rvn_mm_state_statement(rvn_mm_reader_t* r, const char* p, const char* end) {
	rvn_mm_t* mm = r->mm;
	const char* arrow;
	char excerpt[32];
	rvn_mm_trim(&p, &end);
	if (r->note_node >= 0) {
		/* the lines of a note, up to "end note" */
		if (rvn_mm_word(p, end, "end") && rvn_mm_word(rvn_mm_skip(p + 3, end), end, "note")) {
			int state = r->note_node & 0xFFFFF;
			rvn_mm_add_note(r, state, (r->note_node >> 24) & 1, r->note.data ? r->note.data : "", r->note.length);
			r->note_node = -1;
			r->note.length = 0;
		} else {
			if (r->note.length) rvn_text_append(&r->note, "\n", 1);
			rvn_text_append(&r->note, p, (size_t)(end - p));
		}
		return;
	}
	if (p >= end) return;
	if (*p == '}') {
		if (r->depth <= 0) rvn_mm_error(r, "\"}\" without a composite state to close");
		else r->depth--;
		return;
	}
	if (rvn_mm_starts(p, end, "--") && rvn_mm_skip(p + 2, end) == end) return;
	if (rvn_mm_word(p, end, "direction")) {
		char direction = rvn_mm_direction(rvn_mm_skip(p + 9, end), end);
		if (!direction) rvn_mm_error(r, "unknown direction (TB, TD, BT, LR or RL)");
		else if (r->depth == 0) mm->direction = direction;
		return;
	}
	if (rvn_mm_word(p, end, "hide") || rvn_mm_word(p, end, "scale") || rvn_mm_word(p, end, "click")) return;
	if (rvn_mm_word(p, end, "classDef")) {
		rvn_mm_style_statement(r, p + 8, end, 0);
		return;
	}
	if (rvn_mm_word(p, end, "class")) {
		rvn_mm_style_statement(r, p + 5, end, 1);
		return;
	}
	if (rvn_mm_word(p, end, "style")) {
		rvn_mm_style_statement(r, p + 5, end, 2);
		return;
	}
	if (rvn_mm_common_statement(r, p, end)) return;
	if (rvn_mm_word(p, end, "note")) {
		/* note right of S : text, or over several lines up to "end note" */
		const char* q = rvn_mm_skip(p + 4, end);
		int left = 0, state;
		const char* colon;
		if (rvn_mm_word(q, end, "left")) left = 1;
		else if (!rvn_mm_word(q, end, "right")) {
			rvn_mm_error(r, "a note is \"left of\" or \"right of\" a state");
			return;
		}
		q = rvn_mm_skip(q + (left ? 4 : 5), end);
		if (!rvn_mm_word(q, end, "of")) {
			rvn_mm_error(r, "\"of\" is missing in the note");
			return;
		}
		q = rvn_mm_read_state(r, q + 2, end, R_FALSE, &state);
		if (!q) {
			rvn_mm_error(r, "the state of the note is missing");
			return;
		}
		colon = rvn_mm_skip(q, end);
		if (colon < end && *colon == ':') {
			rvn_mm_add_note(r, state, left, colon + 1, (size_t)(end - colon - 1));
		} else {
			r->note_node = state | (left << 24);
			r->note.length = 0;
		}
		return;
	}
	if (rvn_mm_word(p, end, "state")) {
		/* state "Label" as id, state id <<choice>>, state id {, state id : label */
		const char* q = rvn_mm_skip(p + 5, end);
		const char* id;
		const char* id_end;
		char* label = NULL;
		int index;
		if (q < end && *q == '"') {
			const char* close = memchr(q + 1, '"', (size_t)(end - q - 1));
			if (!close) {
				rvn_mm_error(r, "the quote is not closed");
				return;
			}
			label = rvn_mm_label(q + 1, close);
			q = rvn_mm_skip(close + 1, end);
			if (!rvn_mm_word(q, end, "as")) {
				rvn_free(label);
				rvn_mm_error(r, "\"as\" is missing after the name of the state");
				return;
			}
			q = rvn_mm_skip(q + 2, end);
		}
		id = q;
		id_end = rvn_mm_read_id(q, end);
		if (id_end == id) {
			rvn_free(label);
			rvn_mm_error(r, "the name of the state is missing");
			return;
		}
		index = rvn_mm_node(r, id, (size_t)(id_end - id));
		if (index < 0) {
			rvn_free(label);
			return;
		}
		if (mm->nodes[index].shape == RVN_MM_RECT) mm->nodes[index].shape = RVN_MM_ROUND;
		q = rvn_mm_skip(id_end, end);
		if (label) {
			rvn_free(mm->nodes[index].label);
			mm->nodes[index].label = label;
		}
		if (q < end && *q == ':') {
			rvn_free(mm->nodes[index].label);
			mm->nodes[index].label = rvn_mm_label(q + 1, end);
			return;
		}
		if (rvn_mm_starts(q, end, "<<")) {
			const char* close = rvn_mm_find(q, end, ">>");
			if (!close) {
				rvn_mm_error(r, "\"<<\" is not closed by \">>\"");
				return;
			}
			if (rvn_strnicmp(q + 2, "choice", 6) == 0) mm->nodes[index].shape = RVN_MM_RHOMBUS;
			else if (rvn_strnicmp(q + 2, "fork", 4) == 0 || rvn_strnicmp(q + 2, "join", 4) == 0) mm->nodes[index].shape = RVN_MM_BAR;
			mm->nodes[index].declared = R_TRUE;
			if (mm->nodes[index].shape != RVN_MM_ROUND) mm->nodes[index].label[0] = '\0';
			return;
		}
		if (q < end && *q == '{') {
			/* a composite state: a cluster named like the state; the state becomes the cluster */
			char* title = rvn_strdup(mm->nodes[index].label);
			rvn_mm_add_cluster(r, rvn_strndup(id, (size_t)(id_end - id)), title);
			return;
		}
		if (q < end) {
			rvn_mm_excerpt(q, end, excerpt, sizeof(excerpt));
			rvn_mm_error(r, "unexpected \"%s\" after the state", excerpt);
		}
		return;
	}
	arrow = rvn_mm_find(p, end, "-->");
	if (arrow) {
		int from, to, e;
		const char* q = rvn_mm_read_state(r, p, arrow, R_TRUE, &from);
		const char* colon;
		if (!q || rvn_mm_skip(q, arrow) != arrow) {
			if (!mm->error) rvn_mm_error(r, "the state before \"-->\" is missing");
			return;
		}
		q = rvn_mm_read_state(r, arrow + 3, end, R_FALSE, &to);
		if (!q) {
			if (!mm->error) rvn_mm_error(r, "the state after \"-->\" is missing");
			return;
		}
		colon = rvn_mm_skip(q, end);
		if (colon < end && *colon != ':') {
			rvn_mm_excerpt(colon, end, excerpt, sizeof(excerpt));
			rvn_mm_error(r, "unexpected \"%s\" after the transition", excerpt);
			return;
		}
		e = rvn_mm_add_edge(r, from, to);
		if (e >= 0 && colon < end) {
			char* label = rvn_mm_label(colon + 1, end);
			if (label[0]) mm->edges[e].label = label;
			else rvn_free(label);
		}
		return;
	}
	{
		/* a state alone, or "id : label" */
		int index;
		const char* q = rvn_mm_read_state(r, p, end, R_FALSE, &index);
		if (!q) {
			if (!mm->error) {
				rvn_mm_excerpt(p, end, excerpt, sizeof(excerpt));
				rvn_mm_error(r, "unknown statement \"%s\"", excerpt);
			}
			return;
		}
		q = rvn_mm_skip(q, end);
		if (q < end && *q == ':') {
			rvn_free(mm->nodes[index].label);
			mm->nodes[index].label = rvn_mm_label(q + 1, end);
		} else if (q < end) {
			rvn_mm_excerpt(q, end, excerpt, sizeof(excerpt));
			rvn_mm_error(r, "unexpected \"%s\"", excerpt);
		}
	}
}

/* ---------------------------------------------------------------------------
 *  Sequence diagrams
 * ------------------------------------------------------------------------- */

/* A participant named at p .. end (trimmed), made the first time. */
static int rvn_mm_participant(rvn_mm_reader_t* r, const char* p, const char* end) {
	rvn_mm_trim(&p, &end);
	if (p >= end) return -1;
	if (end - p > 200) end = p + 200;
	return rvn_mm_node(r, p, (size_t)(end - p));
}

static void rvn_mm_sequence_statement(rvn_mm_reader_t* r, const char* p, const char* end) {
	static const struct { const char* arrow; uint8_t line, head; } arrows[] = {
		{ "<<-->>", RVN_MM_LINE_DOTTED, 2 }, { "<<->>", RVN_MM_LINE_SOLID, 2 },
		{ "-->>", RVN_MM_LINE_DOTTED, RVN_MM_HEAD_ARROW }, { "->>", RVN_MM_LINE_SOLID, RVN_MM_HEAD_ARROW },
		{ "--x", RVN_MM_LINE_DOTTED, RVN_MM_HEAD_CROSS }, { "-x", RVN_MM_LINE_SOLID, RVN_MM_HEAD_CROSS },
		{ "--)", RVN_MM_LINE_DOTTED, RVN_MM_HEAD_OPEN }, { "-)", RVN_MM_LINE_SOLID, RVN_MM_HEAD_OPEN },
		{ "-->", RVN_MM_LINE_DOTTED, RVN_MM_HEAD_NONE }, { "->", RVN_MM_LINE_SOLID, RVN_MM_HEAD_NONE }
	};
	rvn_mm_t* mm = r->mm;
	rvn_mm_event_t* event;
	char excerpt[32];
	const char* q;
	rvn_mm_trim(&p, &end);
	if (p >= end) return;
	if (rvn_mm_common_statement(r, p, end)) return;
	if (rvn_mm_word(p, end, "create")) p = rvn_mm_skip(p + 6, end);
	if (rvn_mm_word(p, end, "participant") || rvn_mm_word(p, end, "actor")) {
		int actor = rvn_mm_word(p, end, "actor");
		const char* name = rvn_mm_skip(p + (actor ? 5 : 11), end);
		const char* as = name;
		const char* name_end = end;
		int index;
		/* "participant A as Alice" */
		while (as < end) {
			if (rvn_mm_space(*as) && rvn_mm_word(rvn_mm_skip(as, end), end, "as")) break;
			as++;
		}
		if (as < end) name_end = as;
		index = rvn_mm_participant(r, name, name_end);
		if (index < 0) {
			if (!mm->error) rvn_mm_error(r, "the name of the participant is missing");
			return;
		}
		mm->nodes[index].shape = (uint8_t)(actor ? RVN_MM_ACTOR : RVN_MM_RECT);
		mm->nodes[index].declared = R_TRUE;
		if (as < end) {
			rvn_free(mm->nodes[index].label);
			mm->nodes[index].label = rvn_mm_label(rvn_mm_skip(as, end) + 2, end);
		}
		return;
	}
	if (rvn_mm_word(p, end, "destroy") || rvn_mm_word(p, end, "links") || rvn_mm_word(p, end, "link") ||
		rvn_mm_word(p, end, "properties") || rvn_mm_word(p, end, "details")) return;
	if (rvn_mm_word(p, end, "title") || (rvn_mm_starts(p, end, "title") && end - p > 5 && p[5] == ':')) {
		rvn_free(mm->title);
		mm->title = rvn_mm_label(p[5] == ':' ? p + 6 : p + 5, end);
		return;
	}
	if (rvn_mm_word(p, end, "autonumber")) {
		const char* a = rvn_mm_skip(p + 10, end);
		event = rvn_mm_add_event(r, RVN_MM_EV_AUTONUMBER);
		if (!event) return;
		event->from = 1;
		event->to = 1;
		if (rvn_mm_word(a, end, "off")) event->from = 0;
		else if (a < end) {
			char* stop;
			long first = strtol(a, &stop, 10);
			if (stop > a) {
				event->from = (int)rvn_clampi((int)rvn_minf((float)first, 1e6f), 0, 1000000);
				a = rvn_mm_skip(stop, end);
				if (a < end) {
					long step = strtol(a, &stop, 10);
					if (stop > a) event->to = rvn_clampi((int)rvn_minf((float)step, 1e6f), 1, 1000000);
				}
			}
		}
		return;
	}
	if (rvn_mm_word(p, end, "activate") || rvn_mm_word(p, end, "deactivate")) {
		int activate = rvn_mm_word(p, end, "activate");
		int index = rvn_mm_participant(r, p + (activate ? 8 : 10), end);
		if (index < 0) {
			if (!mm->error) rvn_mm_error(r, "the participant to %s is missing", activate ? "activate" : "deactivate");
			return;
		}
		event = rvn_mm_add_event(r, activate ? RVN_MM_EV_ACTIVATE : RVN_MM_EV_DEACTIVATE);
		if (event) event->from = index;
		return;
	}
	if (rvn_mm_word(p, end, "box")) {
		/* box Aqua Group: a cluster that holds the participants declared inside */
		const char* a = rvn_mm_skip(p + 3, end);
		const char* word_end = a;
		rvn_color_t color = 0;
		int cluster;
		while (word_end < end && !rvn_mm_space(*word_end)) word_end++;
		if (rvn_strnicmp(a, "rgb", 3) == 0) {
			const char* close = memchr(a, ')', (size_t)(end - a));
			if (close) word_end = close + 1;
		}
		if (word_end > a && rvn_mm_parse_color(a, (int)(word_end - a), &color)) a = rvn_mm_skip(word_end, end);
		cluster = rvn_mm_add_cluster(r, rvn_strdup("box"), rvn_mm_label(a, end));
		if (cluster >= 0) {
			mm->clusters[cluster].style.fill = color;
			r->stack_kind[r->depth - 1] = 1;
		}
		return;
	}
	{
		static const char* const blocks[] = { "loop", "alt", "opt", "par", "critical", "break", "rect" };
		static const char* const sections[] = { "else", "and", "option" };
		size_t i;
		for (i = 0; i < RVN_COUNT_OF(blocks); i++) {
			if (rvn_mm_word(p, end, blocks[i])) {
				const char* text = rvn_mm_skip(p + strlen(blocks[i]), end);
				if (r->depth >= (int)RVN_COUNT_OF(r->stack)) {
					rvn_mm_error(r, "too many blocks inside each other");
					return;
				}
				event = rvn_mm_add_event(r, RVN_MM_EV_BLOCK);
				if (!event) return;
				event->extra = blocks[i];
				if (i == 6) {
					if (!rvn_mm_parse_color(text, (int)(end - text), &event->color)) event->color = 0;
				} else {
					event->text = rvn_mm_label(text, end);
				}
				r->stack_kind[r->depth] = 0;
				r->stack[r->depth++] = -1;
				return;
			}
		}
		for (i = 0; i < RVN_COUNT_OF(sections); i++) {
			if (rvn_mm_word(p, end, sections[i])) {
				if (r->depth <= 0 || r->stack_kind[r->depth - 1] != 0) {
					rvn_mm_error(r, "\"%s\" outside of a block", sections[i]);
					return;
				}
				event = rvn_mm_add_event(r, RVN_MM_EV_SECTION);
				if (!event) return;
				event->extra = sections[i];
				event->text = rvn_mm_label(p + strlen(sections[i]), end);
				return;
			}
		}
	}
	if (rvn_mm_word(p, end, "end")) {
		if (r->depth <= 0) {
			rvn_mm_error(r, "\"end\" without a block to close");
			return;
		}
		r->depth--;
		if (r->stack_kind[r->depth] == 0) rvn_mm_add_event(r, RVN_MM_EV_END);
		return;
	}
	if (rvn_mm_word(p, end, "note") || (end - p > 4 && rvn_strnicmp(p, "note", 4) == 0 && rvn_mm_space(p[4]))) {
		const char* a = rvn_mm_skip(p + 4, end);
		const char* colon = memchr(a, ':', (size_t)(end - a));
		int where;
		const char* names;
		const char* comma;
		if (rvn_mm_word(a, end, "left") || rvn_mm_word(a, end, "right")) {
			where = rvn_mm_word(a, end, "left") ? RVN_MM_NOTE_LEFT : RVN_MM_NOTE_RIGHT;
			a = rvn_mm_skip(a + (where == RVN_MM_NOTE_LEFT ? 4 : 5), end);
			if (!rvn_mm_word(a, end, "of")) {
				rvn_mm_error(r, "\"of\" is missing in the note");
				return;
			}
			names = a + 2;
		} else if (rvn_mm_word(a, end, "over")) {
			where = RVN_MM_NOTE_OVER;
			names = a + 4;
		} else {
			rvn_mm_error(r, "a note is \"left of\", \"right of\" or \"over\" a participant");
			return;
		}
		if (!colon) {
			rvn_mm_error(r, "\":\" and the text of the note are missing");
			return;
		}
		event = rvn_mm_add_event(r, RVN_MM_EV_NOTE);
		if (!event) return;
		event->line = (uint8_t)where;
		comma = memchr(names, ',', (size_t)(colon - names));
		event->from = rvn_mm_participant(r, names, comma ? comma : colon);
		event->to = comma ? rvn_mm_participant(r, comma + 1, colon) : event->from;
		if (event->from < 0 || event->to < 0) {
			if (!mm->error) rvn_mm_error(r, "the participant of the note is missing");
			return;
		}
		event->text = rvn_mm_label(colon + 1, end);
		return;
	}
	/* a message: A->>B: text */
	for (q = p; q < end; q++) {
		size_t i;
		for (i = 0; i < RVN_COUNT_OF(arrows); i++) {
			const char* arrow = arrows[i].arrow;
			size_t n = strlen(arrow);
			const char* to_start;
			const char* colon;
			int from, to;
			int8_t activation = 0;
			if (!rvn_mm_starts(q, end, arrow)) continue;
			from = rvn_mm_participant(r, p, q);
			if (from < 0) {
				if (!mm->error) rvn_mm_error(r, "the participant before \"%s\" is missing", arrow);
				return;
			}
			to_start = rvn_mm_skip(q + n, end);
			if (to_start < end && (*to_start == '+' || *to_start == '-')) {
				activation = (int8_t)(*to_start == '+' ? 1 : -1);
				to_start++;
			}
			colon = memchr(to_start, ':', (size_t)(end - to_start));
			to = rvn_mm_participant(r, to_start, colon ? colon : end);
			if (to < 0) {
				if (!mm->error) rvn_mm_error(r, "the participant after \"%s\" is missing", arrow);
				return;
			}
			event = rvn_mm_add_event(r, RVN_MM_EV_MESSAGE);
			if (!event) return;
			event->from = from;
			event->to = to;
			event->line = arrows[i].line;
			event->head = arrows[i].head;
			event->activation = activation;
			event->text = colon ? rvn_mm_label(colon + 1, end) : rvn_strdup("");
			return;
		}
	}
	rvn_mm_excerpt(p, end, excerpt, sizeof(excerpt));
	rvn_mm_error(r, "unknown statement \"%s\"", excerpt);
}

/* ---------------------------------------------------------------------------
 *  Pies
 * ------------------------------------------------------------------------- */

static void rvn_mm_pie_statement(rvn_mm_reader_t* r, const char* p, const char* end) {
	rvn_mm_t* mm = r->mm;
	rvn_mm_trim(&p, &end);
	if (p >= end) return;
	if (rvn_mm_common_statement(r, p, end)) return;
	if (rvn_mm_word(p, end, "showData")) {
		mm->show_data = R_TRUE;
		p = rvn_mm_skip(p + 8, end);
		if (p >= end) return;
	}
	if (rvn_mm_word(p, end, "title")) {
		rvn_free(mm->title);
		mm->title = rvn_mm_label(p + 5, end);
		return;
	}
	if (*p == '"' || *p == '\'') {
		const char* close = memchr(p + 1, *p, (size_t)(end - p - 1));
		const char* colon;
		char number[64];
		char* stop;
		double value;
		size_t n;
		rvn_mm_slice_t* slice;
		if (!close) {
			rvn_mm_error(r, "the quote is not closed");
			return;
		}
		colon = rvn_mm_skip(close + 1, end);
		if (colon >= end || *colon != ':') {
			rvn_mm_error(r, "\":\" and a value are missing after the label");
			return;
		}
		colon = rvn_mm_skip(colon + 1, end);
		n = rvn_minu((uint32_t)(end - colon), (uint32_t)sizeof(number) - 1u);
		memcpy(number, colon, n);
		number[n] = '\0';
		value = strtod(number, &stop);
		while (*stop && rvn_mm_space(*stop)) stop++;
		if (stop == number || *stop || !(value >= 0.0) || value > 1e300) {
			rvn_mm_error(r, "the value of the slice is not a positive number");
			return;
		}
		if (mm->slice_count >= 1000u) {
			rvn_mm_error(r, "too many slices");
			return;
		}
		RVN_ARRAY_RESERVE(mm->slices, mm->slice_cap, mm->slice_count + 1);
		slice = &mm->slices[mm->slice_count++];
		slice->label = rvn_mm_label(p + 1, close);
		slice->value = value;
		return;
	}
	{
		char excerpt[32];
		rvn_mm_excerpt(p, end, excerpt, sizeof(excerpt));
		rvn_mm_error(r, "unknown statement \"%s\"", excerpt);
	}
}

/* ---------------------------------------------------------------------------
 *  The text
 * ------------------------------------------------------------------------- */

/* %%{init: {"theme": "forest"}}%%: the theme, if there is one. */
static void rvn_mm_directive(rvn_mm_t* mm, const char* p, const char* end) {
	const char* key = rvn_mm_find(p, end, "theme");
	const char* value;
	const char* value_end;
	if (!key) return;
	value = key + 5;
	while (value < end && (*value == '"' || *value == '\'' || rvn_mm_space(*value) || *value == ':')) value++;
	value_end = value;
	while (value_end < end && rvn_mm_alnum(*value_end)) value_end++;
	if (value_end > value) {
		rvn_free(mm->theme);
		mm->theme = rvn_strndup(value, (size_t)(value_end - value));
	}
}

/*
 * Moves the links that name a subgraph (or a composite state) to a node
 * inside it, and takes away the node that the name made: a link cannot end
 * on a box here, it ends on what the box holds first.
 */
static void rvn_mm_resolve_cluster_links(rvn_mm_t* mm) {
	/* where a link that names a cluster goes: from its end point (or its last node), to its start point (or its first node) */
	int* to_node;
	int* from_node;
	uint32_t i, j, kept = 0;
	int any = R_FALSE;
	if (!mm->cluster_count || !mm->node_count) return;
	to_node = (int*)rvn_alloc(sizeof(int) * mm->node_count);
	from_node = (int*)rvn_alloc(sizeof(int) * mm->node_count);
	for (i = 0; i < mm->node_count; i++) {
		to_node[i] = from_node[i] = (int)i;
		if (mm->nodes[i].declared) continue;
		for (j = 0; j < mm->cluster_count; j++) {
			int first = -1, last = -1, start = -1, end = -1;
			uint32_t k;
			if (strcmp(mm->clusters[j].id, mm->nodes[i].id) != 0) continue;
			for (k = 0; k < mm->node_count; k++) {
				if (k == i || !rvn_mm_in_cluster(mm, mm->nodes[k].cluster, (int)j) || mm->nodes[k].shape == RVN_MM_NOTE) continue;
				if (strcmp(mm->nodes[k].id, mm->nodes[i].id) == 0) continue;
				if (first < 0) first = (int)k;
				last = (int)k;
				if (mm->nodes[k].cluster == (int)j && mm->nodes[k].shape == RVN_MM_START && start < 0) start = (int)k;
				if (mm->nodes[k].cluster == (int)j && mm->nodes[k].shape == RVN_MM_END && end < 0) end = (int)k;
			}
			if (first >= 0) {
				to_node[i] = start >= 0 ? start : first;
				from_node[i] = end >= 0 ? end : last;
			}
			break;
		}
		if (to_node[i] != (int)i) any = R_TRUE;
	}
	if (any) {
		/* the nodes kept, packed */
		int* index = (int*)rvn_alloc(sizeof(int) * mm->node_count);
		for (i = 0; i < mm->node_count; i++) {
			if (to_node[i] == (int)i) {
				index[i] = (int)kept;
				if (kept != i) mm->nodes[kept] = mm->nodes[i];
				kept++;
			} else {
				index[i] = -1;
				rvn_free(mm->nodes[i].id);
				rvn_free(mm->nodes[i].label);
			}
		}
		for (i = 0; i < mm->edge_count; i++) {
			mm->edges[i].from = index[from_node[mm->edges[i].from]];
			mm->edges[i].to = index[to_node[mm->edges[i].to]];
		}
		mm->node_count = kept;
		rvn_free(index);
	}
	rvn_free(to_node);
	rvn_free(from_node);
}

/* The kind of diagram from its first line; R_MERMAID_NONE with an error when it is not one that is drawn. */
static RMermaidKind rvn_mm_header(rvn_mm_reader_t* r, const char* p, const char* end, const char** rest) {
	static const char* const others[] = {
		"classDiagram", "classDiagram-v2", "erDiagram", "gantt", "journey", "gitGraph", "mindmap", "timeline", "quadrantChart",
		"requirementDiagram", "C4Context", "C4Container", "C4Component", "C4Dynamic", "C4Deployment", "sankey-beta", "xychart-beta",
		"block-beta", "packet-beta", "architecture-beta", "kanban", "radar-beta"
	};
	const char* word_end = p;
	size_t i, n;
	while (word_end < end && !rvn_mm_space(*word_end) && *word_end != ';') word_end++;
	n = (size_t)(word_end - p);
	*rest = word_end;
	if ((n == 9 && memcmp(p, "flowchart", 9) == 0) || (n == 5 && memcmp(p, "graph", 5) == 0)) return R_MERMAID_FLOWCHART;
	if (n == 15 && memcmp(p, "sequenceDiagram", 15) == 0) return R_MERMAID_SEQUENCE;
	if ((n == 12 && memcmp(p, "stateDiagram", 12) == 0) || (n == 15 && memcmp(p, "stateDiagram-v2", 15) == 0)) return R_MERMAID_STATE;
	if (n == 3 && memcmp(p, "pie", 3) == 0) return R_MERMAID_PIE;
	for (i = 0; i < RVN_COUNT_OF(others); i++) {
		if (strlen(others[i]) == n && memcmp(p, others[i], n) == 0) {
			rvn_mm_error(r, "%s diagrams are not supported (flowchart, sequenceDiagram, stateDiagram and pie are)", others[i]);
			return R_MERMAID_NONE;
		}
	}
	{
		char excerpt[32];
		rvn_mm_excerpt(p, end, excerpt, sizeof(excerpt));
		rvn_mm_error(r, "unknown kind of diagram \"%s\"", excerpt);
	}
	return R_MERMAID_NONE;
}

/* Calls the reader of the kind for each statement of a line (a flowchart cuts its lines at the ';' outside of texts). */
static void rvn_mm_statements(rvn_mm_reader_t* r, RMermaidKind kind, const char* p, const char* end) {
	if (kind == R_MERMAID_FLOWCHART) {
		const char* start = p;
		const char* q;
		char quote = 0;
		int depth = 0;
		for (q = p; q < end && !r->mm->error; q++) {
			char c = *q;
			if (quote) {
				if (c == quote) quote = 0;
				continue;
			}
			if (c == '"' || c == '|') quote = c;
			else if (c == '[' || c == '(' || c == '{') depth++;
			else if ((c == ']' || c == ')' || c == '}') && depth > 0) depth--;
			else if (c == ';' && depth == 0) {
				rvn_mm_flowchart_statement(r, start, q);
				start = q + 1;
			}
		}
		if (!r->mm->error) rvn_mm_flowchart_statement(r, start, end);
	} else if (kind == R_MERMAID_SEQUENCE) {
		if (end > p && end[-1] == ';') end--;
		rvn_mm_sequence_statement(r, p, end);
	} else if (kind == R_MERMAID_STATE) {
		if (end > p && end[-1] == ';') end--;
		rvn_mm_state_statement(r, p, end);
	} else if (kind == R_MERMAID_PIE) {
		if (end > p && end[-1] == ';') end--;
		rvn_mm_pie_statement(r, p, end);
	}
}

rvn_mm_t* rvn_mm_parse(const char* source, int length) {
	rvn_mm_t* mm = (rvn_mm_t*)rvn_calloc(sizeof(rvn_mm_t));
	rvn_mm_reader_t reader;
	const char* p;
	const char* end;
	RMermaidKind kind = R_MERMAID_NONE;
	int front_matter = 0, first = 1;
	uint32_t i;

	if (!mm) return NULL;
	mm->direction = 'T';
	mm->kind = R_MERMAID_NONE;
	if (!source) source = "";
	if (length < 0) length = (int)strlen(source);
	memset(&reader, 0, sizeof(reader));
	reader.mm = mm;
	reader.note_node = -1;
	p = source;
	end = source + length;

	while (p < end && !mm->error) {
		const char* line_end = memchr(p, '\n', (size_t)(end - p));
		const char* s;
		const char* e;
		if (!line_end) line_end = end;
		reader.line++;
		reader.line_start = p;
		reader.line_end = line_end;
		s = p;
		e = line_end;
		rvn_mm_trim(&s, &e);
		p = line_end < end ? line_end + 1 : end;

		/* front matter: --- title: ... --- before the diagram */
		if (first && e - s == 3 && memcmp(s, "---", 3) == 0) {
			front_matter = 1;
			first = 0;
			continue;
		}
		if (front_matter) {
			if (e - s == 3 && memcmp(s, "---", 3) == 0) front_matter = 0;
			else if (rvn_mm_starts(s, e, "title:")) {
				rvn_free(mm->title);
				mm->title = rvn_mm_label(s + 6, e);
			}
			continue;
		}
		if (s >= e) continue;
		first = 0;
		/* %%{ directives }%% and %% comments */
		if (rvn_mm_starts(s, e, "%%{")) {
			rvn_mm_directive(mm, s, e);
			continue;
		}
		if (rvn_mm_starts(s, e, "%%")) continue;
		if (kind == R_MERMAID_NONE) {
			const char* rest;
			kind = rvn_mm_header(&reader, s, e, &rest);
			if (kind == R_MERMAID_NONE) break;
			mm->kind = kind;
			rest = rvn_mm_skip(rest, e);
			if (kind == R_MERMAID_FLOWCHART) {
				/* graph TD; A --> B */
				const char* dir_end = rest;
				char direction;
				while (dir_end < e && !rvn_mm_space(*dir_end) && *dir_end != ';') dir_end++;
				direction = rvn_mm_direction(rest, dir_end);
				if (direction) {
					mm->direction = direction;
					rest = dir_end;
				} else if (rest < e && *rest != ';') {
					rvn_mm_error(&reader, "unknown direction (TB, TD, BT, LR or RL)");
					break;
				}
				rest = rvn_mm_skip(rest, e);
				if (rest < e && *rest == ';') rest++;
				if (rest < e) rvn_mm_statements(&reader, kind, rest, e);
			} else if (kind == R_MERMAID_PIE) {
				if (rest < e) rvn_mm_pie_statement(&reader, rest, e);
			} else if (rest < e && *rest != ';') {
				char excerpt[32];
				rvn_mm_excerpt(rest, e, excerpt, sizeof(excerpt));
				rvn_mm_error(&reader, "unexpected \"%s\" after the kind of diagram", excerpt);
			}
			continue;
		}
		rvn_mm_statements(&reader, kind, s, e);
	}
	if (!mm->error && reader.in_description) {
		rvn_mm_error(&reader, "accDescr { is not closed by }");
	}
	if (!mm->error && reader.note_node >= 0) rvn_mm_error(&reader, "the note is not closed by \"end note\"");
	if (!mm->error && reader.depth > 0) {
		reader.line_start = NULL;
		if (kind == R_MERMAID_STATE) rvn_mm_error(&reader, "a composite state is not closed by \"}\"");
		else if (kind == R_MERMAID_SEQUENCE) rvn_mm_error(&reader, "a block is not closed by \"end\"");
		else rvn_mm_error(&reader, "a subgraph is not closed by \"end\"");
	}
	if (!mm->error) {
		rvn_mm_apply_styles(&reader);
		if (kind == R_MERMAID_FLOWCHART || kind == R_MERMAID_STATE) rvn_mm_resolve_cluster_links(mm);
		if (kind == R_MERMAID_PIE && mm->slice_count == 0 && !mm->title) {
			/* an empty pie is still a pie */
		}
	}
	if (mm->error) mm->kind = R_MERMAID_NONE;

	rvn_free(reader.table);
	for (i = 0; i < reader.class_count; i++) rvn_free(reader.classes[i].name);
	rvn_free(reader.classes);
	for (i = 0; i < reader.apply_count; i++) rvn_free(reader.applies[i].name);
	rvn_free(reader.applies);
	rvn_free(reader.description.data);
	rvn_free(reader.note.data);
	return mm;
}

void rvn_mm_free(rvn_mm_t* mm) {
	uint32_t i;
	if (!mm) return;
	for (i = 0; i < mm->node_count; i++) {
		rvn_free(mm->nodes[i].id);
		rvn_free(mm->nodes[i].label);
	}
	for (i = 0; i < mm->edge_count; i++) rvn_free(mm->edges[i].label);
	for (i = 0; i < mm->cluster_count; i++) {
		rvn_free(mm->clusters[i].id);
		rvn_free(mm->clusters[i].title);
	}
	for (i = 0; i < mm->event_count; i++) rvn_free(mm->events[i].text);
	for (i = 0; i < mm->slice_count; i++) rvn_free(mm->slices[i].label);
	rvn_free(mm->nodes);
	rvn_free(mm->edges);
	rvn_free(mm->clusters);
	rvn_free(mm->events);
	rvn_free(mm->slices);
	rvn_free(mm->items);
	rvn_free(mm->points);
	rvn_free(mm->strings.data);
	rvn_free(mm->error);
	rvn_free(mm->error_text);
	rvn_free(mm->title);
	rvn_free(mm->acc_title);
	rvn_free(mm->acc_description);
	rvn_free(mm->theme);
	rvn_free(mm);
}

/* ---------------------------------------------------------------------------
 *  The scene
 * ------------------------------------------------------------------------- */

rvn_mm_item_t* rvn_mm_add_item(rvn_mm_t* mm, int kind) {
	rvn_mm_item_t* item;
	RVN_ARRAY_RESERVE(mm->items, mm->item_cap, mm->item_count + 1);
	item = &mm->items[mm->item_count++];
	memset(item, 0, sizeof(*item));
	item->kind = (uint8_t)kind;
	item->text = RVN_MM_C_TEXT;
	item->text_scale = 1.0f;
	item->stroke_width = 1.0f;
	item->align = 0.5f;
	return item;
}

void rvn_mm_item_set_text(rvn_mm_t* mm, rvn_mm_item_t* item, const char* text, int length) {
	if (!text) return;
	if (length < 0) length = (int)strlen(text);
	item->text_start = (uint32_t)mm->strings.length;
	item->text_length = (uint32_t)length;
	rvn_text_append(&mm->strings, text, (size_t)length);
	/* each text ends with a '\0' of its own */
	rvn_text_append(&mm->strings, "", 1);
}

uint32_t rvn_mm_add_point(rvn_mm_t* mm, float x, float y) {
	RVN_ARRAY_RESERVE(mm->points, mm->point_cap, (mm->point_count + 1) * 2u);
	mm->points[mm->point_count * 2u] = x;
	mm->points[mm->point_count * 2u + 1u] = y;
	return mm->point_count++;
}

void rvn_mm_layout(rvn_mm_t* mm, const rvn_mm_measure_t* measure) {
	if (!mm || !measure) return;
	mm->item_count = 0;
	mm->point_count = 0;
	mm->strings.length = 0;
	mm->width = mm->height = 0.0f;
	mm->font_size = measure->size;
	mm->line_height = measure->line_height;
	if (mm->error) rvn_mm_layout_error(mm, measure);
	else if (mm->kind == R_MERMAID_FLOWCHART || mm->kind == R_MERMAID_STATE) rvn_mm_layout_graph(mm, measure);
	else if (mm->kind == R_MERMAID_SEQUENCE) rvn_mm_layout_sequence(mm, measure);
	else if (mm->kind == R_MERMAID_PIE) rvn_mm_layout_pie(mm, measure);
	mm->laid = R_TRUE;
}

void rvn_mm_summary(const rvn_mm_t* mm, rvn_text_t* out) {
	uint32_t i, nodes = 0, edges = 0;
	if (!mm) return;
	if (mm->error) {
		rvn_text_printf(out, "Mermaid diagram with an error on line %d: %s", mm->error_line, mm->error);
		return;
	}
	if (mm->acc_title && mm->acc_title[0]) {
		rvn_text_add(out, mm->acc_title);
		rvn_text_add(out, ", ");
	} else if (mm->title && mm->title[0]) {
		rvn_text_add(out, mm->title);
		rvn_text_add(out, ", ");
	}
	switch (mm->kind) {
		case R_MERMAID_FLOWCHART:
		case R_MERMAID_STATE:
			for (i = 0; i < mm->node_count; i++) {
				uint8_t shape = mm->nodes[i].shape;
				if (shape != RVN_MM_NOTE && (mm->kind == R_MERMAID_FLOWCHART || (shape != RVN_MM_START && shape != RVN_MM_END))) nodes++;
			}
			for (i = 0; i < mm->edge_count; i++) {
				if (mm->edges[i].length > 0) edges++;
			}
			if (mm->kind == R_MERMAID_FLOWCHART) {
				rvn_text_printf(out, "flowchart with %u node%s and %u link%s", nodes, nodes == 1 ? "" : "s", edges, edges == 1 ? "" : "s");
				if (mm->cluster_count) rvn_text_printf(out, " in %u subgraph%s", mm->cluster_count, mm->cluster_count == 1 ? "" : "s");
			} else {
				rvn_text_printf(out, "state diagram with %u state%s and %u transition%s", nodes, nodes == 1 ? "" : "s", edges, edges == 1 ? "" : "s");
			}
			break;
		case R_MERMAID_SEQUENCE:
			for (i = 0; i < mm->event_count; i++) {
				if (mm->events[i].kind == RVN_MM_EV_MESSAGE) edges++;
			}
			rvn_text_printf(out, "sequence diagram with %u participant%s and %u message%s", mm->node_count, mm->node_count == 1 ? "" : "s",
				edges, edges == 1 ? "" : "s");
			break;
		case R_MERMAID_PIE: {
			double total = 0.0;
			rvn_text_printf(out, "pie chart with %u slice%s", mm->slice_count, mm->slice_count == 1 ? "" : "s");
			for (i = 0; i < mm->slice_count; i++) total += mm->slices[i].value;
			for (i = 0; i < mm->slice_count && i < 20u; i++) {
				rvn_text_printf(out, "%s %s %.0f%%", i ? "," : ":", mm->slices[i].label,
					total > 0.0 ? mm->slices[i].value * 100.0 / total : 0.0);
			}
			break;
		}
		default:
			rvn_text_add(out, "empty Mermaid diagram");
			break;
	}
}

/* ---------------------------------------------------------------------------
 *  Themes of Mermaid
 * ------------------------------------------------------------------------- */

int rvn_mm_theme_colors(const char* theme, rvn_color_t* c) {
	static const uint32_t slices_light[12] = {
		0xECECFF, 0xFFFFDE, 0xB9B9FF, 0xD8FFD8, 0xFFD8D8, 0xD8F5FF, 0xFFE9C9, 0xE0D8FF, 0xCDE8D0, 0xF8D8EE, 0xD0D0D0, 0xFFF4B3
	};
	static const uint32_t slices_forest[12] = {
		0xCDE498, 0xCDFFB2, 0x6EAA49, 0xA6D49F, 0xEBF4C6, 0x9DC88D, 0x8AB17D, 0xC5E1A5, 0x7CA982, 0xDCEDC2, 0xB5D99C, 0xE8F3D6
	};
	static const uint32_t slices_dark[12] = {
		0x1F2020, 0x0B0000, 0x4D5D61, 0x3F5E3B, 0x5E3B3B, 0x3B4A5E, 0x5E553B, 0x4B3B5E, 0x3B5E59, 0x5E3B55, 0x555555, 0x2F3B5E
	};
	static const uint32_t slices_neutral[12] = {
		0xEEEEEE, 0xD6D6D6, 0xBDBDBD, 0xA6A6A6, 0x8E8E8E, 0x777777, 0xE4E4E4, 0xCCCCCC, 0xB3B3B3, 0x999999, 0x808080, 0x666666
	};
	const uint32_t* slices;
	uint32_t node, border, text, line, cluster, cluster_border, note, note_border, background, label;
	int i;
	if (!theme) return R_FALSE;
	if (rvn_stricmp(theme, "default") == 0 || rvn_stricmp(theme, "base") == 0) {
		node = 0xECECFF; border = 0x9370DB; text = 0x333333; line = 0x333333; cluster = 0xFFFFDE; cluster_border = 0xAAAA33;
		note = 0xFFF5AD; note_border = 0xAAAA33; background = 0xFFFFFF; label = 0xE8E8E8; slices = slices_light;
	} else if (rvn_stricmp(theme, "forest") == 0) {
		node = 0xCDE498; border = 0x13540C; text = 0x000000; line = 0x008000; cluster = 0xCDFFB2; cluster_border = 0x6EAA49;
		note = 0xFFF5AD; note_border = 0x6EAA49; background = 0xFFFFFF; label = 0xE8E8E8; slices = slices_forest;
	} else if (rvn_stricmp(theme, "dark") == 0) {
		node = 0x1F2020; border = 0xCCCCCC; text = 0xCCCCCC; line = 0xD3D3D3; cluster = 0x2D3637; cluster_border = 0x777777;
		note = 0x4E4E4E; note_border = 0x888888; background = 0x333333; label = 0x585858; slices = slices_dark;
	} else if (rvn_stricmp(theme, "neutral") == 0) {
		node = 0xEEEEEE; border = 0x999999; text = 0x333333; line = 0x666666; cluster = 0xF4F4F4; cluster_border = 0x999999;
		note = 0xEEEEEE; note_border = 0x999999; background = 0xFFFFFF; label = 0xE8E8E8; slices = slices_neutral;
	} else {
		return R_FALSE;
	}
	c[RVN_MM_C_NONE] = 0;
	c[RVN_MM_C_EXPLICIT] = 0;
	c[RVN_MM_C_BACKGROUND] = rvn_hex(background, 255);
	c[RVN_MM_C_NODE] = rvn_hex(node, 255);
	c[RVN_MM_C_NODE_BORDER] = rvn_hex(border, 255);
	c[RVN_MM_C_TEXT] = rvn_hex(text, 255);
	c[RVN_MM_C_LINE] = rvn_hex(line, 255);
	c[RVN_MM_C_CLUSTER] = rvn_hex(cluster, 255);
	c[RVN_MM_C_CLUSTER_BORDER] = rvn_hex(cluster_border, 255);
	c[RVN_MM_C_NOTE] = rvn_hex(note, 255);
	c[RVN_MM_C_NOTE_BORDER] = rvn_hex(note_border, 255);
	c[RVN_MM_C_NOTE_TEXT] = rvn_hex(text, 255);
	c[RVN_MM_C_LABEL] = rvn_hex(label, 255);
	c[RVN_MM_C_ACTIVATION] = rvn_hex(rvn_stricmp(theme, "dark") == 0 ? 0x81B1DB : 0xF4F4F4, 255);
	c[RVN_MM_C_BLOCK_BORDER] = rvn_hex(cluster_border, 255);
	c[RVN_MM_C_ACCENT] = rvn_hex(border, 255);
	c[RVN_MM_C_ACCENT_TEXT] = rvn_hex(background, 255);
	c[RVN_MM_C_ERROR] = rvn_hex(0xFFE4E1, 255);
	c[RVN_MM_C_ERROR_BORDER] = rvn_hex(0xD9534F, 255);
	c[RVN_MM_C_ERROR_TEXT] = rvn_hex(0x552222, 255);
	for (i = 0; i < 12; i++) c[RVN_MM_C_SLICE + i] = rvn_hex(slices[i], 255);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  SVG
 * ------------------------------------------------------------------------- */

static void rvn_mm_svg_color(rvn_text_t* out, const char* attribute, rvn_color_t color) {
	uint32_t a = rvn_color_a(color);
	if (a == 0) {
		rvn_text_printf(out, " %s=\"none\"", attribute);
		return;
	}
	rvn_text_printf(out, " %s=\"#%02x%02x%02x\"", attribute, color & 0xFFu, (color >> 8) & 0xFFu, (color >> 16) & 0xFFu);
	if (a < 255u) rvn_text_printf(out, " %s-opacity=\"%.3f\"", attribute, (double)a / 255.0);
}

static rvn_color_t rvn_mm_resolve(const rvn_color_t* colors, uint8_t slot, rvn_color_t explicit_color) {
	if (slot == RVN_MM_C_EXPLICIT) return explicit_color;
	if (slot >= RVN_MM_C_COUNT_) return 0;
	return colors[slot];
}

static void rvn_mm_svg_escape(rvn_text_t* out, const char* text, size_t length) {
	size_t i;
	for (i = 0; i < length; i++) {
		switch (text[i]) {
			case '<': rvn_text_add(out, "&lt;"); break;
			case '>': rvn_text_add(out, "&gt;"); break;
			case '&': rvn_text_add(out, "&amp;"); break;
			case '"': rvn_text_add(out, "&quot;"); break;
			default: rvn_text_append(out, text + i, 1); break;
		}
	}
}

int rvn_mm_shape_polygon(const rvn_mm_item_t* item, float* xy, int max) {
	float x = item->x, y = item->y, w = item->w, h = item->h;
	float pts[12];
	int n = 0, i;
#define RVN_MM_PT(px, py) do { pts[n * 2] = (px); pts[n * 2 + 1] = (py); n++; } while (0)
	switch (item->shape) {
		case RVN_MM_RHOMBUS:
			RVN_MM_PT(x + w * 0.5f, y); RVN_MM_PT(x + w, y + h * 0.5f); RVN_MM_PT(x + w * 0.5f, y + h); RVN_MM_PT(x, y + h * 0.5f);
			break;
		case RVN_MM_HEXAGON: {
			float s = h * 0.25f;
			RVN_MM_PT(x + s, y); RVN_MM_PT(x + w - s, y); RVN_MM_PT(x + w, y + h * 0.5f);
			RVN_MM_PT(x + w - s, y + h); RVN_MM_PT(x + s, y + h); RVN_MM_PT(x, y + h * 0.5f);
			break;
		}
		case RVN_MM_PARALLELOGRAM: {
			float s = h * 0.3f;
			RVN_MM_PT(x + s, y); RVN_MM_PT(x + w, y); RVN_MM_PT(x + w - s, y + h); RVN_MM_PT(x, y + h);
			break;
		}
		case RVN_MM_PARALLELOGRAM_ALT: {
			float s = h * 0.3f;
			RVN_MM_PT(x, y); RVN_MM_PT(x + w - s, y); RVN_MM_PT(x + w, y + h); RVN_MM_PT(x + s, y + h);
			break;
		}
		case RVN_MM_TRAPEZOID: {
			float s = h * 0.3f;
			RVN_MM_PT(x + s, y); RVN_MM_PT(x + w - s, y); RVN_MM_PT(x + w, y + h); RVN_MM_PT(x, y + h);
			break;
		}
		case RVN_MM_TRAPEZOID_ALT: {
			float s = h * 0.3f;
			RVN_MM_PT(x, y); RVN_MM_PT(x + w, y); RVN_MM_PT(x + w - s, y + h); RVN_MM_PT(x + s, y + h);
			break;
		}
		case RVN_MM_ASYMMETRIC: {
			float s = h * 0.35f;
			RVN_MM_PT(x, y); RVN_MM_PT(x + w, y); RVN_MM_PT(x + w, y + h); RVN_MM_PT(x, y + h); RVN_MM_PT(x + s, y + h * 0.5f);
			break;
		}
		case RVN_MM_TAG: {
			float s = h * 0.35f;
			RVN_MM_PT(x, y); RVN_MM_PT(x + w, y); RVN_MM_PT(x + w, y + h - s); RVN_MM_PT(x + w - s, y + h); RVN_MM_PT(x, y + h);
			break;
		}
		default:
			return 0;
	}
#undef RVN_MM_PT
	if (n > max) n = max;
	for (i = 0; i < n * 2; i++) xy[i] = pts[i];
	return n;
}

void rvn_mm_svg(const rvn_mm_t* mm, const rvn_color_t* colors, const char* font_family, rvn_text_t* out) {
	uint32_t i;
	float size, line_height;
	if (!mm) return;
	size = mm->font_size > 0.0f ? mm->font_size : 14.0f;
	line_height = mm->line_height > 0.0f ? mm->line_height : size * 1.3f;
	rvn_text_printf(out, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%.0f\" height=\"%.0f\" viewBox=\"0 0 %.2f %.2f\">\n",
		(double)ceilf(mm->width), (double)ceilf(mm->height), (double)mm->width, (double)mm->height);
	rvn_text_add(out, "<rect width=\"100%\" height=\"100%\"");
	rvn_mm_svg_color(out, "fill", colors[RVN_MM_C_BACKGROUND]);
	rvn_text_add(out, "/>\n");
	for (i = 0; i < mm->item_count; i++) {
		const rvn_mm_item_t* item = &mm->items[i];
		rvn_color_t fill = rvn_mm_resolve(colors, item->fill, item->fill_color);
		rvn_color_t stroke = rvn_mm_resolve(colors, item->stroke, item->stroke_color);
		switch (item->kind) {
			case RVN_MM_ITEM_SHAPE: {
				float xy[64];
				int n = rvn_mm_shape_polygon(item, xy, 32), k;
				if (item->shape == RVN_MM_CYLINDER) {
					float ry = rvn_minf(item->h * 0.15f, item->w * 0.1f + 3.0f), rx = item->w * 0.5f;
					rvn_text_printf(out, "<path d=\"M%.2f %.2f a%.2f %.2f 0 0 0 %.2f 0 v%.2f a%.2f %.2f 0 0 1 %.2f 0 Z M%.2f %.2f a%.2f %.2f 0 0 0 %.2f 0\"",
						(double)item->x, (double)(item->y + ry), (double)rx, (double)ry, (double)item->w, (double)(item->h - 2.0f * ry),
						(double)rx, (double)ry, (double)-item->w, (double)item->x, (double)(item->y + ry), (double)rx, (double)ry, (double)item->w);
				} else if (n > 0) {
					rvn_text_add(out, "<polygon points=\"");
					for (k = 0; k < n; k++) rvn_text_printf(out, "%s%.2f,%.2f", k ? " " : "", (double)xy[k * 2], (double)xy[k * 2 + 1]);
					rvn_text_add(out, "\"");
				} else if (item->shape == RVN_MM_CIRCLE || item->shape == RVN_MM_DOUBLE_CIRCLE || item->shape == RVN_MM_START || item->shape == RVN_MM_END) {
					rvn_text_printf(out, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"", (double)(item->x + item->w * 0.5f), (double)(item->y + item->h * 0.5f),
						(double)(item->w * 0.5f));
				} else {
					float radius = item->shape == RVN_MM_ROUND ? 5.0f : (item->shape == RVN_MM_STADIUM ? item->h * 0.5f : (item->shape == RVN_MM_BAR ? 2.0f : 0.0f));
					rvn_text_printf(out, "<rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" rx=\"%.2f\"", (double)item->x, (double)item->y,
						(double)item->w, (double)item->h, (double)radius);
				}
				rvn_mm_svg_color(out, "fill", fill);
				rvn_mm_svg_color(out, "stroke", stroke);
				if (rvn_color_a(stroke)) rvn_text_printf(out, " stroke-width=\"%.2f\"", (double)item->stroke_width);
				if (item->line == RVN_MM_LINE_DOTTED) rvn_text_add(out, " stroke-dasharray=\"4 3\"");
				rvn_text_add(out, "/>\n");
				if (item->shape == RVN_MM_DOUBLE_CIRCLE || item->shape == RVN_MM_END) {
					float inset = item->shape == RVN_MM_END ? item->w * 0.25f : 4.0f;
					rvn_text_printf(out, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"", (double)(item->x + item->w * 0.5f), (double)(item->y + item->h * 0.5f),
						(double)(item->w * 0.5f - inset));
					if (item->shape == RVN_MM_END) rvn_mm_svg_color(out, "fill", stroke);
					else rvn_text_add(out, " fill=\"none\"");
					rvn_mm_svg_color(out, "stroke", stroke);
					rvn_text_add(out, "/>\n");
				}
				if (item->shape == RVN_MM_SUBROUTINE) {
					float inset = rvn_minf(8.0f, item->w * 0.1f);
					rvn_text_printf(out, "<path d=\"M%.2f %.2f v%.2f M%.2f %.2f v%.2f\"", (double)(item->x + inset), (double)item->y, (double)item->h,
						(double)(item->x + item->w - inset), (double)item->y, (double)item->h);
					rvn_mm_svg_color(out, "stroke", stroke);
					rvn_text_add(out, "/>\n");
				}
				break;
			}
			case RVN_MM_ITEM_PATH: {
				uint32_t k;
				if (item->point_count < 2) break;
				rvn_text_add(out, "<polyline fill=\"none\" points=\"");
				for (k = 0; k < item->point_count; k++) {
					const float* pt = &mm->points[(item->first_point + k) * 2u];
					rvn_text_printf(out, "%s%.2f,%.2f", k ? " " : "", (double)pt[0], (double)pt[1]);
				}
				rvn_text_add(out, "\"");
				rvn_mm_svg_color(out, "stroke", stroke);
				rvn_text_printf(out, " stroke-width=\"%.2f\" stroke-linecap=\"round\" stroke-linejoin=\"round\"", (double)item->stroke_width);
				if (item->line == RVN_MM_LINE_DOTTED) rvn_text_add(out, " stroke-dasharray=\"3 3\"");
				rvn_text_add(out, "/>\n");
				{
					/* the heads, as the painter draws them */
					int end;
					for (end = 0; end < 2; end++) {
						uint8_t head = end ? item->head_end : item->head_start;
						const float* a = &mm->points[(item->first_point + (end ? item->point_count - 1u : 0u)) * 2u];
						const float* b = &mm->points[(item->first_point + (end ? item->point_count - 2u : 1u)) * 2u];
						float dx = a[0] - b[0], dy = a[1] - b[1], len = sqrtf(dx * dx + dy * dy), s = 8.0f + item->stroke_width;
						if (head == RVN_MM_HEAD_NONE || len <= 0.0f) continue;
						dx /= len;
						dy /= len;
						if (head == RVN_MM_HEAD_ARROW) {
							rvn_text_printf(out, "<polygon points=\"%.2f,%.2f %.2f,%.2f %.2f,%.2f\"", (double)a[0], (double)a[1],
								(double)(a[0] - dx * s - dy * s * 0.45f), (double)(a[1] - dy * s + dx * s * 0.45f),
								(double)(a[0] - dx * s + dy * s * 0.45f), (double)(a[1] - dy * s - dx * s * 0.45f));
							rvn_mm_svg_color(out, "fill", stroke);
							rvn_text_add(out, "/>\n");
						} else if (head == RVN_MM_HEAD_OPEN) {
							rvn_text_printf(out, "<polyline fill=\"none\" points=\"%.2f,%.2f %.2f,%.2f %.2f,%.2f\"", (double)(a[0] - dx * s * 0.6f - dy * s * 0.45f),
								(double)(a[1] - dy * s * 0.6f + dx * s * 0.45f), (double)a[0], (double)a[1], (double)(a[0] - dx * s * 0.6f + dy * s * 0.45f),
								(double)(a[1] - dy * s * 0.6f - dx * s * 0.45f));
							rvn_mm_svg_color(out, "stroke", stroke);
							rvn_text_printf(out, " stroke-width=\"%.2f\"/>\n", (double)item->stroke_width);
						} else if (head == RVN_MM_HEAD_CROSS) {
							float h = s * 0.4f;
							rvn_text_printf(out, "<path d=\"M%.2f %.2f L%.2f %.2f M%.2f %.2f L%.2f %.2f\"", (double)(a[0] - h), (double)(a[1] - h), (double)(a[0] + h),
								(double)(a[1] + h), (double)(a[0] - h), (double)(a[1] + h), (double)(a[0] + h), (double)(a[1] - h));
							rvn_mm_svg_color(out, "stroke", stroke);
							rvn_text_printf(out, " stroke-width=\"%.2f\"/>\n", (double)(item->stroke_width + 0.5f));
						} else if (head == RVN_MM_HEAD_CIRCLE) {
							rvn_text_printf(out, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"", (double)(a[0] - dx * 4.0f), (double)(a[1] - dy * 4.0f), 4.0);
							rvn_mm_svg_color(out, "fill", colors[RVN_MM_C_BACKGROUND]);
							rvn_mm_svg_color(out, "stroke", stroke);
							rvn_text_add(out, "/>\n");
						}
					}
				}
				break;
			}
			case RVN_MM_ITEM_WEDGE: {
				float cx = item->x, cy = item->y, r = item->w;
				float x0 = cx + sinf(item->a0) * r, y0 = cy - cosf(item->a0) * r;
				float x1 = cx + sinf(item->a1) * r, y1 = cy - cosf(item->a1) * r;
				if (item->a1 - item->a0 >= 6.2831f) {
					rvn_text_printf(out, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"", (double)cx, (double)cy, (double)r);
				} else {
					rvn_text_printf(out, "<path d=\"M%.2f %.2f L%.2f %.2f A%.2f %.2f 0 %d 1 %.2f %.2f Z\"", (double)cx, (double)cy, (double)x0, (double)y0,
						(double)r, (double)r, item->a1 - item->a0 > 3.14159265f ? 1 : 0, (double)x1, (double)y1);
				}
				rvn_mm_svg_color(out, "fill", fill);
				rvn_mm_svg_color(out, "stroke", stroke);
				rvn_text_printf(out, " stroke-width=\"%.2f\"/>\n", (double)item->stroke_width);
				break;
			}
			default:
				break;
		}
		/* the text of a shape or a text item */
		if (item->text_length && (item->kind == RVN_MM_ITEM_SHAPE || item->kind == RVN_MM_ITEM_TEXT)) {
			const char* text = rvn_mm_item_text(mm, item);
			const char* line = text;
			const char* stop = text + item->text_length;
			rvn_color_t color = rvn_mm_resolve(colors, item->text, item->text_color);
			float item_line = line_height * item->text_scale;
			int lines = 1, k = 0;
			const char* c;
			float top;
			for (c = text; c < stop; c++) if (*c == '\n') lines++;
			top = item->kind == RVN_MM_ITEM_TEXT ? item->y : item->y + (item->h - item_line * (float)lines) * 0.5f;
			while (line <= stop) {
				const char* line_end = memchr(line, '\n', (size_t)(stop - line));
				float x = item->x + item->w * item->align;
				if (!line_end) line_end = stop;
				rvn_text_printf(out, "<text x=\"%.2f\" y=\"%.2f\" font-family=\"%s\" font-size=\"%.2f\" text-anchor=\"%s\" dominant-baseline=\"central\"",
					(double)x, (double)(top + item_line * ((float)k + 0.5f)), font_family ? font_family : "sans-serif", (double)(size * item->text_scale),
					item->align < 0.25f ? "start" : (item->align > 0.75f ? "end" : "middle"));
				if (item->bold) rvn_text_add(out, " font-weight=\"bold\"");
				rvn_mm_svg_color(out, "fill", color);
				rvn_text_add(out, ">");
				rvn_mm_svg_escape(out, line, (size_t)(line_end - line));
				rvn_text_add(out, "</text>\n");
				k++;
				if (line_end >= stop) break;
				line = line_end + 1;
			}
		}
	}
	rvn_text_add(out, "</svg>\n");
}
