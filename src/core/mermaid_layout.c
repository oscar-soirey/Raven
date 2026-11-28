/**
 * Raven - core/mermaid_layout.c
 * The layouts of the Mermaid diagrams (see core/mermaid.h): the model is
 * given places, and the scene (shapes, paths, texts) is made from them.
 *
 * Graphs (flowcharts, state diagrams) are laid out in layers, in the manner
 * of Sugiyama, top to bottom; the other directions are the same layout
 * turned. The steps:
 *   1. cycles broken: a depth-first search turns back the links that close
 *      a cycle (they are drawn the right way round in the end);
 *   2. layers: the longest path from the sources, then the sources moved
 *      down next to what they lead to; a graph with texts on its links has
 *      every link twice as long, and the text takes the place in the middle;
 *   3. links longer than one layer go through points of their own (dummy
 *      nodes), one per layer they cross;
 *   4. order in each layer: sweeps down and up that sort the nodes by the
 *      mean place of their neighbours (barycenters), keeping the order with
 *      the fewest crossings (counted with a Fenwick tree); the nodes of a
 *      subgraph are kept next to each other, the subgraph sorted as one;
 *   5. places across: each layer in turn pulls its nodes toward their
 *      neighbours (a weighted mean that the long links pull hardest, so they
 *      come out straight), and the nearest places that keep the order and
 *      the gaps are found exactly (isotonic regression, pool adjacent
 *      violators); then the nodes that would fall inside a subgraph they are
 *      not in are pushed out;
 *   6. places along: each layer as thick as its thickest node, the gaps
 *      wider where subgraphs begin and end;
 *   7. the links: smooth curves through their points, leaving and reaching
 *      the nodes along the flow.
 * Everything is in a fixed order: the same text always gives the same
 * picture. A text of any size is laid out in a bounded time: the points of
 * the long links are capped, the sweeps fewer on very large graphs.
 *
 * Sequence diagrams are laid out in columns (one per participant, as far
 * apart as their messages need) and rows (one per message, note, block);
 * pies are a disc and a legend.
 */

#include "mermaid.h"

/* ---------------------------------------------------------------------------
 *  Texts
 * ------------------------------------------------------------------------- */

typedef struct rvn_mm_block_t {
	float w, h;
	int lines;
} rvn_mm_block_t;

static float rvn_mm_width(const rvn_mm_measure_t* m, const char* text, int length, int bold) {
	if (length <= 0 || !m->width) return 0.0f;
	return m->width(m->user, text, length, m->size, bold);
}

/*
 * A label laid out as a block of lines: its own line breaks, and the words
 * wrapped at `max_width` (0: never). The lines are written in `out`
 * separated by '\n'.
 */
static rvn_mm_block_t rvn_mm_text_block(const rvn_mm_measure_t* m, const char* label, float max_width, int bold, float scale, rvn_text_t* out) {
	rvn_mm_block_t block = { 0.0f, 0.0f, 0 };
	const char* p = label ? label : "";
	const char* end = p + strlen(p);
	float space = rvn_mm_width(m, " ", 1, bold) * scale;
	out->length = 0;
	if (out->data) out->data[0] = '\0';
	if (p == end) {
		rvn_text_append(out, "", 0);
		return block;
	}
	while (p <= end) {
		const char* line_end = memchr(p, '\n', (size_t)(end - p));
		const char* word = p;
		float line_width = 0.0f;
		int words_on_line = 0;
		if (!line_end) line_end = end;
		if (block.lines > 0) rvn_text_append(out, "\n", 1);
		block.lines++;
		while (word < line_end) {
			const char* word_end = word;
			float width;
			while (word < line_end && *word == ' ') word++;
			if (word >= line_end) break;
			word_end = word;
			while (word_end < line_end && *word_end != ' ') word_end++;
			width = rvn_mm_width(m, word, (int)(word_end - word), bold) * scale;
			if (words_on_line > 0 && max_width > 0.0f && line_width + space + width > max_width) {
				rvn_text_append(out, "\n", 1);
				block.w = rvn_maxf(block.w, line_width);
				block.lines++;
				line_width = 0.0f;
				words_on_line = 0;
			}
			if (words_on_line > 0) {
				rvn_text_append(out, " ", 1);
				line_width += space;
			}
			rvn_text_append(out, word, (size_t)(word_end - word));
			line_width += width;
			words_on_line++;
			word = word_end;
		}
		block.w = rvn_maxf(block.w, line_width);
		if (line_end >= end) break;
		p = line_end + 1;
	}
	if (!out->data) rvn_text_append(out, "", 0);
	block.h = m->line_height * scale * (float)block.lines;
	return block;
}

/* A text item: lines placed from (x, y) in a width, by `align`. */
static rvn_mm_item_t* rvn_mm_add_text(rvn_mm_t* mm, const char* text, float x, float y, float w, float h, float align, uint8_t color) {
	rvn_mm_item_t* item = rvn_mm_add_item(mm, RVN_MM_ITEM_TEXT);
	item->x = x;
	item->y = y;
	item->w = w;
	item->h = h;
	item->align = align;
	item->text = color;
	rvn_mm_item_set_text(mm, item, text, -1);
	return item;
}

static rvn_mm_item_t* rvn_mm_add_shape(rvn_mm_t* mm, int shape, float x, float y, float w, float h, uint8_t fill, uint8_t stroke) {
	rvn_mm_item_t* item = rvn_mm_add_item(mm, RVN_MM_ITEM_SHAPE);
	item->shape = (uint8_t)shape;
	item->x = x;
	item->y = y;
	item->w = w;
	item->h = h;
	item->fill = fill;
	item->stroke = stroke;
	return item;
}

static rvn_mm_item_t* rvn_mm_add_line(rvn_mm_t* mm, float x0, float y0, float x1, float y1, uint8_t line, uint8_t color) {
	rvn_mm_item_t* item = rvn_mm_add_item(mm, RVN_MM_ITEM_PATH);
	item->first_point = rvn_mm_add_point(mm, x0, y0);
	rvn_mm_add_point(mm, x1, y1);
	item->point_count = 2;
	item->line = line;
	item->stroke = color;
	return item;
}

/* The colors of a node from its style: written ones, else the slots. */
static void rvn_mm_apply_style(rvn_mm_item_t* item, const rvn_mm_style_t* style) {
	if (rvn_color_a(style->fill)) {
		item->fill = RVN_MM_C_EXPLICIT;
		item->fill_color = style->fill;
	}
	if (rvn_color_a(style->stroke)) {
		item->stroke = RVN_MM_C_EXPLICIT;
		item->stroke_color = style->stroke;
	}
	if (rvn_color_a(style->color)) {
		item->text = RVN_MM_C_EXPLICIT;
		item->text_color = style->color;
	}
	if (style->stroke_width > 0.0f) item->stroke_width = style->stroke_width;
	if (style->dashed) item->line = RVN_MM_LINE_DOTTED;
}

/* ---------------------------------------------------------------------------
 *  Paths
 * ------------------------------------------------------------------------- */

/* A cubic curve from (x0, y0) to (x3, y3) added to the points (its first point left out). */
static void rvn_mm_add_cubic(rvn_mm_t* mm, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3) {
	int steps, i;
	float span = fabsf(x3 - x0) + fabsf(y3 - y0);
	/* straight: one segment */
	if (fabsf(x1 - x0) + fabsf(x2 - x3) + fabsf(x3 - x0) < 0.5f || fabsf(y1 - y0) + fabsf(y2 - y3) + fabsf(y3 - y0) < 0.5f) {
		rvn_mm_add_point(mm, x3, y3);
		return;
	}
	steps = rvn_clampi((int)(span / 6.0f), 4, 16);
	for (i = 1; i <= steps; i++) {
		float t = (float)i / (float)steps, u = 1.0f - t;
		float a = u * u * u, b = 3.0f * u * u * t, c = 3.0f * u * t * t, d = t * t * t;
		rvn_mm_add_point(mm, a * x0 + b * x1 + c * x2 + d * x3, a * y0 + b * y1 + c * y2 + d * y3);
	}
}

/* ---------------------------------------------------------------------------
 *  Shapes
 * ------------------------------------------------------------------------- */

/* The size of a shape around a block of text. */
static void rvn_mm_shape_size(int shape, float tw, float th, float k, float* w, float* h) {
	float px = 15.0f * k, py = 9.0f * k;
	switch (shape) {
		case RVN_MM_CIRCLE:
		case RVN_MM_DOUBLE_CIRCLE: {
			float d = rvn_maxf(sqrtf(tw * tw + th * th) + 12.0f * k, 30.0f * k);
			if (shape == RVN_MM_DOUBLE_CIRCLE) d += 10.0f * k;
			*w = *h = d;
			return;
		}
		case RVN_MM_RHOMBUS: {
			/* a diamond that holds the text: half diagonals a across, b down, with tw/2a + th/2b <= 1 */
			float tx = tw * 0.5f + 6.0f * k, ty = th * 0.5f + 4.0f * k;
			float a = tx + ty * 1.6f, b = a / 1.6f;
			b = rvn_maxf(b, ty * a / rvn_maxf(a - tx, 1.0f));
			*w = a * 2.0f;
			*h = b * 2.0f;
			return;
		}
		case RVN_MM_HEXAGON:
			*h = th + py * 2.0f;
			*w = tw + px * 2.0f + *h * 0.5f;
			return;
		case RVN_MM_PARALLELOGRAM:
		case RVN_MM_PARALLELOGRAM_ALT:
		case RVN_MM_TRAPEZOID:
		case RVN_MM_TRAPEZOID_ALT:
			*h = th + py * 2.0f;
			*w = tw + px * 2.0f + *h * 0.6f;
			return;
		case RVN_MM_ASYMMETRIC:
			*h = th + py * 2.0f;
			*w = tw + px * 2.0f + *h * 0.35f;
			return;
		case RVN_MM_STADIUM:
			*h = th + py * 2.0f;
			*w = tw + px * 2.0f + *h * 0.3f;
			return;
		case RVN_MM_SUBROUTINE:
			*h = th + py * 2.0f;
			*w = tw + px * 2.0f + 14.0f * k;
			return;
		case RVN_MM_CYLINDER: {
			float rx = (tw + px * 2.0f) * 0.5f, ry = rx * 0.1f + 3.0f * k;
			*w = rx * 2.0f;
			*h = th + py * 2.0f + ry * 2.0f;
			return;
		}
		case RVN_MM_START:
			*w = *h = 14.0f * k;
			return;
		case RVN_MM_END:
			*w = *h = 16.0f * k;
			return;
		case RVN_MM_BAR:
			*w = 70.0f * k;
			*h = 7.0f * k;
			return;
		case RVN_MM_NOTE:
			*w = tw + 20.0f * k;
			*h = th + 14.0f * k;
			return;
		default:
			*w = tw + px * 2.0f;
			*h = th + py * 2.0f;
			if (shape == RVN_MM_ROUND) *w += 4.0f * k;
			return;
	}
}

/* ---------------------------------------------------------------------------
 *  Graphs: the layered layout
 * ------------------------------------------------------------------------- */

typedef struct rvn_gl_node_t {
	/* size across the layers and along the flow (w and h when going down) */
	float cross, along;
	/* room kept on both sides (a link that loops back to its node) */
	float extra;
	int layer;
	float pos;
	float bary;
	int order;
	/* a node of the model, or -1 for a point of a link */
	int real;
	/* the cluster it is in, -1 for none */
	int cluster;
	/* a note beside a state: the state, and the side (-1 left, +1 right) */
	int host;
	int side;
	uint8_t label;
} rvn_gl_node_t;

/* A link between two adjacent layers: `up` in the layer above `down`. */
typedef struct rvn_gl_edge_t {
	int up, down;
	float weight;
} rvn_gl_edge_t;

typedef struct rvn_gl_t {
	rvn_mm_t* mm;
	float k;
	rvn_gl_node_t* nodes;
	uint32_t count, cap;
	rvn_gl_edge_t* edges;
	uint32_t edge_count, edge_cap;
	/* the chain of nodes of each link of the model (from its upper end), in `chain` */
	uint32_t* chain_start;
	uint32_t* chain_length;
	uint32_t* chain;
	uint32_t chain_count, chain_cap;
	uint8_t* reversed;
	/* the layers: their nodes in order */
	int layer_count;
	uint32_t* layer_start;
	uint32_t* layer_nodes;
	/* neighbours above and below each node (indices of nodes), with weights */
	uint32_t* up_start;
	uint32_t* down_start;
	int* up_list;
	int* down_list;
	float* up_weight;
	float* down_weight;
	/* the clusters: the chain of clusters of each cluster from the outermost, depth */
	int* cluster_depth;
	int* cluster_chain;
	float* cluster_sum;
	int* cluster_hits;
	/* sort scratch */
	int* scratch;
	float node_gap, dummy_gap, rank_gap, cluster_pad, title_h;
} rvn_gl_t;

static int rvn_gl_add_node(rvn_gl_t* g) {
	rvn_gl_node_t* n;
	RVN_ARRAY_RESERVE(g->nodes, g->cap, g->count + 1);
	n = &g->nodes[g->count];
	memset(n, 0, sizeof(*n));
	n->real = -1;
	n->cluster = -1;
	n->host = -1;
	return (int)g->count++;
}

static void rvn_gl_add_edge(rvn_gl_t* g, int up, int down, float weight) {
	RVN_ARRAY_RESERVE(g->edges, g->edge_cap, g->edge_count + 1);
	g->edges[g->edge_count].up = up;
	g->edges[g->edge_count].down = down;
	g->edges[g->edge_count].weight = weight;
	g->edge_count++;
}

/* The innermost cluster that holds both (-1: none). */
static int rvn_gl_common_cluster(const rvn_mm_t* mm, int a, int b) {
	int guard = 0;
	while (a >= 0 && guard++ < 200) {
		int c = b, inner = 0;
		while (c >= 0 && inner++ < 200) {
			if (c == a) return a;
			c = mm->clusters[c].parent;
		}
		a = mm->clusters[a].parent;
	}
	return -1;
}

static int rvn_gl_depth(const rvn_gl_t* g, int cluster) {
	return cluster >= 0 ? g->cluster_depth[cluster] : 0;
}

/* The cluster at `level` (0: outermost) of the chain of a node's cluster, or -1 past its depth. */
static int rvn_gl_chain_at(const rvn_gl_t* g, int cluster, int level) {
	if (cluster < 0 || level >= g->cluster_depth[cluster]) return -1;
	return g->cluster_chain[cluster * 64 + level];
}

/*
 * Order of two nodes of a layer: the groups they are in compared at the
 * first level where they differ (a cluster by the mean barycenter of its
 * nodes in the layer, a node by its own), so a cluster stays together.
 */
static int rvn_gl_before(const rvn_gl_t* g, int a, int b) {
	const rvn_gl_node_t* na = &g->nodes[a];
	const rvn_gl_node_t* nb = &g->nodes[b];
	int level = 0, ga, gb;
	float ka, kb;
	if (na->cluster != nb->cluster) {
		while (rvn_gl_chain_at(g, na->cluster, level) >= 0 && rvn_gl_chain_at(g, na->cluster, level) == rvn_gl_chain_at(g, nb->cluster, level)) level++;
		ga = rvn_gl_chain_at(g, na->cluster, level);
		gb = rvn_gl_chain_at(g, nb->cluster, level);
	} else {
		ga = gb = -1;
	}
	ka = ga >= 0 ? g->cluster_sum[ga] / (float)rvn_maxi(g->cluster_hits[ga], 1) : na->bary;
	kb = gb >= 0 ? g->cluster_sum[gb] / (float)rvn_maxi(g->cluster_hits[gb], 1) : nb->bary;
	if (ka != kb) return ka < kb;
	/* (a cluster before a node at the same place, then by index: a total order) */
	if ((ga >= 0) != (gb >= 0)) return ga >= 0;
	if (ga >= 0) return ga < gb;
	return a < b;
}

/* Sorts the nodes of a layer (a stable merge sort). */
static void rvn_gl_sort(rvn_gl_t* g, uint32_t* items, uint32_t count) {
	uint32_t width, i;
	uint32_t* a = items;
	uint32_t* b = (uint32_t*)g->scratch;
	if (count < 2) return;
	/* insertion sort on small runs, then merges */
	for (i = 0; i < count; i += 8) {
		uint32_t stop = rvn_minu(i + 8, count), j;
		for (j = i + 1; j < stop; j++) {
			uint32_t v = a[j], k = j;
			while (k > i && rvn_gl_before(g, (int)v, (int)a[k - 1])) {
				a[k] = a[k - 1];
				k--;
			}
			a[k] = v;
		}
	}
	for (width = 8; width < count; width *= 2) {
		for (i = 0; i < count; i += width * 2) {
			uint32_t mid = rvn_minu(i + width, count), stop = rvn_minu(i + width * 2, count);
			uint32_t l = i, r = mid, o = i;
			while (l < mid && r < stop) b[o++] = rvn_gl_before(g, (int)a[r], (int)a[l]) ? a[r++] : a[l++];
			while (l < mid) b[o++] = a[l++];
			while (r < stop) b[o++] = a[r++];
		}
		{
			uint32_t* t = a;
			a = b;
			b = t;
		}
	}
	if (a != items) memcpy(items, a, sizeof(uint32_t) * count);
}

/* Crossings between a layer and the next (the links sorted by their upper end, inversions of the lower ends). */
static long rvn_gl_crossings(rvn_gl_t* g, int layer, int* tree, int* pairs) {
	uint32_t from = g->layer_start[layer], to = g->layer_start[layer + 1], next_count = g->layer_start[layer + 2] - to;
	uint32_t i, n = 0;
	long crossings = 0;
	for (i = from; i < to; i++) {
		int v = (int)g->layer_nodes[i];
		uint32_t e;
		uint32_t first = n;
		for (e = g->down_start[v]; e < g->down_start[v + 1]; e++) pairs[n++] = g->nodes[g->down_list[e]].order;
		/* sorted lower ends of the links of this node (insertion: few each) */
		{
			uint32_t j;
			for (j = first + 1; j < n; j++) {
				int value = pairs[j];
				uint32_t k = j;
				while (k > first && pairs[k - 1] > value) {
					pairs[k] = pairs[k - 1];
					k--;
				}
				pairs[k] = value;
			}
		}
	}
	memset(tree, 0, sizeof(int) * (next_count + 1));
	for (i = 0; i < n; i++) {
		/* the links already seen that end further right cross this one */
		int position = pairs[i] + 1, seen = 0, p;
		for (p = position; p > 0; p -= p & -p) seen += tree[p];
		crossings += (long)i - seen;
		for (p = position; p <= (int)next_count; p += p & -p) tree[p]++;
	}
	return crossings;
}

static long rvn_gl_all_crossings(rvn_gl_t* g, int* tree, int* pairs) {
	long total = 0;
	int l;
	for (l = 0; l + 1 < g->layer_count; l++) total += rvn_gl_crossings(g, l, tree, pairs);
	return total;
}

static void rvn_gl_number(rvn_gl_t* g, int layer) {
	uint32_t i;
	for (i = g->layer_start[layer]; i < g->layer_start[layer + 1]; i++) g->nodes[g->layer_nodes[i]].order = (int)(i - g->layer_start[layer]);
}

/* One layer sorted by the barycenters of its neighbours above (down = 1) or below. */
static void rvn_gl_sweep_layer(rvn_gl_t* g, int layer, int down) {
	uint32_t from = g->layer_start[layer], to = g->layer_start[layer + 1], i;
	for (i = from; i < to; i++) {
		int v = (int)g->layer_nodes[i];
		rvn_gl_node_t* n = &g->nodes[v];
		uint32_t start = down ? g->up_start[v] : g->down_start[v], stop = down ? g->up_start[v + 1] : g->down_start[v + 1], e;
		float sum = 0.0f, weight = 0.0f;
		for (e = start; e < stop; e++) {
			int w = down ? g->up_list[e] : g->down_list[e];
			sum += (float)g->nodes[w].order;
			weight += 1.0f;
		}
		n->bary = weight > 0.0f ? sum / weight : (float)n->order;
	}
	/* a note goes beside its state */
	for (i = from; i < to; i++) {
		rvn_gl_node_t* n = &g->nodes[g->layer_nodes[i]];
		if (n->host >= 0) n->bary = g->nodes[n->host].bary + 0.01f * (float)n->side;
	}
	/* the clusters: the mean barycenter of their nodes in the layer */
	for (i = from; i < to; i++) {
		int c = g->nodes[g->layer_nodes[i]].cluster, guard = 0;
		while (c >= 0 && guard++ < 64) {
			g->cluster_sum[c] = 0.0f;
			g->cluster_hits[c] = 0;
			c = g->mm->clusters[c].parent;
		}
	}
	for (i = from; i < to; i++) {
		rvn_gl_node_t* n = &g->nodes[g->layer_nodes[i]];
		int c = n->cluster, guard = 0;
		while (c >= 0 && guard++ < 64) {
			g->cluster_sum[c] += n->bary;
			g->cluster_hits[c]++;
			c = g->mm->clusters[c].parent;
		}
	}
	rvn_gl_sort(g, g->layer_nodes + from, to - from);
	rvn_gl_number(g, layer);
}

/*
 * The places of a layer given what each node wants (`want`, with a weight):
 * the nearest in the weighted least squares that keep the order and the
 * gaps (`gap[i]` between node i and i + 1). Pool adjacent violators.
 */
static void rvn_gl_isotonic(const float* want, const float* weight, const float* gap, uint32_t n, float* out, float* block_sum, float* block_weight, uint32_t* block_end) {
	uint32_t blocks = 0, i, j;
	float offset = 0.0f;
	for (i = 0; i < n; i++) {
		float target = want[i] - offset;
		block_sum[blocks] = target * weight[i];
		block_weight[blocks] = weight[i];
		block_end[blocks] = i + 1;
		blocks++;
		while (blocks >= 2 && block_sum[blocks - 2] / block_weight[blocks - 2] > block_sum[blocks - 1] / block_weight[blocks - 1]) {
			block_sum[blocks - 2] += block_sum[blocks - 1];
			block_weight[blocks - 2] += block_weight[blocks - 1];
			block_end[blocks - 2] = block_end[blocks - 1];
			blocks--;
		}
		if (i + 1 < n) offset += gap[i];
	}
	offset = 0.0f;
	for (i = 0, j = 0; j < blocks; j++) {
		float value = block_sum[j] / block_weight[j];
		for (; i < block_end[j]; i++) {
			out[i] = value + offset;
			if (i + 1 < n) offset += gap[i];
		}
	}
}

/* The gap kept between two nodes next to each other in a layer. */
static float rvn_gl_gap(const rvn_gl_t* g, int a, int b) {
	const rvn_gl_node_t* na = &g->nodes[a];
	const rvn_gl_node_t* nb = &g->nodes[b];
	float gap = (na->real >= 0 && nb->real >= 0) || na->label || nb->label ? g->node_gap : g->dummy_gap;
	int common = na->cluster == nb->cluster ? na->cluster : rvn_gl_common_cluster(g->mm, na->cluster, nb->cluster);
	int borders = rvn_gl_depth(g, na->cluster) + rvn_gl_depth(g, nb->cluster) - 2 * rvn_gl_depth(g, common);
	if (na->host == b || nb->host == a) gap = g->dummy_gap * 2.0f;
	return (na->cross + nb->cross) * 0.5f + na->extra + nb->extra + gap + (float)borders * g->cluster_pad;
}

/* One layer pulled toward its neighbours above (dir 1), below (dir -1) or both (0). */
static void rvn_gl_place_layer(rvn_gl_t* g, int layer, int dir, float* want, float* weight, float* gap, float* out, float* bs, float* bw, uint32_t* be) {
	uint32_t from = g->layer_start[layer], to = g->layer_start[layer + 1], n = to - from, i;
	if (n == 0) return;
	for (i = 0; i < n; i++) {
		int v = (int)g->layer_nodes[from + i];
		rvn_gl_node_t* node = &g->nodes[v];
		float sum = 0.0f, total = 0.0f;
		uint32_t e;
		if (dir >= 0) {
			for (e = g->up_start[v]; e < g->up_start[v + 1]; e++) {
				sum += g->nodes[g->up_list[e]].pos * g->up_weight[e];
				total += g->up_weight[e];
			}
		}
		if (dir <= 0) {
			for (e = g->down_start[v]; e < g->down_start[v + 1]; e++) {
				sum += g->nodes[g->down_list[e]].pos * g->down_weight[e];
				total += g->down_weight[e];
			}
		}
		if (node->host >= 0) {
			const rvn_gl_node_t* host = &g->nodes[node->host];
			want[i] = host->pos + (float)node->side * (host->cross * 0.5f + node->cross * 0.5f + g->dummy_gap * 2.0f);
			weight[i] = 20.0f;
		} else if (total > 0.0f) {
			want[i] = sum / total;
			weight[i] = total;
		} else {
			want[i] = node->pos;
			weight[i] = 0.05f;
		}
		if (i + 1 < n) gap[i] = rvn_gl_gap(g, v, (int)g->layer_nodes[from + i + 1]);
	}
	rvn_gl_isotonic(want, weight, gap, n, out, bs, bw, be);
	for (i = 0; i < n; i++) g->nodes[g->layer_nodes[from + i]].pos = out[i];
}

/* Cluster boxes across: the nodes of a cluster, over the layers it spans. */
typedef struct rvn_gl_box_t {
	float x0, x1;
	int l0, l1;
	int used;
} rvn_gl_box_t;

static void rvn_gl_cluster_boxes(rvn_gl_t* g, rvn_gl_box_t* boxes) {
	uint32_t i;
	for (i = 0; i < g->mm->cluster_count; i++) {
		boxes[i].x0 = 1e30f;
		boxes[i].x1 = -1e30f;
		boxes[i].l0 = 1 << 30;
		boxes[i].l1 = -1;
		boxes[i].used = 0;
	}
	for (i = 0; i < g->count; i++) {
		const rvn_gl_node_t* n = &g->nodes[i];
		int c = n->cluster, guard = 0, level = 0;
		while (c >= 0 && guard++ < 64) {
			/* inner clusters are inside: the outer box is wider by a padding for each level */
			float pad = g->cluster_pad * (float)(level + 1);
			rvn_gl_box_t* b = &boxes[c];
			b->x0 = rvn_minf(b->x0, n->pos - n->cross * 0.5f - n->extra - pad);
			b->x1 = rvn_maxf(b->x1, n->pos + n->cross * 0.5f + n->extra + pad);
			b->l0 = rvn_mini(b->l0, n->layer);
			b->l1 = rvn_maxi(b->l1, n->layer);
			b->used = 1;
			c = g->mm->clusters[c].parent;
			level++;
		}
	}
}

/* Pushes out of each cluster's box the nodes of its layers that are not in it. */
static void rvn_gl_clear_clusters(rvn_gl_t* g, rvn_gl_box_t* boxes) {
	int pass;
	for (pass = 0; pass < 3; pass++) {
		uint32_t c;
		int moved = 0;
		rvn_gl_cluster_boxes(g, boxes);
		/* innermost first */
		for (c = g->mm->cluster_count; c-- > 0;) {
			const rvn_gl_box_t* b = &boxes[c];
			int l;
			if (!b->used) continue;
			for (l = b->l0; l <= b->l1; l++) {
				uint32_t from = g->layer_start[l], to = g->layer_start[l + 1], i;
				/* the members of the cluster in the layer are next to each other: what is before goes left, after goes right */
				int first = -1, last = -1;
				for (i = from; i < to; i++) {
					if (rvn_gl_common_cluster(g->mm, g->nodes[g->layer_nodes[i]].cluster, (int)c) == (int)c) {
						if (first < 0) first = (int)i;
						last = (int)i;
					}
				}
				for (i = from; i < to; i++) {
					rvn_gl_node_t* n = &g->nodes[g->layer_nodes[i]];
					int left;
					float shift;
					if ((int)i >= first && (int)i <= last && first >= 0) continue;
					left = first >= 0 ? (int)i < first : n->pos < (b->x0 + b->x1) * 0.5f;
					if (left) {
						shift = (n->pos + n->cross * 0.5f + n->extra + g->dummy_gap) - b->x0;
						if (shift > 0.01f) {
							uint32_t j;
							for (j = from; j <= i; j++) g->nodes[g->layer_nodes[j]].pos -= shift;
							moved = 1;
						}
					} else {
						shift = b->x1 - (n->pos - n->cross * 0.5f - n->extra - g->dummy_gap);
						if (shift > 0.01f) {
							uint32_t j;
							for (j = i; j < to; j++) g->nodes[g->layer_nodes[j]].pos += shift;
							moved = 1;
						}
						break;
					}
				}
			}
		}
		if (!moved) break;
	}
}

/* The point of a node's outline where a link leaves or arrives along the flow (TB: below or above its center). */
static float rvn_gl_port(const rvn_gl_node_t* n, int below) {
	return below ? n->along * 0.5f : -n->along * 0.5f;
}

void rvn_mm_layout_graph(rvn_mm_t* mm, const rvn_mm_measure_t* m) {
	rvn_gl_t g;
	float k = m->size / 14.0f;
	int vertical = mm->direction == 'T' || mm->direction == 'B';
	uint32_t n_real = mm->node_count, i, e;
	int any_label = 0;
	int* layer;
	int* topo;
	int* indegree;
	uint32_t* out_start;
	uint32_t* out_list;
	int* state;
	int* stack;
	uint32_t* stack_edge;
	rvn_text_t* wrapped;
	rvn_mm_block_t* blocks;
	float* layer_y;
	float* layer_thick;
	float total_w = 0.0f, total_h = 0.0f, min_x, max_x;
	int max_layer = 0;
	uint32_t dummy_budget = 120000u, dummies = 0;
	rvn_gl_box_t* boxes;
	float* cbox;

	memset(&g, 0, sizeof(g));
	g.mm = mm;
	g.k = k;
	g.node_gap = (mm->kind == R_MERMAID_STATE ? 36.0f : 40.0f) * k;
	g.dummy_gap = 10.0f * k;
	g.cluster_pad = 14.0f * k;
	g.title_h = m->line_height + 8.0f * k;
	g.rank_gap = 46.0f * k;
	if (n_real == 0) {
		mm->width = mm->height = 0.0f;
		return;
	}

	/* 1. sizes of the nodes and of the texts of the links */
	wrapped = (rvn_text_t*)rvn_calloc(sizeof(rvn_text_t) * (n_real + mm->edge_count));
	blocks = (rvn_mm_block_t*)rvn_calloc(sizeof(rvn_mm_block_t) * (n_real + mm->edge_count));
	for (i = 0; i < n_real; i++) {
		rvn_mm_node_t* node = &mm->nodes[i];
		int shape = node->shape;
		int has_text = shape != RVN_MM_START && shape != RVN_MM_END && shape != RVN_MM_BAR;
		float max_width = shape == RVN_MM_NOTE ? 220.0f * k : 200.0f * k;
		if (shape == RVN_MM_RHOMBUS || shape == RVN_MM_CIRCLE || shape == RVN_MM_DOUBLE_CIRCLE) max_width = 150.0f * k;
		blocks[i] = has_text ? rvn_mm_text_block(m, node->label, max_width, R_FALSE, 1.0f, &wrapped[i]) : blocks[i];
		if (!has_text) rvn_text_append(&wrapped[i], "", 0);
		rvn_mm_shape_size(shape, blocks[i].w, rvn_maxf(blocks[i].h, m->line_height), k, &node->w, &node->h);
		if (shape == RVN_MM_BAR && !vertical) {
			float t = node->w;
			node->w = node->h;
			node->h = t;
		}
	}
	for (e = 0; e < mm->edge_count; e++) {
		rvn_mm_edge_t* edge = &mm->edges[e];
		if (edge->label && edge->label[0] && edge->length > 0 && edge->from != edge->to) {
			blocks[n_real + e] = rvn_mm_text_block(m, edge->label, 160.0f * k, R_FALSE, 1.0f, &wrapped[n_real + e]);
			any_label = 1;
		} else if (edge->label && edge->label[0]) {
			blocks[n_real + e] = rvn_mm_text_block(m, edge->label, 160.0f * k, R_FALSE, 1.0f, &wrapped[n_real + e]);
		}
	}
	if (any_label) g.rank_gap = 22.0f * k;

	/* the clusters: depth and chain from the outermost */
	if (mm->cluster_count) {
		g.cluster_depth = (int*)rvn_calloc(sizeof(int) * mm->cluster_count);
		g.cluster_chain = (int*)rvn_calloc(sizeof(int) * mm->cluster_count * 64u);
		g.cluster_sum = (float*)rvn_calloc(sizeof(float) * mm->cluster_count);
		g.cluster_hits = (int*)rvn_calloc(sizeof(int) * mm->cluster_count);
		for (i = 0; i < mm->cluster_count; i++) {
			int chain[64], depth = 0, c = (int)i, d;
			while (c >= 0 && depth < 64) {
				chain[depth++] = c;
				c = mm->clusters[c].parent;
			}
			g.cluster_depth[i] = depth;
			for (d = 0; d < depth; d++) g.cluster_chain[i * 64u + (uint32_t)d] = chain[depth - 1 - d];
		}
	}

	/* the real nodes come first in the layout */
	for (i = 0; i < n_real; i++) {
		int v = rvn_gl_add_node(&g);
		rvn_gl_node_t* n = &g.nodes[v];
		n->real = (int)i;
		n->cluster = mm->nodes[i].cluster;
		n->cross = vertical ? mm->nodes[i].w : mm->nodes[i].h;
		n->along = vertical ? mm->nodes[i].h : mm->nodes[i].w;
	}
	for (e = 0; e < mm->edge_count; e++) {
		const rvn_mm_edge_t* edge = &mm->edges[e];
		if (edge->from == edge->to && edge->from >= 0) {
			float room = 28.0f * k;
			if (edge->label && edge->label[0]) room += (vertical ? blocks[n_real + e].w : blocks[n_real + e].h) + 6.0f * k;
			g.nodes[edge->from].extra = rvn_maxf(g.nodes[edge->from].extra, room * 0.5f);
		} else if (edge->length == 0 && edge->from >= 0 && edge->to >= 0) {
			/* a note: beside its state */
			int note = mm->nodes[edge->from].shape == RVN_MM_NOTE ? edge->from : edge->to;
			int host = note == edge->from ? edge->to : edge->from;
			if (g.nodes[note].host < 0 && mm->nodes[host].shape != RVN_MM_NOTE) {
				g.nodes[note].host = host;
				g.nodes[note].side = note == edge->from ? -1 : 1;
			}
		}
	}

	/* 2. cycles broken: a depth-first search, the links back to a node on the path are turned */
	out_start = (uint32_t*)rvn_calloc(sizeof(uint32_t) * (n_real + 1));
	out_list = (uint32_t*)rvn_calloc(sizeof(uint32_t) * (mm->edge_count + 1));
	g.reversed = (uint8_t*)rvn_calloc(mm->edge_count + 1);
	for (e = 0; e < mm->edge_count; e++) {
		const rvn_mm_edge_t* edge = &mm->edges[e];
		if (edge->from != edge->to && edge->length > 0) out_start[edge->from + 1]++;
	}
	for (i = 0; i < n_real; i++) out_start[i + 1] += out_start[i];
	{
		uint32_t* fill = (uint32_t*)rvn_alloc(sizeof(uint32_t) * (n_real + 1));
		memcpy(fill, out_start, sizeof(uint32_t) * (n_real + 1));
		for (e = 0; e < mm->edge_count; e++) {
			const rvn_mm_edge_t* edge = &mm->edges[e];
			if (edge->from != edge->to && edge->length > 0) out_list[fill[edge->from]++] = e;
		}
		rvn_free(fill);
	}
	state = (int*)rvn_calloc(sizeof(int) * n_real);
	stack = (int*)rvn_alloc(sizeof(int) * (n_real + 1));
	stack_edge = (uint32_t*)rvn_alloc(sizeof(uint32_t) * (n_real + 1));
	for (i = 0; i < n_real; i++) {
		int top = 0;
		if (state[i]) continue;
		stack[0] = (int)i;
		stack_edge[0] = out_start[i];
		state[i] = 1;
		top = 1;
		while (top > 0) {
			int v = stack[top - 1];
			if (stack_edge[top - 1] < out_start[v + 1]) {
				uint32_t edge_index = out_list[stack_edge[top - 1]++];
				int w = mm->edges[edge_index].to;
				if (state[w] == 1) g.reversed[edge_index] = 1;
				else if (state[w] == 0) {
					state[w] = 1;
					stack[top] = w;
					stack_edge[top] = out_start[w];
					top++;
				}
			} else {
				state[v] = 2;
				top--;
			}
		}
	}

	/* 3. layers: longest path from the sources (Kahn's order), the links that close a cycle turned */
	layer = (int*)rvn_calloc(sizeof(int) * n_real);
	topo = (int*)rvn_alloc(sizeof(int) * n_real);
	indegree = (int*)rvn_calloc(sizeof(int) * n_real);
	{
		/* the links as they go down, after turning */
		uint32_t* down_start = (uint32_t*)rvn_calloc(sizeof(uint32_t) * (n_real + 1));
		uint32_t* down_list = (uint32_t*)rvn_calloc(sizeof(uint32_t) * (mm->edge_count + 1));
		uint32_t* fill = (uint32_t*)rvn_alloc(sizeof(uint32_t) * (n_real + 1));
		uint32_t head = 0, tail = 0;
		for (e = 0; e < mm->edge_count; e++) {
			const rvn_mm_edge_t* edge = &mm->edges[e];
			int up;
			if (edge->from == edge->to || edge->length == 0) continue;
			up = g.reversed[e] ? edge->to : edge->from;
			down_start[up + 1]++;
			indegree[g.reversed[e] ? edge->from : edge->to]++;
		}
		for (i = 0; i < n_real; i++) down_start[i + 1] += down_start[i];
		memcpy(fill, down_start, sizeof(uint32_t) * (n_real + 1));
		for (e = 0; e < mm->edge_count; e++) {
			const rvn_mm_edge_t* edge = &mm->edges[e];
			if (edge->from == edge->to || edge->length == 0) continue;
			down_list[fill[g.reversed[e] ? edge->to : edge->from]++] = e;
		}
		for (i = 0; i < n_real; i++) {
			if (indegree[i] == 0) topo[tail++] = (int)i;
		}
		while (head < tail) {
			int v = topo[head++];
			uint32_t j;
			for (j = down_start[v]; j < down_start[v + 1]; j++) {
				const rvn_mm_edge_t* edge = &mm->edges[down_list[j]];
				int w = g.reversed[down_list[j]] ? edge->from : edge->to;
				int span = (int)edge->length * (any_label ? 2 : 1);
				layer[w] = rvn_maxi(layer[w], layer[v] + span);
				if (--indegree[w] == 0) topo[tail++] = w;
			}
		}
		/* (a cycle left by a bug would leave nodes out of the order: they keep layer 0) */
		/* the sources down next to what they lead to */
		{
			/* which nodes have a link coming in */
			uint8_t* has_incoming = (uint8_t*)rvn_calloc(n_real);
			for (e = 0; e < mm->edge_count; e++) {
				const rvn_mm_edge_t* edge = &mm->edges[e];
				if (edge->from == edge->to || edge->length == 0) continue;
				has_incoming[g.reversed[e] ? edge->from : edge->to] = 1;
			}
			for (i = tail; i-- > 0;) {
				int v = topo[i];
				uint32_t j;
				int best = 1 << 30;
				if (has_incoming[v]) continue;
				for (j = down_start[v]; j < down_start[v + 1]; j++) {
					const rvn_mm_edge_t* edge = &mm->edges[down_list[j]];
					int w = g.reversed[down_list[j]] ? edge->from : edge->to;
					best = rvn_mini(best, layer[w] - (int)edge->length * (any_label ? 2 : 1));
				}
				if (best < (1 << 30) && best > layer[v]) layer[v] = best;
			}
			rvn_free(has_incoming);
		}
		rvn_free(down_start);
		rvn_free(down_list);
		rvn_free(fill);
	}
	/* notes take the layer of their state */
	for (i = 0; i < n_real; i++) {
		if (g.nodes[i].host >= 0) layer[i] = layer[g.nodes[i].host];
	}
	for (i = 0; i < n_real; i++) {
		g.nodes[i].layer = layer[i];
		max_layer = rvn_maxi(max_layer, layer[i]);
	}

	/* 4. the chains of the links: points for each layer they cross, the text in the middle */
	g.chain_start = (uint32_t*)rvn_calloc(sizeof(uint32_t) * (mm->edge_count + 1));
	g.chain_length = (uint32_t*)rvn_calloc(sizeof(uint32_t) * (mm->edge_count + 1));
	for (e = 0; e < mm->edge_count; e++) {
		const rvn_mm_edge_t* edge = &mm->edges[e];
		int up, down, span, l, prev;
		float weight_base;
		if (edge->from == edge->to || edge->length == 0 || edge->from < 0 || edge->to < 0) continue;
		up = g.reversed[e] ? edge->to : edge->from;
		down = g.reversed[e] ? edge->from : edge->to;
		span = layer[down] - layer[up];
		if (span < 1) continue;
		g.chain_start[e] = g.chain_count;
		RVN_ARRAY_RESERVE(g.chain, g.chain_cap, g.chain_count + (uint32_t)span + 1u);
		g.chain[g.chain_count++] = (uint32_t)up;
		prev = up;
		weight_base = edge->line == RVN_MM_LINE_INVISIBLE ? 0.5f : 1.0f;
		if (dummies + (uint32_t)span > dummy_budget) {
			/* too many points already: this link goes straight, outside of the order */
			g.chain[g.chain_count++] = (uint32_t)down;
			g.chain_length[e] = 2;
			continue;
		}
		for (l = layer[up] + 1; l < layer[down]; l++) {
			int d = rvn_gl_add_node(&g);
			rvn_gl_node_t* dn = &g.nodes[d];
			dn->layer = l;
			dn->cluster = rvn_gl_common_cluster(mm, mm->nodes[up].cluster, mm->nodes[down].cluster);
			if (blocks[n_real + e].lines > 0 && l == layer[up] + span / 2) {
				dn->label = 1;
				dn->cross = (vertical ? blocks[n_real + e].w : blocks[n_real + e].h) + 8.0f * k;
				dn->along = (vertical ? blocks[n_real + e].h : blocks[n_real + e].w) + 4.0f * k;
			}
			rvn_gl_add_edge(&g, prev, d, weight_base * (prev >= (int)n_real ? 8.0f : 2.0f));
			RVN_ARRAY_RESERVE(g.chain, g.chain_cap, g.chain_count + 1);
			g.chain[g.chain_count++] = (uint32_t)d;
			prev = d;
			dummies++;
		}
		rvn_gl_add_edge(&g, prev, down, weight_base * (prev >= (int)n_real ? 2.0f : 1.0f));
		RVN_ARRAY_RESERVE(g.chain, g.chain_cap, g.chain_count + 1);
		g.chain[g.chain_count++] = (uint32_t)down;
		g.chain_length[e] = g.chain_count - g.chain_start[e];
	}

	/* the layers, and the neighbours of each node */
	g.layer_count = max_layer + 1;
	g.layer_start = (uint32_t*)rvn_calloc(sizeof(uint32_t) * ((size_t)g.layer_count + 2));
	g.layer_nodes = (uint32_t*)rvn_alloc(sizeof(uint32_t) * (g.count + 1));
	g.up_start = (uint32_t*)rvn_calloc(sizeof(uint32_t) * (g.count + 1));
	g.down_start = (uint32_t*)rvn_calloc(sizeof(uint32_t) * (g.count + 1));
	g.up_list = (int*)rvn_alloc(sizeof(int) * (g.edge_count + 1));
	g.down_list = (int*)rvn_alloc(sizeof(int) * (g.edge_count + 1));
	g.up_weight = (float*)rvn_alloc(sizeof(float) * (g.edge_count + 1));
	g.down_weight = (float*)rvn_alloc(sizeof(float) * (g.edge_count + 1));
	g.scratch = (int*)rvn_alloc(sizeof(int) * (g.count + 1));
	for (i = 0; i < g.edge_count; i++) {
		g.up_start[g.edges[i].down + 1]++;
		g.down_start[g.edges[i].up + 1]++;
	}
	for (i = 0; i < g.count; i++) {
		g.up_start[i + 1] += g.up_start[i];
		g.down_start[i + 1] += g.down_start[i];
	}
	{
		uint32_t* up_fill = (uint32_t*)rvn_alloc(sizeof(uint32_t) * (g.count + 1));
		uint32_t* down_fill = (uint32_t*)rvn_alloc(sizeof(uint32_t) * (g.count + 1));
		memcpy(up_fill, g.up_start, sizeof(uint32_t) * (g.count + 1));
		memcpy(down_fill, g.down_start, sizeof(uint32_t) * (g.count + 1));
		for (i = 0; i < g.edge_count; i++) {
			const rvn_gl_edge_t* edge = &g.edges[i];
			g.up_list[up_fill[edge->down]] = edge->up;
			g.up_weight[up_fill[edge->down]++] = edge->weight;
			g.down_list[down_fill[edge->up]] = edge->down;
			g.down_weight[down_fill[edge->up]++] = edge->weight;
		}
		rvn_free(up_fill);
		rvn_free(down_fill);
	}

	/* 5. first order: depth first from the sources, in the order of the text */
	{
		uint32_t* count = (uint32_t*)rvn_calloc(sizeof(uint32_t) * ((size_t)g.layer_count + 1));
		uint8_t* seen = (uint8_t*)rvn_calloc(g.count + 1);
		int* dfs = (int*)rvn_alloc(sizeof(int) * (g.count + 1));
		uint32_t* dfs_edge = (uint32_t*)rvn_alloc(sizeof(uint32_t) * (g.count + 1));
		uint32_t* fill = (uint32_t*)rvn_alloc(sizeof(uint32_t) * ((size_t)g.layer_count + 1));
		int pass;
		for (i = 0; i < g.count; i++) count[g.nodes[i].layer + 1]++;
		for (i = 0; i < (uint32_t)g.layer_count; i++) count[i + 1] += count[i];
		memcpy(g.layer_start, count, sizeof(uint32_t) * ((size_t)g.layer_count + 1));
		g.layer_start[g.layer_count + 1] = g.layer_start[g.layer_count];
		memcpy(fill, count, sizeof(uint32_t) * ((size_t)g.layer_count + 1));
		/* the sources first, then whatever is left (nodes inside cycles) */
		for (pass = 0; pass < 2; pass++) {
			for (i = 0; i < g.count; i++) {
				int top;
				if (seen[i] || g.nodes[i].host >= 0) continue;
				if (pass == 0 && g.up_start[i + 1] > g.up_start[i]) continue;
				dfs[0] = (int)i;
				dfs_edge[0] = g.down_start[i];
				top = 1;
				seen[i] = 1;
				g.layer_nodes[fill[g.nodes[i].layer]++] = i;
				while (top > 0) {
					int v = dfs[top - 1];
					if (dfs_edge[top - 1] < g.down_start[v + 1]) {
						int w = g.down_list[dfs_edge[top - 1]++];
						if (!seen[w] && g.nodes[w].host < 0) {
							seen[w] = 1;
							g.layer_nodes[fill[g.nodes[w].layer]++] = (uint32_t)w;
							dfs[top] = w;
							dfs_edge[top] = g.down_start[w];
							top++;
						}
					} else {
						top--;
					}
				}
			}
		}
		for (i = 0; i < g.count; i++) {
			if (!seen[i]) g.layer_nodes[fill[g.nodes[i].layer]++] = i;
		}
		for (i = 0; i < (uint32_t)g.layer_count; i++) rvn_gl_number(&g, (int)i);
		rvn_free(count);
		rvn_free(seen);
		rvn_free(dfs);
		rvn_free(dfs_edge);
		rvn_free(fill);
	}

	/* 6. fewer crossings: sweeps down and up, the best order kept */
	{
		int* tree = (int*)rvn_alloc(sizeof(int) * (g.count + 2));
		int* pairs = (int*)rvn_alloc(sizeof(int) * (g.edge_count + 1));
		uint32_t* best = (uint32_t*)rvn_alloc(sizeof(uint32_t) * (g.count + 1));
		long best_crossings, crossings;
		int sweep, sweeps = g.count > 20000u ? 4 : (g.count > 3000u ? 8 : 24), stale = 0, l;
		/* (the clusters are sorted as a whole from the start) */
		for (l = 0; l < g.layer_count; l++) {
			uint32_t j;
			for (j = g.layer_start[l]; j < g.layer_start[l + 1]; j++) g.nodes[g.layer_nodes[j]].bary = (float)g.nodes[g.layer_nodes[j]].order;
			rvn_gl_sweep_layer(&g, l, 1);
		}
		best_crossings = rvn_gl_all_crossings(&g, tree, pairs);
		memcpy(best, g.layer_nodes, sizeof(uint32_t) * g.count);
		for (sweep = 0; sweep < sweeps && best_crossings > 0; sweep++) {
			if (sweep % 2 == 0) {
				for (l = 1; l < g.layer_count; l++) rvn_gl_sweep_layer(&g, l, 1);
			} else {
				for (l = g.layer_count - 2; l >= 0; l--) rvn_gl_sweep_layer(&g, l, 0);
			}
			crossings = rvn_gl_all_crossings(&g, tree, pairs);
			if (crossings < best_crossings) {
				best_crossings = crossings;
				memcpy(best, g.layer_nodes, sizeof(uint32_t) * g.count);
				stale = 0;
			} else if (++stale >= 4) {
				break;
			}
		}
		memcpy(g.layer_nodes, best, sizeof(uint32_t) * g.count);
		for (l = 0; l < g.layer_count; l++) rvn_gl_number(&g, l);
		rvn_free(tree);
		rvn_free(pairs);
		rvn_free(best);
	}

	/* 7. places across: packed, then pulled toward the neighbours */
	{
		uint32_t widest = 1;
		float* want;
		float* weight;
		float* gap;
		float* out;
		float* bs;
		float* bw;
		uint32_t* be;
		int l, pass, passes = g.count > 20000u ? 2 : 8;
		for (l = 0; l < g.layer_count; l++) widest = rvn_maxu(widest, g.layer_start[l + 1] - g.layer_start[l]);
		want = (float*)rvn_alloc(sizeof(float) * widest);
		weight = (float*)rvn_alloc(sizeof(float) * widest);
		gap = (float*)rvn_alloc(sizeof(float) * widest);
		out = (float*)rvn_alloc(sizeof(float) * widest);
		bs = (float*)rvn_alloc(sizeof(float) * widest);
		bw = (float*)rvn_alloc(sizeof(float) * widest);
		be = (uint32_t*)rvn_alloc(sizeof(uint32_t) * widest);
		for (l = 0; l < g.layer_count; l++) {
			uint32_t j;
			float x = 0.0f;
			for (j = g.layer_start[l]; j < g.layer_start[l + 1]; j++) {
				if (j > g.layer_start[l]) x += rvn_gl_gap(&g, (int)g.layer_nodes[j - 1], (int)g.layer_nodes[j]);
				g.nodes[g.layer_nodes[j]].pos = x;
			}
			/* centered on 0 */
			for (j = g.layer_start[l]; j < g.layer_start[l + 1]; j++) g.nodes[g.layer_nodes[j]].pos -= x * 0.5f;
		}
		for (pass = 0; pass < passes; pass++) {
			for (l = 1; l < g.layer_count; l++) rvn_gl_place_layer(&g, l, 1, want, weight, gap, out, bs, bw, be);
			for (l = g.layer_count - 2; l >= 0; l--) rvn_gl_place_layer(&g, l, -1, want, weight, gap, out, bs, bw, be);
		}
		for (l = 0; l < g.layer_count; l++) rvn_gl_place_layer(&g, l, 0, want, weight, gap, out, bs, bw, be);
		rvn_free(want);
		rvn_free(weight);
		rvn_free(gap);
		rvn_free(out);
		rvn_free(bs);
		rvn_free(bw);
		rvn_free(be);
	}
	boxes = (rvn_gl_box_t*)rvn_calloc(sizeof(rvn_gl_box_t) * (mm->cluster_count + 1));
	if (mm->cluster_count) rvn_gl_clear_clusters(&g, boxes);

	/* 8. places along: layers as thick as their thickest node, gaps for the titles of the clusters */
	layer_y = (float*)rvn_calloc(sizeof(float) * ((size_t)g.layer_count + 1));
	layer_thick = (float*)rvn_calloc(sizeof(float) * ((size_t)g.layer_count + 1));
	{
		float* above = (float*)rvn_calloc(sizeof(float) * ((size_t)g.layer_count + 1));
		float* below = (float*)rvn_calloc(sizeof(float) * ((size_t)g.layer_count + 1));
		float y = 0.0f;
		int l;
		for (i = 0; i < g.count; i++) layer_thick[g.nodes[i].layer] = rvn_maxf(layer_thick[g.nodes[i].layer], g.nodes[i].along);
		if (mm->cluster_count) {
			rvn_gl_cluster_boxes(&g, boxes);
			for (i = 0; i < mm->cluster_count; i++) {
				if (!boxes[i].used) continue;
				above[boxes[i].l0] += g.cluster_pad + (vertical ? g.title_h : 0.0f);
				below[boxes[i].l1] += g.cluster_pad;
			}
		}
		for (l = 0; l < g.layer_count; l++) {
			if (l > 0) y += g.rank_gap + below[l - 1] + above[l];
			else y += above[0];
			layer_y[l] = y;
			y += layer_thick[l];
		}
		total_h = y + below[g.layer_count - 1];
		rvn_free(above);
		rvn_free(below);
	}

	/* 9. the places of the model, turned to the direction; the bounds */
	min_x = 1e30f;
	max_x = -1e30f;
	for (i = 0; i < g.count; i++) {
		const rvn_gl_node_t* n = &g.nodes[i];
		min_x = rvn_minf(min_x, n->pos - n->cross * 0.5f - n->extra);
		max_x = rvn_maxf(max_x, n->pos + n->cross * 0.5f + n->extra);
	}
	if (mm->cluster_count) {
		for (i = 0; i < mm->cluster_count; i++) {
			if (!boxes[i].used) continue;
			min_x = rvn_minf(min_x, boxes[i].x0);
			max_x = rvn_maxf(max_x, boxes[i].x1);
		}
	}
	total_w = max_x - min_x;
	{
		float margin = 8.0f * k;
		/* (x across, y along) to the diagram */
#define RVN_GL_X(c, a) (mm->direction == 'T' || mm->direction == 'B' ? (c) - min_x + margin : (mm->direction == 'L' ? (a) + margin : total_h - (a) + margin))
#define RVN_GL_Y(c, a) (mm->direction == 'T' ? (a) + margin : (mm->direction == 'B' ? total_h - (a) + margin : (c) - min_x + margin))
		cbox = (float*)rvn_calloc(sizeof(float) * 4u * (mm->cluster_count + 1));
		for (i = 0; i < n_real; i++) {
			const rvn_gl_node_t* n = &g.nodes[i];
			float along = layer_y[n->layer] + layer_thick[n->layer] * 0.5f;
			mm->nodes[i].x = RVN_GL_X(n->pos, along);
			mm->nodes[i].y = RVN_GL_Y(n->pos, along);
			mm->nodes[i].layer = n->layer;
			mm->nodes[i].order = n->order;
		}
		mm->width = (vertical ? total_w : total_h) + margin * 2.0f;
		mm->height = (vertical ? total_h : total_w) + margin * 2.0f;

		/* the boxes of the clusters, from the innermost: their nodes, the clusters inside, a title on top */
		if (mm->cluster_count) {
			int* order = (int*)rvn_alloc(sizeof(int) * mm->cluster_count);
			uint32_t a, b;
			for (a = 0; a < mm->cluster_count; a++) {
				cbox[a * 4] = 1e30f;
				cbox[a * 4 + 1] = 1e30f;
				cbox[a * 4 + 2] = -1e30f;
				cbox[a * 4 + 3] = -1e30f;
				order[a] = (int)a;
			}
			/* deepest first */
			for (a = 1; a < mm->cluster_count; a++) {
				int v = order[a];
				b = a;
				while (b > 0 && g.cluster_depth[order[b - 1]] < g.cluster_depth[v]) {
					order[b] = order[b - 1];
					b--;
				}
				order[b] = v;
			}
			for (i = 0; i < g.count; i++) {
				const rvn_gl_node_t* n = &g.nodes[i];
				float along = layer_y[n->layer] + layer_thick[n->layer] * 0.5f;
				float cx = RVN_GL_X(n->pos, along), cy = RVN_GL_Y(n->pos, along);
				float hw = (vertical ? n->cross : n->along) * 0.5f + (vertical ? n->extra : 0.0f);
				float hh = (vertical ? n->along : n->cross) * 0.5f + (vertical ? 0.0f : n->extra);
				int c = n->cluster;
				if (c < 0) continue;
				if (n->real < 0 && !n->label) {
					/* a point of a link counts across only */
					hw = vertical ? hw : 0.0f;
					hh = vertical ? 0.0f : hh;
				}
				cbox[c * 4] = rvn_minf(cbox[c * 4], cx - hw);
				cbox[c * 4 + 1] = rvn_minf(cbox[c * 4 + 1], cy - hh);
				cbox[c * 4 + 2] = rvn_maxf(cbox[c * 4 + 2], cx + hw);
				cbox[c * 4 + 3] = rvn_maxf(cbox[c * 4 + 3], cy + hh);
			}
			for (a = 0; a < mm->cluster_count; a++) {
				int c = order[a];
				int parent = mm->clusters[c].parent;
				rvn_mm_block_t title;
				rvn_text_t scratch = { NULL, 0, 0 };
				float* box = &cbox[c * 4];
				if (box[0] > box[2]) continue;
				box[0] -= g.cluster_pad;
				box[1] -= g.cluster_pad;
				box[2] += g.cluster_pad;
				box[3] += g.cluster_pad;
				title = rvn_mm_text_block(m, mm->clusters[c].title, 0.0f, R_FALSE, 1.0f, &scratch);
				rvn_free(scratch.data);
				if (title.lines > 0 && mm->clusters[c].title[0]) {
					float need = title.w + g.cluster_pad * 2.0f - (box[2] - box[0]);
					if (vertical) box[1] -= rvn_maxf(title.h, m->line_height) + 6.0f * k - g.cluster_pad * 0.5f;
					else box[1] -= title.h + 4.0f * k;
					if (need > 0.0f) {
						box[0] -= need * 0.5f;
						box[2] += need * 0.5f;
					}
				}
				if (parent >= 0) {
					cbox[parent * 4] = rvn_minf(cbox[parent * 4], box[0]);
					cbox[parent * 4 + 1] = rvn_minf(cbox[parent * 4 + 1], box[1]);
					cbox[parent * 4 + 2] = rvn_maxf(cbox[parent * 4 + 2], box[2]);
					cbox[parent * 4 + 3] = rvn_maxf(cbox[parent * 4 + 3], box[3]);
				}
			}
			/* the scene: outer clusters first */
			for (a = mm->cluster_count; a-- > 0;) {
				int c = order[a];
				float* box = &cbox[c * 4];
				rvn_mm_item_t* item;
				rvn_mm_cluster_t* cl = &mm->clusters[c];
				if (box[0] > box[2]) continue;
				cl->x = box[0];
				cl->y = box[1];
				cl->w = box[2] - box[0];
				cl->h = box[3] - box[1];
				item = rvn_mm_add_shape(mm, RVN_MM_CLUSTER, cl->x, cl->y, cl->w, cl->h, RVN_MM_C_CLUSTER, RVN_MM_C_CLUSTER_BORDER);
				if (mm->kind == R_MERMAID_STATE) {
					item->shape = RVN_MM_ROUND;
					item->fill = RVN_MM_C_BACKGROUND;
					item->stroke = RVN_MM_C_NODE_BORDER;
				}
				rvn_mm_apply_style(item, &cl->style);
				if (cl->title && cl->title[0]) {
					rvn_text_t t = { NULL, 0, 0 };
					rvn_mm_block_t title = rvn_mm_text_block(m, cl->title, 0.0f, R_FALSE, 1.0f, &t);
					rvn_mm_item_t* text = rvn_mm_add_text(mm, t.data, cl->x, cl->y + 4.0f * k, cl->w, title.h, 0.5f, RVN_MM_C_TEXT);
					if (rvn_color_a(cl->style.color)) {
						text->text = RVN_MM_C_EXPLICIT;
						text->text_color = cl->style.color;
					}
					rvn_free(t.data);
				}
				/* the bounds grow with the boxes */
				if (cl->x < 0.0f || cl->y < 0.0f || cl->x + cl->w > mm->width || cl->y + cl->h > mm->height) {
					mm->width = rvn_maxf(mm->width, cl->x + cl->w + margin);
					mm->height = rvn_maxf(mm->height, cl->y + cl->h + margin);
				}
			}
			rvn_free(order);
		}

		/* 10. the links: curves through their points, leaving and reaching along the flow */
		for (e = 0; e < mm->edge_count; e++) {
			rvn_mm_edge_t* edge = &mm->edges[e];
			rvn_mm_item_t* item;
			uint32_t start = mm->point_count;
			edge->first_point = start;
			edge->point_count = 0;
			if (edge->from < 0 || edge->to < 0) continue;
			if (edge->from == edge->to) {
				/* a loop on the side of the node (the right, going down) */
				const rvn_gl_node_t* n = &g.nodes[edge->from];
				float along = layer_y[n->layer] + layer_thick[n->layer] * 0.5f;
				float side = n->pos + n->cross * 0.5f;
				float reach = side + 26.0f * k;
				float a0 = along - n->along * 0.25f, a1 = along + n->along * 0.25f;
				float c0 = RVN_GL_X(side, a0), d0 = RVN_GL_Y(side, a0);
				rvn_mm_add_point(mm, c0, d0);
				rvn_mm_add_cubic(mm, c0, d0, RVN_GL_X(reach, a0 - n->along * 0.35f), RVN_GL_Y(reach, a0 - n->along * 0.35f),
					RVN_GL_X(reach, a1 + n->along * 0.35f), RVN_GL_Y(reach, a1 + n->along * 0.35f), RVN_GL_X(side, a1), RVN_GL_Y(side, a1));
				edge->label_x = RVN_GL_X(reach + (vertical ? blocks[n_real + e].w * 0.5f : blocks[n_real + e].h * 0.5f) + 2.0f * k, along);
				edge->label_y = RVN_GL_Y(reach + (vertical ? blocks[n_real + e].w * 0.5f : blocks[n_real + e].h * 0.5f) + 2.0f * k, along);
			} else if (edge->length == 0) {
				/* a note and its state: a straight line between their sides */
				const rvn_gl_node_t* a = &g.nodes[edge->from];
				const rvn_gl_node_t* b = &g.nodes[edge->to];
				float along_a = layer_y[a->layer] + layer_thick[a->layer] * 0.5f;
				float along_b = layer_y[b->layer] + layer_thick[b->layer] * 0.5f;
				float sa = a->pos < b->pos ? 1.0f : -1.0f;
				rvn_mm_add_point(mm, RVN_GL_X(a->pos + sa * a->cross * 0.5f, along_a), RVN_GL_Y(a->pos + sa * a->cross * 0.5f, along_a));
				rvn_mm_add_point(mm, RVN_GL_X(b->pos - sa * b->cross * 0.5f, along_b), RVN_GL_Y(b->pos - sa * b->cross * 0.5f, along_b));
			} else if (g.chain_length[e] >= 2) {
				uint32_t c, count = g.chain_length[e];
				const uint32_t* chain = &g.chain[g.chain_start[e]];
				float px = 0.0f, pa = 0.0f;
				int label_set = 0;
				for (c = 0; c < count; c++) {
					const rvn_gl_node_t* n = &g.nodes[chain[c]];
					float top = layer_y[n->layer], bottom = layer_y[n->layer] + layer_thick[n->layer];
					float center = (top + bottom) * 0.5f;
					float in_a, out_a;
					if (c == 0) {
						in_a = out_a = center + rvn_gl_port(n, 1);
					} else if (c + 1 == count) {
						in_a = out_a = center + rvn_gl_port(n, 0);
					} else {
						/* a point of the link: straight through its layer */
						in_a = top;
						out_a = bottom;
					}
					if (c == 0) {
						rvn_mm_add_point(mm, RVN_GL_X(n->pos, out_a), RVN_GL_Y(n->pos, out_a));
					} else {
						float mid = (pa + in_a) * 0.5f;
						rvn_mm_add_cubic(mm, RVN_GL_X(px, pa), RVN_GL_Y(px, pa), RVN_GL_X(px, mid), RVN_GL_Y(px, mid), RVN_GL_X(n->pos, mid), RVN_GL_Y(n->pos, mid),
							RVN_GL_X(n->pos, in_a), RVN_GL_Y(n->pos, in_a));
						if (out_a > in_a) rvn_mm_add_point(mm, RVN_GL_X(n->pos, out_a), RVN_GL_Y(n->pos, out_a));
					}
					if (n->label && !label_set) {
						edge->label_x = RVN_GL_X(n->pos, center);
						edge->label_y = RVN_GL_Y(n->pos, center);
						label_set = 1;
					}
					px = n->pos;
					pa = out_a;
				}
				if (!label_set) {
					/* in the middle of the path */
					uint32_t mid = start + (mm->point_count - start) / 2u;
					edge->label_x = mm->points[mid * 2u];
					edge->label_y = mm->points[mid * 2u + 1u];
				}
				if (g.reversed[e]) {
					/* drawn from its first node to its second */
					uint32_t a = start, b = mm->point_count - 1u;
					while (a < b) {
						float tx = mm->points[a * 2u], ty = mm->points[a * 2u + 1u];
						mm->points[a * 2u] = mm->points[b * 2u];
						mm->points[a * 2u + 1u] = mm->points[b * 2u + 1u];
						mm->points[b * 2u] = tx;
						mm->points[b * 2u + 1u] = ty;
						a++;
						b--;
					}
				}
			} else {
				continue;
			}
			edge->point_count = mm->point_count - start;
			if (edge->line == RVN_MM_LINE_INVISIBLE || edge->point_count < 2) continue;
			item = rvn_mm_add_item(mm, RVN_MM_ITEM_PATH);
			item->first_point = start;
			item->point_count = edge->point_count;
			item->line = edge->line == RVN_MM_LINE_THICK ? RVN_MM_LINE_SOLID : edge->line;
			item->stroke_width = (edge->line == RVN_MM_LINE_THICK ? 3.0f : 1.4f) * rvn_maxf(k, 0.5f);
			item->stroke = RVN_MM_C_LINE;
			item->head_start = edge->head_start;
			item->head_end = edge->head_end;
		}
		/* the texts of the links, over the lines */
		for (e = 0; e < mm->edge_count; e++) {
			rvn_mm_edge_t* edge = &mm->edges[e];
			const rvn_mm_block_t* block = &blocks[n_real + e];
			rvn_mm_item_t* item;
			if (!edge->label || !edge->label[0] || block->lines == 0 || edge->point_count < 2 || edge->line == RVN_MM_LINE_INVISIBLE) continue;
			item = rvn_mm_add_shape(mm, RVN_MM_RECT, edge->label_x - block->w * 0.5f - 4.0f * k, edge->label_y - block->h * 0.5f - 2.0f * k,
				block->w + 8.0f * k, block->h + 4.0f * k, RVN_MM_C_LABEL, RVN_MM_C_NONE);
			item->stroke_width = 0.0f;
			rvn_mm_item_set_text(mm, item, wrapped[n_real + e].data, -1);
		}

		/* 11. the nodes, over everything */
		for (i = 0; i < n_real; i++) {
			rvn_mm_node_t* node = &mm->nodes[i];
			rvn_mm_item_t* item = rvn_mm_add_shape(mm, node->shape, node->x - node->w * 0.5f, node->y - node->h * 0.5f, node->w, node->h,
				RVN_MM_C_NODE, RVN_MM_C_NODE_BORDER);
			if (node->shape == RVN_MM_NOTE) {
				item->fill = RVN_MM_C_NOTE;
				item->stroke = RVN_MM_C_NOTE_BORDER;
				item->text = RVN_MM_C_NOTE_TEXT;
			} else if (node->shape == RVN_MM_START || node->shape == RVN_MM_BAR) {
				item->fill = RVN_MM_C_LINE;
				item->stroke = RVN_MM_C_NONE;
			} else if (node->shape == RVN_MM_END) {
				item->fill = RVN_MM_C_BACKGROUND;
				item->stroke = RVN_MM_C_LINE;
			}
			rvn_mm_apply_style(item, &node->style);
			if (wrapped[i].data && wrapped[i].data[0]) rvn_mm_item_set_text(mm, item, wrapped[i].data, -1);
		}
#undef RVN_GL_X
#undef RVN_GL_Y
	}

	for (i = 0; i < n_real + mm->edge_count; i++) rvn_free(wrapped[i].data);
	rvn_free(wrapped);
	rvn_free(blocks);
	rvn_free(out_start);
	rvn_free(out_list);
	rvn_free(state);
	rvn_free(stack);
	rvn_free(stack_edge);
	rvn_free(layer);
	rvn_free(topo);
	rvn_free(indegree);
	rvn_free(layer_y);
	rvn_free(layer_thick);
	rvn_free(boxes);
	rvn_free(cbox);
	rvn_free(g.nodes);
	rvn_free(g.edges);
	rvn_free(g.chain_start);
	rvn_free(g.chain_length);
	rvn_free(g.chain);
	rvn_free(g.reversed);
	rvn_free(g.layer_start);
	rvn_free(g.layer_nodes);
	rvn_free(g.up_start);
	rvn_free(g.down_start);
	rvn_free(g.up_list);
	rvn_free(g.down_list);
	rvn_free(g.up_weight);
	rvn_free(g.down_weight);
	rvn_free(g.scratch);
	rvn_free(g.cluster_depth);
	rvn_free(g.cluster_chain);
	rvn_free(g.cluster_sum);
	rvn_free(g.cluster_hits);
}

/* ---------------------------------------------------------------------------
 *  The scene moved and measured
 * ------------------------------------------------------------------------- */

/* Moves every item by (dx, dy). */
static void rvn_mm_translate(rvn_mm_t* mm, uint32_t first_item, uint32_t first_point, float dx, float dy) {
	uint32_t i;
	for (i = first_item; i < mm->item_count; i++) {
		mm->items[i].x += dx;
		mm->items[i].y += dy;
	}
	for (i = first_point; i < mm->point_count; i++) {
		mm->points[i * 2u] += dx;
		mm->points[i * 2u + 1u] += dy;
	}
}

/* The bounds of the items: x0, y0, x1, y1. */
static void rvn_mm_bounds(const rvn_mm_t* mm, float* b) {
	uint32_t i;
	b[0] = b[1] = 1e30f;
	b[2] = b[3] = -1e30f;
	for (i = 0; i < mm->item_count; i++) {
		const rvn_mm_item_t* item = &mm->items[i];
		if (item->kind == RVN_MM_ITEM_PATH) {
			uint32_t p;
			for (p = 0; p < item->point_count; p++) {
				const float* pt = &mm->points[(item->first_point + p) * 2u];
				b[0] = rvn_minf(b[0], pt[0] - 6.0f);
				b[1] = rvn_minf(b[1], pt[1] - 6.0f);
				b[2] = rvn_maxf(b[2], pt[0] + 6.0f);
				b[3] = rvn_maxf(b[3], pt[1] + 6.0f);
			}
		} else if (item->kind == RVN_MM_ITEM_WEDGE) {
			b[0] = rvn_minf(b[0], item->x - item->w);
			b[1] = rvn_minf(b[1], item->y - item->w);
			b[2] = rvn_maxf(b[2], item->x + item->w);
			b[3] = rvn_maxf(b[3], item->y + item->w);
		} else {
			b[0] = rvn_minf(b[0], item->x);
			b[1] = rvn_minf(b[1], item->y);
			b[2] = rvn_maxf(b[2], item->x + item->w);
			b[3] = rvn_maxf(b[3], item->y + item->h);
		}
	}
	if (b[0] > b[2]) b[0] = b[1] = b[2] = b[3] = 0.0f;
}

/* Puts the scene at a margin from the corner and takes its size. */
static void rvn_mm_fit(rvn_mm_t* mm, float margin) {
	float b[4];
	rvn_mm_bounds(mm, b);
	rvn_mm_translate(mm, 0, 0, margin - b[0], margin - b[1]);
	mm->width = b[2] - b[0] + margin * 2.0f;
	mm->height = b[3] - b[1] + margin * 2.0f;
}

/* Moves the items from `from` on so that they come first (drawn under the others). */
static void rvn_mm_move_under(rvn_mm_t* mm, uint32_t at, uint32_t from) {
	uint32_t count = mm->item_count - from;
	rvn_mm_item_t* moved;
	if (count == 0 || from <= at) return;
	moved = (rvn_mm_item_t*)rvn_alloc(sizeof(rvn_mm_item_t) * count);
	memcpy(moved, mm->items + from, sizeof(rvn_mm_item_t) * count);
	memmove(mm->items + at + count, mm->items + at, sizeof(rvn_mm_item_t) * (from - at));
	memcpy(mm->items + at, moved, sizeof(rvn_mm_item_t) * count);
	rvn_free(moved);
}

/* ---------------------------------------------------------------------------
 *  Sequence diagrams
 * ------------------------------------------------------------------------- */

#define RVN_SQ_MAX_DEPTH 8

typedef struct rvn_sq_block_t {
	float top;
	float x0, x1;
	uint32_t first_item;
	const rvn_mm_event_t* event;
	/* the dividers of its sections: their y */
	float dividers[32];
	const rvn_mm_event_t* sections[32];
	int section_count;
} rvn_sq_block_t;

typedef struct rvn_sq_t {
	rvn_mm_t* mm;
	const rvn_mm_measure_t* m;
	float k;
	float* center;
	float* box_w;
	float* box_h;
	/* activations open: their top, for each participant */
	float* active_top;
	int* depth;
	rvn_sq_block_t blocks[64];
	int block_depth;
	rvn_text_t text;
} rvn_sq_t;

/* The participants touched widen the blocks open. */
static void rvn_sq_extend(rvn_sq_t* s, float x0, float x1) {
	int b;
	for (b = 0; b < s->block_depth; b++) {
		s->blocks[b].x0 = rvn_minf(s->blocks[b].x0, x0);
		s->blocks[b].x1 = rvn_maxf(s->blocks[b].x1, x1);
	}
}

/* The x where a message leaves or reaches a participant, on the side `toward` (+1 right): the edge of its activation. */
static float rvn_sq_edge(const rvn_sq_t* s, int p, float toward) {
	int d = s->depth[p];
	float c = s->center[p];
	if (d <= 0) return c;
	d = rvn_mini(d, RVN_SQ_MAX_DEPTH);
	return c + (float)(d - 1) * 5.0f * s->k + toward * 5.0f * s->k;
}

static void rvn_sq_close_activation(rvn_sq_t* s, int p, float y) {
	rvn_mm_item_t* item;
	int d = s->depth[p];
	float top;
	if (d <= 0) return;
	s->depth[p]--;
	if (d > RVN_SQ_MAX_DEPTH) return;
	top = s->active_top[p * RVN_SQ_MAX_DEPTH + d - 1];
	item = rvn_mm_add_shape(s->mm, RVN_MM_RECT, s->center[p] - 5.0f * s->k + (float)(d - 1) * 5.0f * s->k, top, 10.0f * s->k,
		rvn_maxf(y - top, 6.0f * s->k), RVN_MM_C_ACTIVATION, RVN_MM_C_NODE_BORDER);
	(void)item;
}

static void rvn_sq_participant(rvn_sq_t* s, int p, float top, const char* label) {
	rvn_mm_t* mm = s->mm;
	const rvn_mm_node_t* node = &mm->nodes[p];
	float k = s->k, c = s->center[p], w = s->box_w[p], h = s->box_h[p];
	rvn_mm_block_t block = rvn_mm_text_block(s->m, label, 0.0f, R_FALSE, 1.0f, &s->text);
	if (node->shape == RVN_MM_ACTOR) {
		/* a figure, its name under it */
		rvn_mm_item_t* head = rvn_mm_add_shape(mm, RVN_MM_CIRCLE, c - 6.0f * k, top, 12.0f * k, 12.0f * k, RVN_MM_C_NODE, RVN_MM_C_NODE_BORDER);
		head->stroke_width = 1.4f * k;
		rvn_mm_add_line(mm, c, top + 12.0f * k, c, top + 25.0f * k, RVN_MM_LINE_SOLID, RVN_MM_C_NODE_BORDER)->stroke_width = 1.4f * k;
		rvn_mm_add_line(mm, c - 10.0f * k, top + 16.0f * k, c + 10.0f * k, top + 16.0f * k, RVN_MM_LINE_SOLID, RVN_MM_C_NODE_BORDER)->stroke_width = 1.4f * k;
		rvn_mm_add_line(mm, c, top + 25.0f * k, c - 9.0f * k, top + 34.0f * k, RVN_MM_LINE_SOLID, RVN_MM_C_NODE_BORDER)->stroke_width = 1.4f * k;
		rvn_mm_add_line(mm, c, top + 25.0f * k, c + 9.0f * k, top + 34.0f * k, RVN_MM_LINE_SOLID, RVN_MM_C_NODE_BORDER)->stroke_width = 1.4f * k;
		rvn_mm_add_text(mm, s->text.data, c - w * 0.5f, top + 36.0f * k, w, block.h, 0.5f, RVN_MM_C_TEXT);
	} else {
		rvn_mm_item_t* item = rvn_mm_add_shape(mm, RVN_MM_ROUND, c - w * 0.5f, top, w, h, RVN_MM_C_NODE, RVN_MM_C_NODE_BORDER);
		rvn_mm_apply_style(item, &node->style);
		rvn_mm_item_set_text(mm, item, s->text.data, -1);
	}
}

/* An arrow head, and the line of a message. */
static void rvn_sq_message_line(rvn_sq_t* s, float x0, float x1, float y, const rvn_mm_event_t* ev) {
	rvn_mm_item_t* item = rvn_mm_add_line(s->mm, x0, y, x1, y, ev->line, RVN_MM_C_LINE);
	item->stroke_width = 1.4f * s->k;
	item->head_end = ev->head == 2 ? (uint8_t)RVN_MM_HEAD_ARROW : ev->head;
	item->head_start = ev->head == 2 ? (uint8_t)RVN_MM_HEAD_ARROW : (uint8_t)RVN_MM_HEAD_NONE;
}

void rvn_mm_layout_sequence(rvn_mm_t* mm, const rvn_mm_measure_t* m) {
	rvn_sq_t s;
	float k = m->size / 14.0f, margin = 8.0f * k;
	uint32_t n = mm->node_count, i, e;
	float* gaps;
	float* need_left;
	float* need_right;
	float top = 0.0f, y, header_h = 0.0f;
	uint32_t lifeline_first, box_first;
	int numbering = 0, number = 1, step = 1;
	typedef struct { int a, b; float width; } req_t;
	req_t* reqs;
	uint32_t req_count = 0;

	memset(&s, 0, sizeof(s));
	s.mm = mm;
	s.m = m;
	s.k = k;
	if (n == 0) {
		mm->width = mm->height = 0.0f;
		return;
	}
	s.center = (float*)rvn_calloc(sizeof(float) * n);
	s.box_w = (float*)rvn_calloc(sizeof(float) * n);
	s.box_h = (float*)rvn_calloc(sizeof(float) * n);
	s.active_top = (float*)rvn_calloc(sizeof(float) * n * RVN_SQ_MAX_DEPTH);
	s.depth = (int*)rvn_calloc(sizeof(int) * n);
	gaps = (float*)rvn_calloc(sizeof(float) * (n + 1));
	need_left = (float*)rvn_calloc(sizeof(float) * (n + 1));
	need_right = (float*)rvn_calloc(sizeof(float) * (n + 1));
	reqs = (req_t*)rvn_alloc(sizeof(req_t) * (mm->event_count + 1));

	/* the boxes of the participants */
	for (i = 0; i < n; i++) {
		const rvn_mm_node_t* node = &mm->nodes[i];
		rvn_mm_block_t block = rvn_mm_text_block(m, node->label, 0.0f, R_FALSE, 1.0f, &s.text);
		if (node->shape == RVN_MM_ACTOR) {
			s.box_w[i] = rvn_maxf(block.w + 10.0f * k, 50.0f * k);
			s.box_h[i] = 36.0f * k + block.h;
		} else {
			s.box_w[i] = rvn_maxf(block.w + 28.0f * k, 72.0f * k);
			s.box_h[i] = rvn_maxf(block.h + 20.0f * k, 36.0f * k);
		}
		header_h = rvn_maxf(header_h, s.box_h[i]);
	}
	for (i = 0; i + 1 < n; i++) gaps[i] = (s.box_w[i] + s.box_w[i + 1]) * 0.5f + 24.0f * k;

	/* what the messages and the notes need between the columns */
	for (e = 0; e < mm->event_count; e++) {
		const rvn_mm_event_t* ev = &mm->events[e];
		rvn_mm_block_t block;
		if (ev->kind != RVN_MM_EV_MESSAGE && ev->kind != RVN_MM_EV_NOTE) continue;
		if (ev->from < 0 || ev->to < 0) continue;
		block = rvn_mm_text_block(m, ev->text, 0.0f, R_FALSE, 1.0f, &s.text);
		if (ev->kind == RVN_MM_EV_MESSAGE) {
			if (ev->from == ev->to) {
				float width = 36.0f * k + block.w + 10.0f * k;
				if ((uint32_t)ev->from + 1u < n) {
					reqs[req_count].a = ev->from;
					reqs[req_count].b = ev->from + 1;
					reqs[req_count++].width = width + s.box_w[ev->from + 1] * 0.25f;
				} else {
					need_right[ev->from] = rvn_maxf(need_right[ev->from], width);
				}
			} else {
				reqs[req_count].a = rvn_mini(ev->from, ev->to);
				reqs[req_count].b = rvn_maxi(ev->from, ev->to);
				reqs[req_count++].width = block.w + 40.0f * k;
			}
		} else {
			float width = block.w + 20.0f * k + 24.0f * k;
			if (ev->line == RVN_MM_NOTE_RIGHT) {
				if ((uint32_t)ev->from + 1u < n) {
					reqs[req_count].a = ev->from;
					reqs[req_count].b = ev->from + 1;
					reqs[req_count++].width = width;
				} else {
					need_right[ev->from] = rvn_maxf(need_right[ev->from], width);
				}
			} else if (ev->line == RVN_MM_NOTE_LEFT) {
				if (ev->from > 0) {
					reqs[req_count].a = ev->from - 1;
					reqs[req_count].b = ev->from;
					reqs[req_count++].width = width;
				} else {
					need_left[0] = rvn_maxf(need_left[0], width);
				}
			}
		}
	}
	/* the shortest spans first, then the longer ones take what is still missing */
	for (i = 1; i < req_count; i++) {
		req_t v = reqs[i];
		uint32_t j = i;
		while (j > 0 && reqs[j - 1].b - reqs[j - 1].a > v.b - v.a) {
			reqs[j] = reqs[j - 1];
			j--;
		}
		reqs[j] = v;
	}
	for (i = 0; i < req_count; i++) {
		float have = 0.0f;
		int g;
		for (g = reqs[i].a; g < reqs[i].b; g++) have += gaps[g];
		if (have < reqs[i].width) {
			float add = (reqs[i].width - have) / (float)(reqs[i].b - reqs[i].a);
			for (g = reqs[i].a; g < reqs[i].b; g++) gaps[g] += add;
		}
	}
	s.center[0] = need_left[0] + s.box_w[0] * 0.5f;
	for (i = 1; i < n; i++) s.center[i] = s.center[i - 1] + gaps[i - 1];

	/* the title */
	if (mm->title && mm->title[0]) {
		rvn_mm_block_t block = rvn_mm_text_block(m, mm->title, 0.0f, R_TRUE, 1.15f, &s.text);
		float span = s.center[n - 1] - s.center[0];
		rvn_mm_item_t* item = rvn_mm_add_text(mm, s.text.data, s.center[0] - 200.0f * k, 0.0f, span + 400.0f * k, block.h, 0.5f, RVN_MM_C_TEXT);
		item->bold = 1;
		item->text_scale = 1.15f;
		top = block.h + 12.0f * k;
	}

	/* the boxes that group participants, under everything (their height is set at the end) */
	box_first = mm->item_count;
	for (i = 0; i < mm->cluster_count; i++) {
		float x0 = 1e30f, x1 = -1e30f;
		uint32_t p;
		rvn_mm_item_t* item;
		for (p = 0; p < n; p++) {
			if (mm->nodes[p].cluster == (int)i) {
				x0 = rvn_minf(x0, s.center[p] - s.box_w[p] * 0.5f);
				x1 = rvn_maxf(x1, s.center[p] + s.box_w[p] * 0.5f);
			}
		}
		if (x0 > x1) continue;
		item = rvn_mm_add_shape(mm, RVN_MM_RECT, x0 - 10.0f * k, top, x1 - x0 + 20.0f * k, 0.0f, RVN_MM_C_CLUSTER, RVN_MM_C_CLUSTER_BORDER);
		if (rvn_color_a(mm->clusters[i].style.fill)) {
			item->fill = RVN_MM_C_EXPLICIT;
			item->fill_color = mm->clusters[i].style.fill;
		}
		if (mm->clusters[i].title && mm->clusters[i].title[0]) {
			rvn_mm_block_t block = rvn_mm_text_block(m, mm->clusters[i].title, 0.0f, R_TRUE, 1.0f, &s.text);
			rvn_mm_item_t* text = rvn_mm_add_text(mm, s.text.data, x0 - 10.0f * k, top + 4.0f * k, x1 - x0 + 20.0f * k, block.h, 0.5f, RVN_MM_C_TEXT);
			text->bold = 1;
		}
	}
	if (mm->cluster_count) top += m->line_height + 10.0f * k;

	/* the lifelines (their end is set at the end) */
	lifeline_first = mm->item_count;
	for (i = 0; i < n; i++) {
		rvn_mm_item_t* item = rvn_mm_add_line(mm, s.center[i], top + header_h, s.center[i], top + header_h, RVN_MM_LINE_DOTTED, RVN_MM_C_NODE_BORDER);
		item->stroke_width = 1.0f * k;
	}
	for (i = 0; i < n; i++) rvn_sq_participant(&s, (int)i, top + header_h - s.box_h[i], mm->nodes[i].label);
	y = top + header_h + 18.0f * k;

	for (e = 0; e < mm->event_count; e++) {
		const rvn_mm_event_t* ev = &mm->events[e];
		switch (ev->kind) {
			case RVN_MM_EV_AUTONUMBER:
				numbering = ev->from > 0;
				if (numbering) {
					number = ev->from;
					step = ev->to;
				}
				break;
			case RVN_MM_EV_ACTIVATE:
				if (ev->from >= 0) {
					int d = s.depth[ev->from];
					if (d < RVN_SQ_MAX_DEPTH) s.active_top[ev->from * RVN_SQ_MAX_DEPTH + d] = y - 6.0f * k;
					s.depth[ev->from]++;
				}
				break;
			case RVN_MM_EV_DEACTIVATE:
				if (ev->from >= 0) rvn_sq_close_activation(&s, ev->from, y - 6.0f * k);
				break;
			case RVN_MM_EV_MESSAGE: {
				rvn_mm_block_t block;
				float arrow_y;
				if (ev->from < 0 || ev->to < 0) break;
				block = rvn_mm_text_block(m, ev->text, 0.0f, R_FALSE, 1.0f, &s.text);
				if (ev->from == ev->to) {
					float x0 = rvn_sq_edge(&s, ev->from, 1.0f);
					float loop_top = y + block.h + 2.0f * k, loop_h = 18.0f * k;
					rvn_mm_item_t* item;
					if (block.lines && s.text.data[0]) rvn_mm_add_text(mm, s.text.data, x0 + 8.0f * k, y, block.w, block.h, 0.0f, RVN_MM_C_TEXT);
					item = rvn_mm_add_item(mm, RVN_MM_ITEM_PATH);
					item->first_point = rvn_mm_add_point(mm, x0, loop_top);
					rvn_mm_add_point(mm, x0 + 30.0f * k, loop_top);
					rvn_mm_add_point(mm, x0 + 30.0f * k, loop_top + loop_h);
					rvn_mm_add_point(mm, x0, loop_top + loop_h);
					item->point_count = 4;
					item->line = ev->line;
					item->stroke = RVN_MM_C_LINE;
					item->stroke_width = 1.4f * k;
					item->head_end = ev->head == 2 ? (uint8_t)RVN_MM_HEAD_ARROW : ev->head;
					rvn_sq_extend(&s, s.center[ev->from] - 10.0f * k, x0 + rvn_maxf(36.0f * k, block.w + 12.0f * k));
					arrow_y = loop_top;
					if (numbering) {
						char digits[16];
						rvn_mm_item_t* dot = rvn_mm_add_shape(mm, RVN_MM_CIRCLE, x0 - 8.0f * k, arrow_y - 8.0f * k, 16.0f * k, 16.0f * k, RVN_MM_C_ACCENT, RVN_MM_C_NONE);
						snprintf(digits, sizeof(digits), "%d", number);
						dot->text = RVN_MM_C_ACCENT_TEXT;
						dot->text_scale = 0.72f;
						dot->bold = 1;
						rvn_mm_item_set_text(mm, dot, digits, -1);
						number += step;
					}
					if (ev->activation > 0) {
						int d = s.depth[ev->to];
						if (d < RVN_SQ_MAX_DEPTH) s.active_top[ev->to * RVN_SQ_MAX_DEPTH + d] = loop_top + loop_h;
						s.depth[ev->to]++;
					} else if (ev->activation < 0) {
						rvn_sq_close_activation(&s, ev->from, loop_top + loop_h);
					}
					y = loop_top + loop_h + 14.0f * k;
				} else {
					float dir = s.center[ev->to] > s.center[ev->from] ? 1.0f : -1.0f;
					float x0 = rvn_sq_edge(&s, ev->from, dir), x1;
					arrow_y = y + block.h + 5.0f * k;
					if (ev->activation > 0) {
						int d = s.depth[ev->to];
						if (d < RVN_SQ_MAX_DEPTH) s.active_top[ev->to * RVN_SQ_MAX_DEPTH + d] = arrow_y;
						s.depth[ev->to]++;
					}
					x1 = rvn_sq_edge(&s, ev->to, -dir);
					if (block.lines && s.text.data[0]) {
						float mid = (s.center[ev->from] + s.center[ev->to]) * 0.5f;
						rvn_mm_add_text(mm, s.text.data, mid - block.w * 0.5f - 4.0f * k, y, block.w + 8.0f * k, block.h, 0.5f, RVN_MM_C_TEXT);
					}
					rvn_sq_message_line(&s, x0, x1, arrow_y, ev);
					rvn_sq_extend(&s, rvn_minf(s.center[ev->from], s.center[ev->to]), rvn_maxf(s.center[ev->from], s.center[ev->to]));
					if (numbering) {
						char digits[16];
						rvn_mm_item_t* dot = rvn_mm_add_shape(mm, RVN_MM_CIRCLE, x0 - 8.0f * k, arrow_y - 8.0f * k, 16.0f * k, 16.0f * k, RVN_MM_C_ACCENT, RVN_MM_C_NONE);
						snprintf(digits, sizeof(digits), "%d", number);
						dot->text = RVN_MM_C_ACCENT_TEXT;
						dot->text_scale = 0.72f;
						dot->bold = 1;
						rvn_mm_item_set_text(mm, dot, digits, -1);
						number += step;
					}
					if (ev->activation < 0) rvn_sq_close_activation(&s, ev->from, arrow_y);
					y = arrow_y + 16.0f * k;
				}
				break;
			}
			case RVN_MM_EV_NOTE: {
				rvn_mm_block_t block;
				float w, h, x;
				rvn_mm_item_t* item;
				if (ev->from < 0 || ev->to < 0) break;
				block = rvn_mm_text_block(m, ev->text, 0.0f, R_FALSE, 1.0f, &s.text);
				w = block.w + 20.0f * k;
				h = rvn_maxf(block.h, m->line_height) + 14.0f * k;
				if (ev->line == RVN_MM_NOTE_RIGHT) x = rvn_sq_edge(&s, ev->from, 1.0f) + 10.0f * k;
				else if (ev->line == RVN_MM_NOTE_LEFT) x = rvn_sq_edge(&s, ev->from, -1.0f) - 10.0f * k - w;
				else {
					float c0 = rvn_minf(s.center[ev->from], s.center[ev->to]), c1 = rvn_maxf(s.center[ev->from], s.center[ev->to]);
					if (ev->from != ev->to) w = rvn_maxf(w, c1 - c0 + 50.0f * k);
					else w = rvn_maxf(w, 60.0f * k);
					x = (c0 + c1) * 0.5f - w * 0.5f;
				}
				y += 4.0f * k;
				item = rvn_mm_add_shape(mm, RVN_MM_NOTE, x, y, w, h, RVN_MM_C_NOTE, RVN_MM_C_NOTE_BORDER);
				item->text = RVN_MM_C_NOTE_TEXT;
				rvn_mm_item_set_text(mm, item, s.text.data, -1);
				rvn_sq_extend(&s, x, x + w);
				y += h + 12.0f * k;
				break;
			}
			case RVN_MM_EV_BLOCK: {
				rvn_sq_block_t* b;
				rvn_mm_block_t block;
				if (s.block_depth >= (int)RVN_COUNT_OF(s.blocks)) break;
				b = &s.blocks[s.block_depth++];
				memset(b, 0, sizeof(*b));
				b->top = y;
				b->x0 = 1e30f;
				b->x1 = -1e30f;
				b->event = ev;
				b->first_item = mm->item_count;
				block = rvn_mm_text_block(m, ev->text ? ev->text : "", 0.0f, R_FALSE, 1.0f, &s.text);
				y += (strcmp(ev->extra, "rect") == 0 ? 10.0f * k : rvn_maxf(block.h, m->line_height) + 16.0f * k);
				break;
			}
			case RVN_MM_EV_SECTION: {
				rvn_sq_block_t* b;
				rvn_mm_block_t block;
				if (s.block_depth <= 0) break;
				b = &s.blocks[s.block_depth - 1];
				block = rvn_mm_text_block(m, ev->text ? ev->text : "", 0.0f, R_FALSE, 1.0f, &s.text);
				if (b->section_count < 32) {
					b->dividers[b->section_count] = y;
					b->sections[b->section_count++] = ev;
				}
				y += rvn_maxf(block.h, m->line_height) + 12.0f * k;
				break;
			}
			case RVN_MM_EV_END: {
				rvn_sq_block_t* b;
				float x0, x1, pad = 12.0f * k;
				int is_rect, d;
				rvn_mm_item_t* item;
				rvn_mm_block_t block;
				if (s.block_depth <= 0) break;
				b = &s.blocks[--s.block_depth];
				is_rect = strcmp(b->event->extra, "rect") == 0;
				if (b->x0 > b->x1) {
					b->x0 = s.center[0] - 40.0f * k;
					b->x1 = s.center[0] + 40.0f * k;
				}
				x0 = b->x0 - pad;
				x1 = b->x1 + pad;
				y += 4.0f * k;
				if (is_rect) {
					uint32_t at = b->first_item;
					item = rvn_mm_add_shape(mm, RVN_MM_RECT, x0, b->top, x1 - x0, y - b->top, RVN_MM_C_CLUSTER, RVN_MM_C_NONE);
					item->stroke_width = 0.0f;
					if (rvn_color_a(b->event->color)) {
						item->fill = RVN_MM_C_EXPLICIT;
						item->fill_color = b->event->color;
					}
					/* under what it holds */
					rvn_mm_move_under(mm, at, mm->item_count - 1u);
				} else {
					char tag[16];
					float tag_w;
					rvn_mm_item_t* label;
					item = rvn_mm_add_shape(mm, RVN_MM_RECT, x0, b->top, x1 - x0, y - b->top, RVN_MM_C_NONE, RVN_MM_C_BLOCK_BORDER);
					item->stroke_width = 1.2f * k;
					/* the keyword in a tab at the corner, the text beside it */
					snprintf(tag, sizeof(tag), "%s", b->event->extra);
					tag_w = rvn_mm_width(m, tag, (int)strlen(tag), R_TRUE) * 0.92f + 16.0f * k;
					label = rvn_mm_add_shape(mm, RVN_MM_TAG, x0, b->top, tag_w, m->line_height + 6.0f * k, RVN_MM_C_BLOCK_BORDER, RVN_MM_C_NONE);
					label->text = RVN_MM_C_ACCENT_TEXT;
					label->bold = 1;
					label->text_scale = 0.92f;
					label->stroke_width = 0.0f;
					rvn_mm_item_set_text(mm, label, tag, -1);
					if (b->event->text && b->event->text[0]) {
						rvn_text_t bracket = { NULL, 0, 0 };
						rvn_text_printf(&bracket, "[%s]", b->event->text);
						block = rvn_mm_text_block(m, bracket.data, 0.0f, R_TRUE, 1.0f, &s.text);
						rvn_free(bracket.data);
						label = rvn_mm_add_text(mm, s.text.data, x0 + tag_w, b->top + 3.0f * k, rvn_maxf(x1 - x0 - tag_w * 2.0f, block.w), block.h, 0.5f, RVN_MM_C_TEXT);
						label->bold = 1;
					}
					for (d = 0; d < b->section_count; d++) {
						rvn_mm_item_t* line = rvn_mm_add_line(mm, x0, b->dividers[d], x1, b->dividers[d], RVN_MM_LINE_DOTTED, RVN_MM_C_BLOCK_BORDER);
						line->stroke_width = 1.2f * k;
						if (b->sections[d]->text && b->sections[d]->text[0]) {
							rvn_text_t bracket = { NULL, 0, 0 };
							rvn_text_printf(&bracket, "[%s]", b->sections[d]->text);
							block = rvn_mm_text_block(m, bracket.data, 0.0f, R_TRUE, 1.0f, &s.text);
							rvn_free(bracket.data);
							label = rvn_mm_add_text(mm, s.text.data, x0, b->dividers[d] + 4.0f * k, x1 - x0, block.h, 0.5f, RVN_MM_C_TEXT);
							label->bold = 1;
						}
					}
				}
				/* the block open around it holds it */
				if (s.block_depth > 0) rvn_sq_extend(&s, x0 - 2.0f * k, x1 + 2.0f * k);
				y += 10.0f * k;
				break;
			}
			default:
				break;
		}
	}
	y += 8.0f * k;
	/* the activations left open end here */
	for (i = 0; i < n; i++) {
		while (s.depth[i] > 0) rvn_sq_close_activation(&s, (int)i, y);
	}
	/* the participants again at the bottom, the lifelines down to them */
	for (i = 0; i < n; i++) {
		rvn_mm_item_t* line = &mm->items[lifeline_first + i];
		mm->points[(line->first_point + 1u) * 2u + 1u] = y;
		rvn_sq_participant(&s, (int)i, y, mm->nodes[i].label);
		mm->nodes[i].x = s.center[i];
		mm->nodes[i].y = top + header_h - s.box_h[i] * 0.5f;
		mm->nodes[i].w = s.box_w[i];
		mm->nodes[i].h = s.box_h[i];
	}
	for (i = box_first; i < lifeline_first; i++) {
		if (mm->items[i].kind == RVN_MM_ITEM_SHAPE) mm->items[i].h = y + header_h + 10.0f * k - mm->items[i].y;
	}
	rvn_mm_fit(mm, margin);

	rvn_free(s.center);
	rvn_free(s.box_w);
	rvn_free(s.box_h);
	rvn_free(s.active_top);
	rvn_free(s.depth);
	rvn_free(s.text.data);
	rvn_free(gaps);
	rvn_free(need_left);
	rvn_free(need_right);
	rvn_free(reqs);
}

/* ---------------------------------------------------------------------------
 *  Pies
 * ------------------------------------------------------------------------- */

void rvn_mm_layout_pie(rvn_mm_t* mm, const rvn_mm_measure_t* m) {
	float k = m->size / 14.0f, margin = 8.0f * k, r = 105.0f * k;
	float top = 0.0f, cx, cy, angle = 0.0f, legend_x, legend_y, row;
	double total = 0.0;
	uint32_t i;
	rvn_text_t text = { NULL, 0, 0 };
	if (mm->title && mm->title[0]) {
		rvn_mm_block_t block = rvn_mm_text_block(m, mm->title, 0.0f, R_TRUE, 1.15f, &text);
		rvn_mm_item_t* item = rvn_mm_add_text(mm, text.data, 0.0f, 0.0f, r * 2.0f, block.h, 0.5f, RVN_MM_C_TEXT);
		item->bold = 1;
		item->text_scale = 1.15f;
		top = block.h + 14.0f * k;
	}
	cx = r;
	cy = top + r;
	for (i = 0; i < mm->slice_count; i++) total += mm->slices[i].value;
	if (total <= 0.0) {
		rvn_mm_item_t* item = rvn_mm_add_shape(mm, RVN_MM_CIRCLE, cx - r, cy - r, r * 2.0f, r * 2.0f, RVN_MM_C_NONE, RVN_MM_C_NODE_BORDER);
		(void)item;
	} else {
		for (i = 0; i < mm->slice_count; i++) {
			float sweep = (float)(mm->slices[i].value / total) * 6.2831853f;
			rvn_mm_item_t* item;
			if (sweep <= 0.0f) continue;
			item = rvn_mm_add_item(mm, RVN_MM_ITEM_WEDGE);
			item->x = cx;
			item->y = cy;
			item->w = r;
			item->a0 = angle;
			item->a1 = angle + sweep;
			item->fill = (uint8_t)(RVN_MM_C_SLICE + (i % 12u));
			item->stroke = RVN_MM_C_BACKGROUND;
			item->stroke_width = 2.0f * k;
			angle += sweep;
		}
		/* the shares written on the slices big enough */
		angle = 0.0f;
		for (i = 0; i < mm->slice_count; i++) {
			float share = (float)(mm->slices[i].value / total), mid, tx, ty;
			char percent[16];
			rvn_mm_item_t* item;
			mid = angle + share * 3.14159265f;
			angle += share * 6.2831853f;
			if (share < 0.04f) continue;
			snprintf(percent, sizeof(percent), "%.0f%%", (double)share * 100.0);
			tx = cx + sinf(mid) * r * 0.66f;
			ty = cy - cosf(mid) * r * 0.66f;
			item = rvn_mm_add_text(mm, percent, tx - 30.0f * k, ty - m->line_height * 0.5f, 60.0f * k, m->line_height, 0.5f, RVN_MM_C_TEXT);
			item->bold = 1;
			item->text_scale = 0.92f;
		}
	}
	/* the legend */
	row = m->line_height + 8.0f * k;
	legend_x = cx + r + 28.0f * k;
	legend_y = cy - row * (float)mm->slice_count * 0.5f;
	for (i = 0; i < mm->slice_count; i++) {
		rvn_text_t label = { NULL, 0, 0 };
		rvn_mm_block_t block;
		float y = legend_y + row * (float)i;
		rvn_mm_item_t* swatch = rvn_mm_add_shape(mm, RVN_MM_RECT, legend_x, y + (row - 14.0f * k) * 0.5f, 14.0f * k, 14.0f * k,
			(uint8_t)(RVN_MM_C_SLICE + (i % 12u)), RVN_MM_C_NONE);
		swatch->stroke_width = 0.0f;
		if (mm->show_data) rvn_text_printf(&label, "%s [%g]", mm->slices[i].label, mm->slices[i].value);
		else rvn_text_add(&label, mm->slices[i].label);
		block = rvn_mm_text_block(m, label.data, 0.0f, R_FALSE, 1.0f, &text);
		rvn_mm_add_text(mm, text.data, legend_x + 22.0f * k, y + (row - m->line_height) * 0.5f, block.w, block.h, 0.0f, RVN_MM_C_TEXT);
		rvn_free(label.data);
	}
	rvn_free(text.data);
	rvn_mm_fit(mm, margin);
}

/* ---------------------------------------------------------------------------
 *  Errors
 * ------------------------------------------------------------------------- */

void rvn_mm_layout_error(rvn_mm_t* mm, const rvn_mm_measure_t* m) {
	float k = m->size / 14.0f;
	rvn_text_t message = { NULL, 0, 0 };
	rvn_text_t wrapped = { NULL, 0, 0 };
	rvn_mm_block_t block;
	rvn_mm_item_t* item;
	rvn_text_printf(&message, "Syntax error in the Mermaid diagram\nLine %d: %s", mm->error_line, mm->error ? mm->error : "");
	if (mm->error_text && mm->error_text[0]) rvn_text_printf(&message, "\n%d | %s", mm->error_line, mm->error_text);
	block = rvn_mm_text_block(m, message.data, 520.0f * k, R_FALSE, 1.0f, &wrapped);
	item = rvn_mm_add_shape(mm, RVN_MM_ERROR, 0.0f, 0.0f, block.w + 28.0f * k, block.h + 20.0f * k, RVN_MM_C_ERROR, RVN_MM_C_ERROR_BORDER);
	item->text = RVN_MM_C_ERROR_TEXT;
	item->align = 0.0f;
	rvn_mm_item_set_text(mm, item, wrapped.data, -1);
	rvn_free(message.data);
	rvn_free(wrapped.data);
	rvn_mm_fit(mm, 2.0f * k);
}
