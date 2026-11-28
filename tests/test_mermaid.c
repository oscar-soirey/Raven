/**
 * Raven tests - Mermaid diagrams (core/mermaid.c, core/mermaid_layout.c,
 * widgets/mermaid_view.c, the ```mermaid blocks of the Markdown).
 *
 * The reader, for each kind of diagram: what valid texts give, and the line
 * of the first error of invalid ones. Random texts (fragments of the
 * language and random bytes) are read and laid out without crashing, in a
 * bounded time. The layout of graphs: no two nodes overlap, each path joins
 * the nodes of its link, the layers follow the direction, the subgraphs hold
 * their nodes and no other, the same text gives the same scene, and 200
 * nodes are laid out in a few milliseconds. Then the widget (kind, errors,
 * nodes, fit, zoom, SVG, XML, accessibility, theme colors) and the Markdown
 * (a block becomes a diagram, or stays code when turned off).
 */

#include "test.h"
#include "../src/core/mermaid.h"

#include <stdlib.h>
#include <time.h>

/* (internal: widgets/mermaid_view.c and widgets/label.c) */
struct rvn_widget_t;
int rvn_label_diagram_count(struct rvn_widget_t* widget);
struct rvn_widget_t* rvn_widget_get(Rid id);
void rvn_mermaid_colors(const struct rvn_widget_t* widget, const struct rvn_mm_t* mm, rvn_color_t* colors);

static Rid g_window;

/* ---- helpers ---- */

static float fake_width(void* user, const char* text, int length, float size, int bold) {
	(void)user;
	(void)text;
	return (float)length * size * (bold ? 0.6f : 0.55f);
}

static rvn_mm_measure_t fake_measure(void) {
	rvn_mm_measure_t m;
	m.width = fake_width;
	m.user = NULL;
	m.size = 14.0f;
	m.line_height = 18.0f;
	return m;
}

static rvn_mm_t* parse(const char* text) {
	return rvn_mm_parse(text, -1);
}

static rvn_mm_t* parse_laid(const char* text) {
	rvn_mm_t* mm = rvn_mm_parse(text, -1);
	rvn_mm_measure_t m = fake_measure();
	rvn_mm_layout(mm, &m);
	return mm;
}

static int node_index(const rvn_mm_t* mm, const char* id) {
	uint32_t i;
	for (i = 0; i < mm->node_count; i++) {
		if (strcmp(mm->nodes[i].id, id) == 0) return (int)i;
	}
	return -1;
}

static const rvn_mm_node_t* node(const rvn_mm_t* mm, const char* id) {
	int i = node_index(mm, id);
	return i >= 0 ? &mm->nodes[i] : NULL;
}

/* The link from a to b (by name), or NULL. */
static const rvn_mm_edge_t* edge(const rvn_mm_t* mm, const char* a, const char* b) {
	int from = node_index(mm, a), to = node_index(mm, b);
	uint32_t i;
	for (i = 0; i < mm->edge_count; i++) {
		if (mm->edges[i].from == from && mm->edges[i].to == to) return &mm->edges[i];
	}
	return NULL;
}

static int error_line(const char* text) {
	rvn_mm_t* mm = parse(text);
	int line = mm->error ? mm->error_line : 0;
	rvn_mm_free(mm);
	return line;
}

static double now_ms(void) {
	return (double)clock() * 1000.0 / (double)CLOCKS_PER_SEC;
}

static uint32_t g_seed = 1234567u;
static uint32_t rnd(void) {
	g_seed = g_seed * 1664525u + 1013904223u;
	return g_seed >> 8;
}

/* ---- the reader ---- */

static void test_flowchart(void) {
	rvn_mm_t* mm = parse(
		"flowchart TD\n"
		"    %% a comment\n"
		"    A[Start] --> B{Is it?}\n"
		"    B -->|Yes| C[OK]\n"
		"    C --> D[Rethink]\n"
		"    D --> B\n"
		"    B ---->|No| E[End]\n");
	CHECK(mm->kind == R_MERMAID_FLOWCHART);
	CHECK(mm->error == NULL);
	CHECK(mm->direction == 'T');
	CHECK(mm->node_count == 5);
	CHECK(mm->edge_count == 5);
	CHECK(node(mm, "B") && node(mm, "B")->shape == RVN_MM_RHOMBUS);
	CHECK_STR(node(mm, "B")->label, "Is it?");
	CHECK(edge(mm, "B", "C") && edge(mm, "B", "C")->label && strcmp(edge(mm, "B", "C")->label, "Yes") == 0);
	CHECK(edge(mm, "B", "E") && edge(mm, "B", "E")->length == 3);
	CHECK(edge(mm, "A", "B") && edge(mm, "A", "B")->label == NULL && edge(mm, "A", "B")->head_end == RVN_MM_HEAD_ARROW);
	rvn_mm_free(mm);

	/* every shape */
	mm = parse("graph LR\n a[rect] --> b(round) --> c([stadium]) --> d[[sub]] --> e[(db)] --> f((circle)) --> g(((double)))\n"
		" h{rhombus} --> i{{hex}} --> j[/para/] --> k[\\alt\\] --> l[/trap\\] --> m[\\trap alt/] --> n>flag]\n");
	CHECK(mm->kind == R_MERMAID_FLOWCHART && mm->error == NULL);
	CHECK(mm->direction == 'L');
	CHECK(node(mm, "a")->shape == RVN_MM_RECT);
	CHECK(node(mm, "b")->shape == RVN_MM_ROUND);
	CHECK(node(mm, "c")->shape == RVN_MM_STADIUM);
	CHECK(node(mm, "d")->shape == RVN_MM_SUBROUTINE);
	CHECK(node(mm, "e")->shape == RVN_MM_CYLINDER);
	CHECK(node(mm, "f")->shape == RVN_MM_CIRCLE);
	CHECK(node(mm, "g")->shape == RVN_MM_DOUBLE_CIRCLE);
	CHECK(node(mm, "h")->shape == RVN_MM_RHOMBUS);
	CHECK(node(mm, "i")->shape == RVN_MM_HEXAGON);
	CHECK(node(mm, "j")->shape == RVN_MM_PARALLELOGRAM);
	CHECK(node(mm, "k")->shape == RVN_MM_PARALLELOGRAM_ALT);
	CHECK(node(mm, "l")->shape == RVN_MM_TRAPEZOID);
	CHECK(node(mm, "m")->shape == RVN_MM_TRAPEZOID_ALT);
	CHECK(node(mm, "n")->shape == RVN_MM_ASYMMETRIC);
	CHECK_STR(node(mm, "e")->label, "db");
	CHECK_STR(node(mm, "m")->label, "trap alt");
	rvn_mm_free(mm);

	/* the links */
	mm = parse("flowchart TB\nA-.->B\nC==>D\nE---F\nG-- some text -->H\nI -. dotted text .-> J\nK == thick text ==> L\n"
		"M <--> N\nO --x P\nQ --o R\nS ~~~ T\nU -.- V\nW === X\n");
	CHECK(mm->error == NULL);
	CHECK(edge(mm, "A", "B")->line == RVN_MM_LINE_DOTTED && edge(mm, "A", "B")->head_end == RVN_MM_HEAD_ARROW);
	CHECK(edge(mm, "C", "D")->line == RVN_MM_LINE_THICK && edge(mm, "C", "D")->head_end == RVN_MM_HEAD_ARROW);
	CHECK(edge(mm, "E", "F")->line == RVN_MM_LINE_SOLID && edge(mm, "E", "F")->head_end == RVN_MM_HEAD_NONE);
	CHECK_STR(edge(mm, "G", "H")->label, "some text");
	CHECK(edge(mm, "G", "H")->head_end == RVN_MM_HEAD_ARROW);
	CHECK_STR(edge(mm, "I", "J")->label, "dotted text");
	CHECK(edge(mm, "I", "J")->line == RVN_MM_LINE_DOTTED);
	CHECK_STR(edge(mm, "K", "L")->label, "thick text");
	CHECK(edge(mm, "K", "L")->line == RVN_MM_LINE_THICK);
	CHECK(edge(mm, "M", "N")->head_start == RVN_MM_HEAD_ARROW && edge(mm, "M", "N")->head_end == RVN_MM_HEAD_ARROW);
	CHECK(edge(mm, "O", "P")->head_end == RVN_MM_HEAD_CROSS);
	CHECK(edge(mm, "Q", "R")->head_end == RVN_MM_HEAD_CIRCLE);
	CHECK(edge(mm, "S", "T")->line == RVN_MM_LINE_INVISIBLE);
	CHECK(edge(mm, "U", "V")->line == RVN_MM_LINE_DOTTED && edge(mm, "U", "V")->head_end == RVN_MM_HEAD_NONE);
	CHECK(edge(mm, "W", "X")->line == RVN_MM_LINE_THICK && edge(mm, "W", "X")->head_end == RVN_MM_HEAD_NONE);
	rvn_mm_free(mm);

	/* chains, groups, semicolons, quoted texts, line breaks and entities */
	mm = parse("graph TD; A --> B --> C; D & E --> F & G\nH[\"a [quoted] text\"] --> I[\"two<br>lines #quot;x#quot;\"]\n");
	CHECK(mm->error == NULL);
	CHECK(mm->edge_count == 2 + 4 + 1);
	CHECK(edge(mm, "A", "B") && edge(mm, "B", "C") && edge(mm, "D", "G") && edge(mm, "E", "F"));
	CHECK_STR(node(mm, "H")->label, "a [quoted] text");
	CHECK_STR(node(mm, "I")->label, "two\nlines \"x\"");
	rvn_mm_free(mm);

	/* subgraphs, inside each other, and a link to a subgraph */
	mm = parse("flowchart LR\n"
		"subgraph one [The first]\n  a1 --> a2\n  subgraph inner\n    direction TB\n    i1 --> i2\n  end\nend\n"
		"subgraph two\n  b1 --> b2\nend\n"
		"c1 --> one\nb2 --> a1\n");
	CHECK(mm->error == NULL);
	CHECK(mm->cluster_count == 3);
	CHECK_STR(mm->clusters[0].title, "The first");
	CHECK(mm->clusters[1].parent == 0);
	CHECK(node(mm, "a1")->cluster == 0);
	CHECK(node(mm, "i1")->cluster == 1);
	CHECK(node(mm, "b1")->cluster == 2);
	CHECK(node(mm, "c1")->cluster == -1);
	/* "one" is no node: the link goes to a node inside it */
	CHECK(node(mm, "one") == NULL);
	CHECK(mm->node_count == 7);
	{
		uint32_t i;
		int found = 0;
		for (i = 0; i < mm->edge_count; i++) {
			if (mm->edges[i].from == node_index(mm, "c1")) found = mm->nodes[mm->edges[i].to].cluster == 0 || mm->nodes[mm->edges[i].to].cluster == 1;
		}
		CHECK(found);
	}
	rvn_mm_free(mm);

	/* styles */
	mm = parse("flowchart TD\nclassDef hot fill:#f96,stroke:#333,stroke-width:4px,color:#fff\nclassDef default fill:#eee\n"
		"A:::hot --> B\nclass C hot\nstyle B fill:#00ff00,stroke-dasharray: 5 5\nC --> D\nlinkStyle 0 stroke:#f00\nclick A callback\n");
	CHECK(mm->error == NULL);
	CHECK(node(mm, "A")->style.fill == rvn_hex(0xFF9966, 255));
	CHECK(node(mm, "A")->style.stroke_width == 4.0f);
	CHECK(node(mm, "A")->style.color == rvn_hex(0xFFFFFF, 255));
	CHECK(node(mm, "B")->style.fill == rvn_hex(0x00FF00, 255));
	CHECK(node(mm, "B")->style.dashed);
	CHECK(node(mm, "C")->style.fill == rvn_hex(0xFF9966, 255));
	CHECK(node(mm, "D")->style.fill == rvn_hex(0xEEEEEE, 255));
	rvn_mm_free(mm);

	/* front matter, a theme, accessibility */
	mm = parse("---\ntitle: My flow\n---\n%%{init: {'theme': 'forest'}}%%\nflowchart TD\naccTitle: Order flow\naccDescr: How an order goes\nA --> B\n");
	CHECK(mm->error == NULL);
	CHECK_STR(mm->title, "My flow");
	CHECK_STR(mm->theme, "forest");
	CHECK_STR(mm->acc_title, "Order flow");
	CHECK_STR(mm->acc_description, "How an order goes");
	{
		rvn_text_t out = { NULL, 0, 0 };
		rvn_mm_summary(mm, &out);
		CHECK_STR(out.data, "Order flow, flowchart with 2 nodes and 1 link");
		Rmemory_free(out.data);
	}
	rvn_mm_free(mm);

	/* errors, and their lines */
	CHECK(error_line("flowchart TD\nA --> B\nC -->\n") == 3);
	CHECK(error_line("flowchart XY\nA --> B\n") == 1);
	CHECK(error_line("flowchart TD\nA --> B\nend\n") == 3);
	CHECK(error_line("flowchart TD\n\nA[unclosed --> B\n") == 3);
	CHECK(error_line("flowchart TD\nsubgraph s\nA --> B\n") > 0);
	CHECK(error_line("flowchart TD\nA --> B\nC D\n") == 3);
	CHECK(error_line("flowchart TD\n  A -->|never closed B\n") == 2);
	CHECK(error_line("classDiagram\nA <|-- B\n") == 1);
	CHECK(error_line("nonsense\n") == 1);
	CHECK(error_line("\n\n%% only a comment\nflowchart\nA-->B\n") == 0);
	mm = parse("flowchart TD\nA --> B\nC -->\n");
	CHECK(mm->kind == R_MERMAID_NONE);
	CHECK_STR(mm->error_text, "C -->");
	rvn_mm_free(mm);
	/* empty: no diagram, no error */
	mm = parse("");
	CHECK(mm->kind == R_MERMAID_NONE && mm->error == NULL);
	rvn_mm_free(mm);
}

static void test_sequence(void) {
	rvn_mm_t* mm = parse(
		"sequenceDiagram\n"
		"    autonumber\n"
		"    participant A as Alice\n"
		"    actor B as Bob\n"
		"    A->>+B: Hello Bob\n"
		"    B-->>-A: Hi Alice\n"
		"    A-)C: async\n"
		"    C-xA: lost\n"
		"    C--xA: lost too\n"
		"    A->B: open\n"
		"    A-->B: open dotted\n"
		"    loop Every minute\n"
		"        A->>A: think\n"
		"    end\n"
		"    alt is sick\n"
		"        B->>A: Not so good\n"
		"    else is well\n"
		"        B->>A: Feeling fresh\n"
		"    end\n"
		"    opt Extra\n        A->>B: Thanks\n    end\n"
		"    par Alice to Bob\n        A->>B: Hello\n    and Alice to C\n        A->>C: Hello\n    end\n"
		"    Note right of A: a note\n"
		"    Note over A,B: shared\n"
		"    Note left of C: left\n"
		"    activate C\n    deactivate C\n"
		"    rect rgb(200, 220, 255)\n        A->>C: inside\n    end\n");
	uint32_t i, messages = 0, notes = 0, blocks = 0, sections = 0;
	CHECK(mm->kind == R_MERMAID_SEQUENCE);
	CHECK(mm->error == NULL);
	CHECK(mm->node_count == 3);
	CHECK_STR(mm->nodes[0].label, "Alice");
	CHECK(mm->nodes[1].shape == RVN_MM_ACTOR);
	CHECK_STR(mm->nodes[2].id, "C");
	for (i = 0; i < mm->event_count; i++) {
		const rvn_mm_event_t* ev = &mm->events[i];
		messages += ev->kind == RVN_MM_EV_MESSAGE;
		notes += ev->kind == RVN_MM_EV_NOTE;
		blocks += ev->kind == RVN_MM_EV_BLOCK;
		sections += ev->kind == RVN_MM_EV_SECTION;
	}
	CHECK(messages == 14);
	CHECK(notes == 3);
	CHECK(blocks == 5);
	CHECK(sections == 2);
	/* the arrows */
	CHECK(mm->events[1].kind == RVN_MM_EV_MESSAGE && mm->events[1].activation == 1 && mm->events[1].head == RVN_MM_HEAD_ARROW &&
		mm->events[1].line == RVN_MM_LINE_SOLID);
	CHECK(mm->events[2].activation == -1 && mm->events[2].line == RVN_MM_LINE_DOTTED);
	CHECK(mm->events[3].head == RVN_MM_HEAD_OPEN);
	CHECK(mm->events[4].head == RVN_MM_HEAD_CROSS && mm->events[4].line == RVN_MM_LINE_SOLID);
	CHECK(mm->events[5].head == RVN_MM_HEAD_CROSS && mm->events[5].line == RVN_MM_LINE_DOTTED);
	CHECK(mm->events[6].head == RVN_MM_HEAD_NONE && mm->events[6].line == RVN_MM_LINE_SOLID);
	CHECK(mm->events[7].head == RVN_MM_HEAD_NONE && mm->events[7].line == RVN_MM_LINE_DOTTED);
	CHECK_STR(mm->events[1].text, "Hello Bob");
	rvn_mm_free(mm);

	CHECK(error_line("sequenceDiagram\nA->>B: hi\nelse oops\n") == 3);
	CHECK(error_line("sequenceDiagram\nA->>B: hi\nend\n") == 3);
	CHECK(error_line("sequenceDiagram\nloop x\nA->>B: hi\n") > 0);
	CHECK(error_line("sequenceDiagram\nNote nowhere: x\n") == 2);
	CHECK(error_line("sequenceDiagram\nA->>B: hi\nthis is not a statement\n") == 3);
	CHECK(error_line("sequenceDiagram\n->>B: hi\n") == 2);
}

static void test_state(void) {
	rvn_mm_t* mm = parse(
		"stateDiagram-v2\n"
		"    [*] --> Still\n"
		"    Still --> [*]\n"
		"    Still --> Moving : push\n"
		"    Moving --> Still\n"
		"    Moving --> Crash\n"
		"    Crash --> [*]\n"
		"    state \"Long name\" as Long\n"
		"    Long : described\n"
		"    state Active {\n"
		"        [*] --> On\n"
		"        On --> Off\n"
		"        Off --> [*]\n"
		"    }\n"
		"    state pick <<choice>>\n"
		"    state f <<fork>>\n"
		"    Crash --> Active\n"
		"    note right of Crash : very bad\n"
		"    note left of Still\n        two lines\n        of note\n    end note\n");
	const rvn_mm_node_t* n;
	CHECK(mm->kind == R_MERMAID_STATE);
	CHECK(mm->error == NULL);
	CHECK(mm->cluster_count == 1);
	CHECK(node(mm, "Still")->shape == RVN_MM_ROUND);
	CHECK(node(mm, "[*]start-1") && node(mm, "[*]start-1")->shape == RVN_MM_START);
	CHECK(node(mm, "[*]end-1") && node(mm, "[*]end-1")->shape == RVN_MM_END);
	/* the start and end inside the composite state are its own */
	CHECK(node(mm, "[*]start0") && node(mm, "[*]start0")->cluster == 0);
	CHECK(node(mm, "On")->cluster == 0);
	CHECK_STR(edge(mm, "Still", "Moving")->label, "push");
	CHECK_STR(node(mm, "Long")->label, "described");
	CHECK(node(mm, "pick")->shape == RVN_MM_RHOMBUS);
	CHECK(node(mm, "f")->shape == RVN_MM_BAR);
	/* a link to the composite state goes to its start */
	CHECK(node(mm, "Active") == NULL);
	CHECK(edge(mm, "Crash", "[*]start0") != NULL);
	n = node(mm, "note0");
	CHECK(n && n->shape == RVN_MM_NOTE);
	CHECK(n && strcmp(n->label, "very bad") == 0);
	n = node(mm, "note1");
	CHECK(n && strcmp(n->label, "two lines\nof note") == 0);
	rvn_mm_free(mm);

	CHECK(error_line("stateDiagram-v2\nstate A {\n  B --> C\n") > 0);
	CHECK(error_line("stateDiagram-v2\nA --> B\n}\n") == 3);
	CHECK(error_line("stateDiagram-v2\nA --> B\nA --> : x\n") == 3);
	CHECK(error_line("stateDiagram-v2\nnote right of A\n  never closed\n") > 0);
	CHECK(error_line("stateDiagram\nA --> B extra\n") == 2);
}

static void test_pie(void) {
	rvn_mm_t* mm = parse("pie showData title Pets adopted\n    \"Dogs\" : 386\n    \"Cats\" : 85.5\n    \"Rats\" : 15\n");
	CHECK(mm->kind == R_MERMAID_PIE);
	CHECK(mm->error == NULL);
	CHECK(mm->show_data);
	CHECK_STR(mm->title, "Pets adopted");
	CHECK(mm->slice_count == 3);
	CHECK(mm->slices[1].value == 85.5);
	CHECK_STR(mm->slices[2].label, "Rats");
	rvn_mm_free(mm);
	mm = parse_laid("pie\ntitle Key\n\"A\" : 1\n\"B\" : 3\n");
	{
		uint32_t i, wedges = 0;
		float total = 0.0f;
		for (i = 0; i < mm->item_count; i++) {
			if (mm->items[i].kind == RVN_MM_ITEM_WEDGE) {
				wedges++;
				total += mm->items[i].a1 - mm->items[i].a0;
			}
		}
		CHECK(wedges == 2);
		CHECK(fabsf(total - 6.2831853f) < 0.001f);
	}
	rvn_mm_free(mm);
	CHECK(error_line("pie\n\"A\" : lots\n") == 2);
	CHECK(error_line("pie\n\"A\" : -3\n") == 2);
	CHECK(error_line("pie\n\"A : 3\n") == 2);
}

/* ---- random texts ---- */

static void test_fuzz(void) {
	static const char* const pieces[] = {
		"flowchart TD\n", "graph LR\n", "sequenceDiagram\n", "stateDiagram-v2\n", "pie\n", "A", "B", "C", "-->", "---", "-.->", "==>",
		"--", "|", "[", "]", "(", ")", "{", "}", "{{", "((", "[(", "[/", "\\]", ">", "<", "&", ";", ":", ":::", "\"", "'", "subgraph ",
		"end\n", "\n", " ", "text", "classDef c fill:#f00\n", "class A c\n", "style A fill:#0f0\n", "%%", "%%{init: {'theme':'dark'}}%%\n",
		"participant ", "actor ", " as ", "->>", "-->>", "-x", "-)", "+", "-", "Note right of ", "Note over A,B", "loop ", "alt ",
		"else ", "par ", "and ", "opt ", "rect rgb(1,2,3)\n", "autonumber\n", "activate A\n", "deactivate A\n", "[*]", "state ",
		"<<choice>>", "<<fork>>", " {\n", "}\n", "note left of ", "end note\n", "title ", "showData", "\"x\" : 3\n", "#quot;", "<br>",
		"accDescr {", "accTitle: ", "direction LR\n", "\xC3\xA9", "\xFF", "\t", "1", "x", "o", "~~~", "=", "."
	};
	rvn_mm_measure_t m = fake_measure();
	double start = now_ms(), elapsed;
	int iteration, errors = 0, laid = 0;
	char text[2048];
	for (iteration = 0; iteration < 4000; iteration++) {
		size_t length = 0;
		int count = (int)(rnd() % 60u) + 1, i;
		rvn_mm_t* mm;
		if (iteration % 5 == 0) {
			/* raw bytes after a header */
			const char* header = pieces[rnd() % 5u];
			length = strlen(header);
			memcpy(text, header, length);
			for (i = 0; i < count * 4 && length + 1 < sizeof(text); i++) text[length++] = (char)(rnd() & 0xFFu);
		} else {
			for (i = 0; i < count; i++) {
				const char* piece = pieces[rnd() % RVN_COUNT_OF(pieces)];
				size_t n = strlen(piece);
				if (i == 0) piece = pieces[rnd() % 5u], n = strlen(piece);
				if (length + n + 1 >= sizeof(text)) break;
				memcpy(text + length, piece, n);
				length += n;
			}
		}
		mm = rvn_mm_parse(text, (int)length);
		CHECK(mm != NULL);
		if (!mm) continue;
		errors += mm->error != NULL;
		rvn_mm_layout(mm, &m);
		laid += mm->laid;
		CHECK(mm->width >= 0.0f && mm->height >= 0.0f && mm->width < 1e6f && mm->height < 1e6f);
		{
			rvn_text_t out = { NULL, 0, 0 };
			rvn_mm_summary(mm, &out);
			Rmemory_free(out.data);
		}
		rvn_mm_free(mm);
	}
	elapsed = now_ms() - start;
	printf("  4000 random texts read and laid out in %.0f ms (%d with an error)\n", elapsed, errors);
	CHECK(laid == 4000);
	CHECK(errors > 0 && errors < 4000);
	CHECK(elapsed < 20000.0);
}

/* ---- the layout of graphs ---- */

static int overlap(const rvn_mm_node_t* a, const rvn_mm_node_t* b) {
	return fabsf(a->x - b->x) * 2.0f < a->w + b->w - 0.5f && fabsf(a->y - b->y) * 2.0f < a->h + b->h - 0.5f;
}

/* The distance from a point to the box of a node (0 inside). */
static float box_distance(const rvn_mm_node_t* n, float x, float y) {
	float dx = fabsf(x - n->x) - n->w * 0.5f, dy = fabsf(y - n->y) - n->h * 0.5f;
	dx = dx > 0.0f ? dx : 0.0f;
	dy = dy > 0.0f ? dy : 0.0f;
	return sqrtf(dx * dx + dy * dy);
}

static void check_graph(const rvn_mm_t* mm) {
	uint32_t i, j;
	int overlaps = 0, loose = 0;
	for (i = 0; i < mm->node_count; i++) {
		for (j = i + 1; j < mm->node_count; j++) overlaps += overlap(&mm->nodes[i], &mm->nodes[j]);
		CHECK(mm->nodes[i].x - mm->nodes[i].w * 0.5f >= -0.01f && mm->nodes[i].x + mm->nodes[i].w * 0.5f <= mm->width + 0.01f);
		CHECK(mm->nodes[i].y - mm->nodes[i].h * 0.5f >= -0.01f && mm->nodes[i].y + mm->nodes[i].h * 0.5f <= mm->height + 0.01f);
	}
	CHECK(overlaps == 0);
	/* each path goes from the box of its first node to the box of its second */
	for (i = 0; i < mm->edge_count; i++) {
		const rvn_mm_edge_t* e = &mm->edges[i];
		const float* first;
		const float* last;
		if (e->point_count < 2) continue;
		first = &mm->points[e->first_point * 2u];
		last = &mm->points[(e->first_point + e->point_count - 1u) * 2u];
		if (box_distance(&mm->nodes[e->from], first[0], first[1]) > 1.0f) loose++;
		if (box_distance(&mm->nodes[e->to], last[0], last[1]) > 1.0f) loose++;
	}
	CHECK(loose == 0);
}

static void test_layout(void) {
	static const char* const directions[] = { "TD", "BT", "LR", "RL" };
	int d;
	for (d = 0; d < 4; d++) {
		char text[512];
		rvn_mm_t* mm;
		uint32_t i;
		int forward = 0, links = 0;
		snprintf(text, sizeof(text), "flowchart %s\nA[Start] --> B{Choice}\nB -->|one| C[First]\nB -->|two| D[Second]\nC --> E((End))\nD --> E\n"
			"A --> E\nE --> A\nC --> C\n", directions[d]);
		mm = parse_laid(text);
		CHECK(mm->error == NULL);
		check_graph(mm);
		/* the layers follow the direction (the link that closes the cycle goes back) */
		for (i = 0; i < mm->edge_count; i++) {
			const rvn_mm_node_t* a = &mm->nodes[mm->edges[i].from];
			const rvn_mm_node_t* b = &mm->nodes[mm->edges[i].to];
			if (a == b) continue;
			links++;
			if (d == 0) forward += b->y > a->y;
			else if (d == 1) forward += b->y < a->y;
			else if (d == 2) forward += b->x > a->x;
			else forward += b->x < a->x;
		}
		CHECK(forward == links - 1);
		CHECK(node(mm, "A")->layer < node(mm, "B")->layer && node(mm, "B")->layer < node(mm, "C")->layer);
		CHECK(node(mm, "C")->layer == node(mm, "D")->layer);
		rvn_mm_free(mm);
	}

	/* subgraphs hold their nodes, and no other */
	{
		rvn_mm_t* mm = parse_laid("flowchart TB\nx0 --> a1\nsubgraph S [Group]\n a1 --> a2 --> a3\n a1 --> a3\nend\nx0 --> x1 --> x2 --> x3\nx2 --> a2\n"
			"subgraph T\n b1 --> b2\nend\nx3 --> b1\na3 --> b2\n");
		uint32_t c, i;
		CHECK(mm->error == NULL);
		check_graph(mm);
		for (c = 0; c < mm->cluster_count; c++) {
			const rvn_mm_cluster_t* cl = &mm->clusters[c];
			for (i = 0; i < mm->node_count; i++) {
				const rvn_mm_node_t* n = &mm->nodes[i];
				int inside = n->x - n->w * 0.5f >= cl->x && n->x + n->w * 0.5f <= cl->x + cl->w && n->y - n->h * 0.5f >= cl->y &&
					n->y + n->h * 0.5f <= cl->y + cl->h;
				int touches = fabsf(n->x - (cl->x + cl->w * 0.5f)) * 2.0f < n->w + cl->w && fabsf(n->y - (cl->y + cl->h * 0.5f)) * 2.0f < n->h + cl->h;
				if (n->cluster == (int)c) CHECK(inside);
				else CHECK(!touches);
			}
		}
		rvn_mm_free(mm);
	}

	/* a state diagram: notes beside their states */
	{
		rvn_mm_t* mm = parse_laid("stateDiagram-v2\n[*] --> A\nA --> B: go\nB --> [*]\nnote right of A : beside\nstate C {\n [*] --> D\n D --> [*]\n}\nB --> C\n");
		const rvn_mm_node_t* a = node(mm, "A");
		const rvn_mm_node_t* n = node(mm, "note0");
		CHECK(mm->error == NULL);
		check_graph(mm);
		CHECK(a && n && fabsf(a->y - n->y) < 0.5f && n->x > a->x);
		rvn_mm_free(mm);
	}

	/* the same text, the same scene */
	{
		const char* text = "flowchart LR\nA --> B & C & D\nB --> E\nC --> E\nD --> F\nF --> A\nsubgraph G\nE --> H\nend\n";
		rvn_mm_t* a = parse_laid(text);
		rvn_mm_t* b = parse_laid(text);
		CHECK(a->item_count == b->item_count && a->point_count == b->point_count);
		CHECK(a->item_count == b->item_count && memcmp(a->items, b->items, sizeof(rvn_mm_item_t) * a->item_count) == 0);
		CHECK(a->point_count == b->point_count && memcmp(a->points, b->points, sizeof(float) * 2u * a->point_count) == 0);
		CHECK(a->width == b->width && a->height == b->height);
		rvn_mm_free(a);
		rvn_mm_free(b);
	}

	/* a sequence diagram: the participants in their order, the messages down the page */
	{
		rvn_mm_t* mm = parse_laid("sequenceDiagram\nparticipant A\nparticipant B\nparticipant C\nA->>B: one\nB->>C: a much longer message here\nC-->>A: back\n");
		uint32_t i;
		float last_y = -1.0f;
		int down = 1;
		CHECK(mm->nodes[0].x < mm->nodes[1].x && mm->nodes[1].x < mm->nodes[2].x);
		/* room for the long message between B and C */
		CHECK(mm->nodes[2].x - mm->nodes[1].x >= 27.0f * 14.0f * 0.55f);
		for (i = 0; i < mm->item_count; i++) {
			const rvn_mm_item_t* item = &mm->items[i];
			if (item->kind == RVN_MM_ITEM_PATH && item->head_end != RVN_MM_HEAD_NONE) {
				float y = mm->points[item->first_point * 2u + 1u];
				if (y <= last_y) down = 0;
				last_y = y;
			}
		}
		CHECK(down);
		check_graph(mm);
		rvn_mm_free(mm);
	}

	/* an error is a box that tells it */
	{
		rvn_mm_t* mm = parse_laid("flowchart TD\nA -->\n");
		CHECK(mm->item_count == 1 && mm->items[0].shape == RVN_MM_ERROR);
		CHECK(strstr(rvn_mm_item_text(mm, &mm->items[0]), "Line 2") != NULL);
		CHECK(mm->width > 0.0f && mm->height > 0.0f);
		rvn_mm_free(mm);
	}
}

/* A random graph: `n` nodes, about 1.5 links each, mostly forward, some back. */
static char* random_graph(int n, int clusters) {
	size_t cap = (size_t)n * 64 + 256, length = 0;
	char* text = (char*)malloc(cap);
	int i;
	length += (size_t)snprintf(text + length, cap - length, "flowchart TD\n");
	for (i = 1; i < n; i++) {
		int from = (int)(rnd() % (uint32_t)i);
		length += (size_t)snprintf(text + length, cap - length, "n%d --> n%d\n", from, i);
		if (rnd() % 2u == 0) {
			int to = (int)(rnd() % (uint32_t)n);
			length += (size_t)snprintf(text + length, cap - length, "n%d -->|t%d| n%d\n", i, i, to);
		}
	}
	for (i = 0; i < clusters; i++) {
		int a = (int)(rnd() % (uint32_t)n);
		length += (size_t)snprintf(text + length, cap - length, "subgraph g%d\nn%d\nn%d\nend\n", i, a, (a + 1) % n);
	}
	return text;
}

/* Random graphs of every size, direction and with subgraphs: never two nodes over each other, every path on its nodes. */
static void test_random_graphs(void) {
	static const char* const directions[] = { "TD", "BT", "LR", "RL" };
	int iteration;
	for (iteration = 0; iteration < 400; iteration++) {
		char* text = random_graph((int)(rnd() % 40u) + 1, (int)(rnd() % 4u));
		char* header = strchr(text, '\n');
		rvn_mm_t* mm;
		/* "flowchart TD" -> another direction */
		memcpy(header - 2, directions[rnd() % 4u], 2);
		mm = parse_laid(text);
		CHECK(mm->error == NULL);
		check_graph(mm);
		rvn_mm_free(mm);
		free(text);
	}
}

static void test_performance(void) {
	rvn_mm_measure_t m = fake_measure();
	char* text = random_graph(200, 0);
	rvn_mm_t* mm = rvn_mm_parse(text, -1);
	double start, best = 1e9;
	int round;
	CHECK(mm->error == NULL);
	CHECK(mm->node_count == 200);
	for (round = 0; round < 3; round++) {
		start = now_ms();
		rvn_mm_layout(mm, &m);
		best = rvn_minf((float)best, (float)(now_ms() - start));
	}
	printf("  200 nodes and %u links laid out in %.1f ms\n", mm->edge_count, best);
	check_graph(mm);
	/* (the bound of the request is 50 ms in an optimized build; this one may run with sanitizers) */
	CHECK(best < 250.0);
	rvn_mm_free(mm);
	free(text);

	/* subgraphs too, and a graph at the limits stays bounded */
	text = random_graph(300, 12);
	mm = rvn_mm_parse(text, -1);
	CHECK(mm->error == NULL);
	start = now_ms();
	rvn_mm_layout(mm, &m);
	printf("  300 nodes with 12 subgraphs in %.1f ms\n", now_ms() - start);
	{
		uint32_t i, j;
		int overlaps = 0;
		for (i = 0; i < mm->node_count; i++)
			for (j = i + 1; j < mm->node_count; j++) overlaps += overlap(&mm->nodes[i], &mm->nodes[j]);
		CHECK(overlaps == 0);
	}
	rvn_mm_free(mm);
	free(text);

	text = random_graph(1500, 0);
	mm = rvn_mm_parse(text, -1);
	start = now_ms();
	rvn_mm_layout(mm, &m);
	printf("  1500 nodes in %.1f ms\n", now_ms() - start);
	CHECK(now_ms() - start < 10000.0);
	rvn_mm_free(mm);
	free(text);
}

/* ---- SVG ---- */

static void test_svg(void) {
	rvn_mm_t* mm = parse_laid("flowchart LR\nA[\"a < b & c\"] -->|go| B((B))\nB -.-> C{C}\n");
	rvn_color_t colors[RVN_MM_C_COUNT_];
	rvn_text_t out = { NULL, 0, 0 };
	CHECK(rvn_mm_theme_colors("default", colors));
	CHECK(!rvn_mm_theme_colors("unknown", colors));
	rvn_mm_svg(mm, colors, "sans-serif", &out);
	CHECK(out.data && strncmp(out.data, "<svg", 4) == 0);
	CHECK(out.data && strstr(out.data, "a &lt; b &amp; c") != NULL);
	CHECK(out.data && strstr(out.data, "<circle") != NULL);
	CHECK(out.data && strstr(out.data, "<polygon") != NULL);
	CHECK(out.data && strstr(out.data, "stroke-dasharray") != NULL);
	CHECK(out.data && strstr(out.data, "</svg>") != NULL);
	Rmemory_free(out.data);
	rvn_mm_free(mm);
}

/* ---- the widget ---- */

static void show(Rid widget) {
	Rwindow_set_content(g_window, widget);
	Rdraw_frame();
}

static void test_view(void) {
	Rid view = Rmermaid_view_create("flowchart LR\nA[Start] --> B{Ready?}\nB -->|yes| C([Go])\nB -->|no| A\n");
	const char* id = NULL;
	const char* label = NULL;
	float x, y, w, h, dw, dh;
	int from = -1, to = -1, line = -1;
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rid narrow;
	Rwidget_add_child(box, view);
	show(box);
	CHECK(Rwidget_get_type(view) == R_WIDGET_MERMAID_VIEW);
	CHECK(Rmermaid_view_get_kind(view) == R_MERMAID_FLOWCHART);
	CHECK(Rmermaid_view_get_error_line(view) == 0);
	CHECK_STR(Rmermaid_view_get_error(view), "");
	CHECK(Rmermaid_view_get_node_count(view) == 3);
	CHECK(Rmermaid_view_get_edge_count(view) == 3);
	CHECK(Rmermaid_view_get_node(view, 1, &id, &label, &x, &y, &w, &h));
	CHECK_STR(id, "B");
	CHECK_STR(label, "Ready?");
	CHECK(w > 0.0f && h > 0.0f);
	CHECK(Rmermaid_view_get_edge(view, 1, &from, &to, &label));
	CHECK(from == 1 && to == 2);
	CHECK_STR(label, "yes");
	CHECK(!Rmermaid_view_get_node(view, 3, NULL, NULL, NULL, NULL, NULL, NULL));
	CHECK_STR(Rmermaid_view_get_summary(view), "flowchart with 3 nodes and 3 links");
	/* accessibility: an image named by its summary, described by its text */
	CHECK(Rwidget_get_accessible_role(view) == R_ROLE_IMAGE);
	CHECK_STR(Rwidget_get_accessible_name(view), "flowchart with 3 nodes and 3 links");

	/* fit: scaled down to the width, never up */
	Rmermaid_view_get_diagram_size(view, &dw, &dh);
	CHECK(dw > 0.0f && dh > 0.0f);
	Rwidget_get_rect(view, &x, &y, &w, &h);
	CHECK(Rmermaid_view_get_scale(view) == 1.0f || dw > w);
	{
		narrow = Rmermaid_view_create(Rmermaid_view_get_source(view));
		Rwidget_set_size(narrow, dw * 0.5f, -1.0f);
		Rwidget_add_child(box, narrow);
		Rslot_set_halign(narrow, R_HALIGN_LEFT);
		Rdraw_frame();
		Rdraw_frame();
		Rwidget_get_rect(narrow, &x, &y, &w, &h);
		CHECK_NEAR(Rmermaid_view_get_scale(narrow), 0.5f);
		CHECK(fabsf(h - dh * 0.5f) <= 1.5f);
		Rmermaid_view_set_fit(narrow, R_MERMAID_FIT_NONE);
		Rmermaid_view_set_zoom(narrow, 2.0f);
		Rdraw_frame();
		CHECK(Rmermaid_view_get_fit(narrow) == R_MERMAID_FIT_NONE);
		CHECK(Rmermaid_view_get_zoom(narrow) == 2.0f);
		CHECK(Rmermaid_view_get_scale(narrow) == 2.0f);
		Rwidget_get_rect(narrow, &x, &y, &w, &h);
		CHECK(fabsf(h - dh * 2.0f) <= 1.5f);
		Rmermaid_view_set_zoom(narrow, 100.0f);
		CHECK(Rmermaid_view_get_zoom(narrow) == 8.0f);
		Rwidget_remove_child(box, narrow);
		Rwidget_destroy(narrow);
	}

	/* an error: its line, and the box that tells it */
	Rmermaid_view_set_source(view, "sequenceDiagram\nA->>B: hi\nelse what\n");
	Rdraw_frame();
	CHECK(Rmermaid_view_get_kind(view) == R_MERMAID_NONE);
	CHECK(Rmermaid_view_get_error_line(view) == 3);
	CHECK(strstr(Rmermaid_view_get_error(view), "else") != NULL);
	Rmermaid_view_get_diagram_size(view, &dw, &dh);
	CHECK(dw > 0.0f && dh > 0.0f);

	/* the other kinds */
	Rmermaid_view_set_source(view, "sequenceDiagram\nA->>B: hi\nB-->>A: hello\n");
	CHECK(Rmermaid_view_get_kind(view) == R_MERMAID_SEQUENCE);
	CHECK(Rmermaid_view_get_node_count(view) == 2);
	CHECK(Rmermaid_view_get_edge_count(view) == 2);
	CHECK(Rmermaid_view_get_edge(view, 1, &from, &to, &label) && from == 1 && to == 0);
	CHECK_STR(label, "hello");
	CHECK_STR(Rmermaid_view_get_summary(view), "sequence diagram with 2 participants and 2 messages");
	Rmermaid_view_set_source(view, "pie title Pets\n\"Dogs\" : 3\n\"Cats\" : 1\n");
	CHECK(Rmermaid_view_get_kind(view) == R_MERMAID_PIE);
	CHECK(Rmermaid_view_get_node_count(view) == 2);
	CHECK_STR(Rmermaid_view_get_summary(view), "Pets, pie chart with 2 slices: Dogs 75%, Cats 25%");
	Rmermaid_view_set_source(view, "stateDiagram-v2\n[*] --> A\nA --> [*]\n");
	CHECK(Rmermaid_view_get_kind(view) == R_MERMAID_STATE);
	CHECK_STR(Rmermaid_view_get_summary(view), "state diagram with 1 state and 2 transitions");
	Rdraw_frame();

	/* SVG */
	{
		size_t size = 0;
		char* svg = Rmermaid_view_to_svg(view, &size);
		const char* path = "test_mermaid_out.svg";
		FILE* file;
		CHECK(svg && strstr(svg, "<svg") != NULL);
		CHECK(svg && size == strlen(svg));
		Rmemory_free(svg);
		CHECK(Rmermaid_view_save_svg(view, path));
		file = fopen(path, "rb");
		CHECK(file != NULL);
		if (file) {
			char head[5] = { 0 };
			CHECK(fread(head, 1, 4, file) == 4);
			CHECK(strcmp(head, "<svg") == 0);
			fclose(file);
		}
		remove(path);
		/* (the null backend has no pixels to capture) */
		Rmermaid_view_save_png(view, "test_mermaid_out.png");
		remove("test_mermaid_out.png");
	}

	/* checked without a widget */
	CHECK(Rmermaid_check("flowchart TD\nA-->B\n", &line) == R_MERMAID_FLOWCHART);
	CHECK(line == 0);
	CHECK(Rmermaid_check_message("flowchart TD\nA-->B\n", NULL) == NULL);
	CHECK(Rmermaid_check("flowchart TD\nA-->B\nC -->\n", &line) == R_MERMAID_NONE);
	CHECK(line == 3);
	{
		size_t size = 0;
		char* message = Rmermaid_check_message("flowchart TD\nA-->B\nC -->\n", &size);
		CHECK(message && size == strlen(message) && size > 0);
		Rmemory_free(message);
	}
	CHECK(Rmermaid_check(NULL, NULL) == R_MERMAID_NONE);
	Rwindow_set_content(g_window, R_INVALID_ID);
}

static void test_xml(void) {
	static const char* xml =
		"<VBox>"
		"<MermaidView Name=\"a\" Fit=\"None\" Zoom=\"1.5\"><![CDATA[\n"
		"  flowchart TD\n"
		"    A & B --> C\n"
		"]]></MermaidView>"
		"<Mermaid Name=\"b\" Text=\"pie&#10;&quot;x&quot; : 1\"/>"
		"<Label Name=\"md\" Markdown=\"true\" Mermaid=\"false\" Text=\"```mermaid&#10;graph TD&#10;A-->B&#10;```\"/>"
		"</VBox>";
	Rid root = Rwidget_create_from_xml(xml, strlen(xml));
	Rid a, b, md;
	CHECK(root != R_INVALID_ID);
	show(root);
	a = Rwidget_find(root, "a");
	b = Rwidget_find(root, "b");
	md = Rwidget_find(root, "md");
	CHECK(Rmermaid_view_get_kind(a) == R_MERMAID_FLOWCHART);
	CHECK(Rmermaid_view_get_edge_count(a) == 2);
	CHECK(Rmermaid_view_get_fit(a) == R_MERMAID_FIT_NONE);
	CHECK(Rmermaid_view_get_zoom(a) == 1.5f);
	CHECK(Rmermaid_view_get_kind(b) == R_MERMAID_PIE);
	CHECK(!Rlabel_get_mermaid(md));
	CHECK(rvn_label_diagram_count(rvn_widget_get(md)) == 0);
	Rwindow_set_content(g_window, R_INVALID_ID);
}

static void test_markdown(void) {
	static const char* text =
		"# Notes\n\nBefore.\n\n```mermaid\nflowchart LR\n  A --> B --> C\n```\n\nBetween.\n\n"
		"```mermaid\nsequenceDiagram\n  A->>B: hi\n```\n\n```mermaid\nflowchart TD\n  A -->\n```\n\n```c\nint x;\n```\n";
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rid label = Rlabel_create(text);
	float x, y, w, h, h_code;
	Rlabel_set_markdown(label, R_TRUE);
	Rslot_set_valign(label, R_VALIGN_TOP);
	Rwidget_add_child(box, label);
	show(box);
	CHECK(Rlabel_get_mermaid(label));
	/* two diagrams and the box of an error; the C stays code */
	CHECK(rvn_label_diagram_count(rvn_widget_get(label)) == 3);
	CHECK(strstr(Rlabel_get_plain_text(label), "flowchart with 3 nodes and 2 links\nflowchart LR") != NULL);
	CHECK(strstr(Rlabel_get_plain_text(label), "int x;") != NULL);
	Rwidget_get_rect(label, &x, &y, &w, &h);

	/* turned off: code */
	Rlabel_set_mermaid(label, R_FALSE);
	Rdraw_frame();
	CHECK(!Rlabel_get_mermaid(label));
	CHECK(rvn_label_diagram_count(rvn_widget_get(label)) == 0);
	CHECK(strstr(Rlabel_get_plain_text(label), "flowchart with") == NULL);
	Rwidget_get_rect(label, &x, &y, &w, &h_code);
	CHECK(h_code != h);
	Rlabel_set_mermaid(label, R_TRUE);
	Rdraw_frame();
	CHECK(rvn_label_diagram_count(rvn_widget_get(label)) == 3);

	/* a diagram wider than the label is scaled down to it */
	Rlabel_set_text(label, "```mermaid\nflowchart LR\nA[a long first node] --> B[a long second node] --> C[a long third node] --> D[a long fourth node] --> E[and a fifth]\n```\n");
	Rwidget_set_size(label, 300.0f, -1.0f);
	Rslot_set_halign(label, R_HALIGN_LEFT);
	Rdraw_frame();
	Rdraw_frame();
	Rwidget_get_rect(label, &x, &y, &w, &h);
	CHECK(w <= 300.5f);
	CHECK(h > 10.0f && h < 200.0f);
	Rwindow_set_content(g_window, R_INVALID_ID);
}

static void test_theme(void) {
	Rid view = Rmermaid_view_create("flowchart TD\nA --> B\n");
	rvn_color_t dark[RVN_MM_C_COUNT_], light[RVN_MM_C_COUNT_], forest[RVN_MM_C_COUNT_];
	show(view);
	Rlook_set(R_LOOK_RAVEN_DARK);
	Rdraw_frame();
	rvn_mermaid_colors(rvn_widget_get(view), NULL, dark);
	Rlook_set(R_LOOK_RAVEN_LIGHT);
	Rdraw_frame();
	rvn_mermaid_colors(rvn_widget_get(view), NULL, light);
	/* the colors follow the theme: the text, the nodes, the background */
	CHECK(dark[RVN_MM_C_TEXT] != light[RVN_MM_C_TEXT]);
	CHECK(dark[RVN_MM_C_NODE] != light[RVN_MM_C_NODE]);
	CHECK(dark[RVN_MM_C_BACKGROUND] != light[RVN_MM_C_BACKGROUND]);
	/* a theme of Mermaid keeps its own colors */
	Rmermaid_view_set_source(view, "%%{init: {\"theme\": \"forest\"}}%%\nflowchart TD\nA --> B\n");
	Rdraw_frame();
	{
		rvn_mm_t* mm = rvn_mm_parse("%%{init: {\"theme\": \"forest\"}}%%\nflowchart TD\nA --> B\n", -1);
		rvn_mermaid_colors(rvn_widget_get(view), mm, forest);
		CHECK(forest[RVN_MM_C_NODE] == rvn_hex(0xCDE498, 255));
		rvn_mm_free(mm);
	}
	Rlook_set(R_LOOK_RAVEN_DARK);
	Rdraw_frame();
	Rwindow_set_content(g_window, R_INVALID_ID);
}

int main(void) {
	RUN(test_flowchart);
	RUN(test_sequence);
	RUN(test_state);
	RUN(test_pie);
	RUN(test_fuzz);
	RUN(test_layout);
	RUN(test_random_graphs);
	RUN(test_performance);
	RUN(test_svg);
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("mermaid", 800, 600, R_WINDOW_DEFAULT);
	RUN(test_view);
	RUN(test_xml);
	RUN(test_markdown);
	RUN(test_theme);
	Rdraw_frame();
	Rshutdown();
	return test_report("test_mermaid");
}
