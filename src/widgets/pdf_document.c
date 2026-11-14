/**
 * Raven - widgets/pdf_document.c
 * PDF documents written by the application: pages of text, shapes and
 * pictures (an invoice, a report, a snapshot of a window), saved to a file
 * or to memory, shown by a PdfView or printed (pdf_print.c).
 *
 * Nothing outside Raven is needed. The text is set in a TrueType font that
 * is embedded in the file (the Roboto fonts of Raven, or one given by the
 * application) and addressed by glyph (Identity-H), with a ToUnicode map so
 * that the text can be searched and copied. Streams are compressed with a
 * small deflate encoder (fixed Huffman codes, LZ77 over a 32 KB window).
 *
 * The coordinates are in points (1/72 inch), from the top left corner of
 * the page, y going down, like everything else in Raven; they are turned
 * into those of PDF (from the bottom left, y going up) as the page is drawn.
 */

#include "common.h"
#include "../core/handle.h"
#include "../render/font.h"

#include <time.h>

/* ---------------------------------------------------------------------------
 *  A growing buffer of bytes
 * ------------------------------------------------------------------------- */

typedef struct rvn_pdf_buf_t {
	char* data;
	size_t size;
	size_t cap;
} rvn_pdf_buf_t;

static void rvn_pdf_buf_reserve(rvn_pdf_buf_t* buf, size_t extra) {
	size_t cap;
	if (buf->size + extra <= buf->cap) return;
	cap = buf->cap ? buf->cap : 256;
	while (cap < buf->size + extra) cap *= 2;
	buf->data = (char*)rvn_realloc(buf->data, cap);
	buf->cap = cap;
}

static void rvn_pdf_buf_add(rvn_pdf_buf_t* buf, const void* data, size_t size) {
	rvn_pdf_buf_reserve(buf, size);
	memcpy(buf->data + buf->size, data, size);
	buf->size += size;
}

static void rvn_pdf_buf_str(rvn_pdf_buf_t* buf, const char* text) {
	rvn_pdf_buf_add(buf, text, strlen(text));
}

static void rvn_pdf_buf_byte(rvn_pdf_buf_t* buf, int byte) {
	rvn_pdf_buf_reserve(buf, 1);
	buf->data[buf->size++] = (char)byte;
}

static void rvn_pdf_buf_uint(rvn_pdf_buf_t* buf, unsigned long value) {
	char digits[24];
	int count = 0;
	do {
		digits[count++] = (char)('0' + value % 10);
		value /= 10;
	} while (value);
	while (count) rvn_pdf_buf_byte(buf, digits[--count]);
}

/* A number with up to 3 decimals, never in the notation of the locale (printf would write "1,5" in some). */
static void rvn_pdf_buf_num(rvn_pdf_buf_t* buf, double value) {
	unsigned long whole, part;
	double scaled;
	if (!(value == value) || value > 1e9 || value < -1e9) value = 0.0;
	if (value < 0) {
		scaled = -value * 1000.0 + 0.5;
		if (scaled >= 1.0) rvn_pdf_buf_byte(buf, '-');
	} else {
		scaled = value * 1000.0 + 0.5;
	}
	whole = (unsigned long)(scaled / 1000.0);
	part = (unsigned long)scaled - whole * 1000ul;
	rvn_pdf_buf_uint(buf, whole);
	if (part) {
		rvn_pdf_buf_byte(buf, '.');
		rvn_pdf_buf_byte(buf, (int)('0' + part / 100));
		if (part % 100) {
			rvn_pdf_buf_byte(buf, (int)('0' + part / 10 % 10));
			if (part % 10) rvn_pdf_buf_byte(buf, (int)('0' + part % 10));
		}
	}
}

/* Numbers separated by spaces, then an operator: "10 20 m\n". */
static void rvn_pdf_buf_op(rvn_pdf_buf_t* buf, const double* values, int count, const char* op) {
	int i;
	for (i = 0; i < count; i++) {
		rvn_pdf_buf_num(buf, values[i]);
		rvn_pdf_buf_byte(buf, ' ');
	}
	rvn_pdf_buf_str(buf, op);
	rvn_pdf_buf_byte(buf, '\n');
}

static void rvn_pdf_buf_hex4(rvn_pdf_buf_t* buf, unsigned value) {
	static const char hex[] = "0123456789ABCDEF";
	rvn_pdf_buf_byte(buf, hex[(value >> 12) & 15]);
	rvn_pdf_buf_byte(buf, hex[(value >> 8) & 15]);
	rvn_pdf_buf_byte(buf, hex[(value >> 4) & 15]);
	rvn_pdf_buf_byte(buf, hex[value & 15]);
}

/* A text string of the document information: UTF-16BE with a byte order mark, in hexadecimal. */
static void rvn_pdf_buf_text_string(rvn_pdf_buf_t* buf, const char* text) {
	const char* end = text + strlen(text);
	rvn_pdf_buf_str(buf, "<FEFF");
	while (text < end) {
		uint32_t code = rvn_utf8_decode(&text, end);
		if (code >= 0x10000) {
			code -= 0x10000;
			rvn_pdf_buf_hex4(buf, 0xD800 + (code >> 10));
			rvn_pdf_buf_hex4(buf, 0xDC00 + (code & 0x3FF));
		} else {
			rvn_pdf_buf_hex4(buf, code);
		}
	}
	rvn_pdf_buf_byte(buf, '>');
}

/* ---------------------------------------------------------------------------
 *  Deflate (RFC 1950 / 1951): one block of fixed Huffman codes
 * ------------------------------------------------------------------------- */

typedef struct rvn_deflate_t {
	rvn_pdf_buf_t* out;
	uint32_t bits;
	int count;
} rvn_deflate_t;

static void rvn_deflate_bits(rvn_deflate_t* z, uint32_t value, int count) {
	z->bits |= value << z->count;
	z->count += count;
	while (z->count >= 8) {
		rvn_pdf_buf_byte(z->out, (int)(z->bits & 0xFF));
		z->bits >>= 8;
		z->count -= 8;
	}
}

/* Huffman codes are sent from their first bit: reversed into the stream of the least significant bit first. */
static void rvn_deflate_code(rvn_deflate_t* z, uint32_t code, int length) {
	uint32_t reversed = 0;
	int i;
	for (i = 0; i < length; i++) reversed |= ((code >> i) & 1u) << (length - 1 - i);
	rvn_deflate_bits(z, reversed, length);
}

static void rvn_deflate_symbol(rvn_deflate_t* z, int symbol) {
	if (symbol < 144) rvn_deflate_code(z, 0x30u + (uint32_t)symbol, 8);
	else if (symbol < 256) rvn_deflate_code(z, 0x190u + (uint32_t)(symbol - 144), 9);
	else if (symbol < 280) rvn_deflate_code(z, (uint32_t)(symbol - 256), 7);
	else rvn_deflate_code(z, 0xC0u + (uint32_t)(symbol - 280), 8);
}

static const unsigned short rvn_deflate_len_base[29] = {
	3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
	35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};
static const unsigned char rvn_deflate_len_extra[29] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};
static const unsigned short rvn_deflate_dist_base[30] = {
	1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
	257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};
static const unsigned char rvn_deflate_dist_extra[30] = {
	0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

static void rvn_deflate_match(rvn_deflate_t* z, int length, int distance) {
	int i = 28;
	while (rvn_deflate_len_base[i] > length) i--;
	rvn_deflate_symbol(z, 257 + i);
	if (rvn_deflate_len_extra[i]) rvn_deflate_bits(z, (uint32_t)(length - rvn_deflate_len_base[i]), rvn_deflate_len_extra[i]);
	i = 29;
	while (rvn_deflate_dist_base[i] > distance) i--;
	rvn_deflate_code(z, (uint32_t)i, 5);
	if (rvn_deflate_dist_extra[i]) rvn_deflate_bits(z, (uint32_t)(distance - rvn_deflate_dist_base[i]), rvn_deflate_dist_extra[i]);
}

#define RVN_DEFLATE_WINDOW 32768
#define RVN_DEFLATE_HASH 65536
#define RVN_DEFLATE_CHAIN 48

static uint32_t rvn_deflate_hash(const uint8_t* p) {
	return (((uint32_t)p[0] << 16 | (uint32_t)p[1] << 8 | p[2]) * 2654435761u) >> 16;
}

/* Appends `data` compressed as a zlib stream. */
static void rvn_deflate(rvn_pdf_buf_t* out, const uint8_t* data, size_t size) {
	rvn_deflate_t z;
	int32_t* head = (int32_t*)rvn_alloc(sizeof(int32_t) * RVN_DEFLATE_HASH);
	int32_t* prev = (int32_t*)rvn_alloc(sizeof(int32_t) * RVN_DEFLATE_WINDOW);
	uint32_t a = 1, b = 0;
	size_t i, pos = 0;

	z.out = out;
	z.bits = 0;
	z.count = 0;
	rvn_pdf_buf_byte(out, 0x78);
	rvn_pdf_buf_byte(out, 0x01);
	for (i = 0; i < RVN_DEFLATE_HASH; i++) head[i] = -1;
	rvn_deflate_bits(&z, 1, 1); /* last block */
	rvn_deflate_bits(&z, 1, 2); /* fixed codes */

	while (pos < size) {
		int best = 0, best_dist = 0;
		if (pos + 3 <= size) {
			uint32_t h = rvn_deflate_hash(data + pos);
			int32_t candidate = head[h];
			int chain = RVN_DEFLATE_CHAIN;
			size_t limit = size - pos < 258 ? size - pos : 258;
			while (candidate >= 0 && pos - (size_t)candidate <= RVN_DEFLATE_WINDOW && chain-- > 0) {
				const uint8_t* p = data + candidate;
				const uint8_t* q = data + pos;
				size_t length = 0;
				while (length < limit && p[length] == q[length]) length++;
				if ((int)length > best) {
					best = (int)length;
					best_dist = (int)(pos - (size_t)candidate);
					if (length == limit) break;
				}
				candidate = prev[candidate % RVN_DEFLATE_WINDOW];
			}
		}
		if (best >= 3) {
			size_t end = pos + (size_t)best;
			rvn_deflate_match(&z, best, best_dist);
			for (; pos < end; pos++) {
				if (pos + 3 <= size) {
					uint32_t h = rvn_deflate_hash(data + pos);
					prev[pos % RVN_DEFLATE_WINDOW] = head[h];
					head[h] = (int32_t)pos;
				}
			}
		} else {
			if (pos + 3 <= size) {
				uint32_t h = rvn_deflate_hash(data + pos);
				prev[pos % RVN_DEFLATE_WINDOW] = head[h];
				head[h] = (int32_t)pos;
			}
			rvn_deflate_symbol(&z, data[pos]);
			pos++;
		}
	}
	rvn_deflate_symbol(&z, 256);
	if (z.count) rvn_deflate_bits(&z, 0, 8 - z.count);

	for (i = 0; i < size; i++) {
		a = (a + data[i]) % 65521u;
		b = (b + a) % 65521u;
	}
	rvn_pdf_buf_byte(out, (int)(b >> 8));
	rvn_pdf_buf_byte(out, (int)(b & 0xFF));
	rvn_pdf_buf_byte(out, (int)(a >> 8));
	rvn_pdf_buf_byte(out, (int)(a & 0xFF));
	rvn_free(head);
	rvn_free(prev);
}

/* ---------------------------------------------------------------------------
 *  The document
 * ------------------------------------------------------------------------- */

typedef struct rvn_pdf_font_t {
	uint8_t* data;      /* owned copy for the fonts of the application, else the one of Raven */
	size_t size;
	int owned;
	rvn_ttf_t* ttf;
	int glyph_count;
	float units_per_em;
	float bbox[4];      /* in thousandths of the em */
	float ascent, descent, line_gap;
	/** Code point of each glyph used (0: not used): the widths and the ToUnicode map of the file. */
	uint32_t* used;
	/** R_TRUE once text was set in it: only those fonts go in the file. */
	int drawn;
} rvn_pdf_font_t;

typedef struct rvn_pdf_image_t {
	rvn_pdf_buf_t color;   /* RGB, compressed */
	rvn_pdf_buf_t alpha;   /* gray, compressed; empty when every pixel is opaque */
	int width, height;
} rvn_pdf_image_t;

typedef struct rvn_pdf_page_out_t {
	float width, height;
	rvn_pdf_buf_t content;
} rvn_pdf_page_out_t;

typedef struct rvn_pdf_doc_t {
	Rid id;
	char* title;
	char* author;
	char* subject;

	rvn_pdf_page_out_t* pages;
	int page_count, page_cap;

	rvn_pdf_font_t* fonts;
	int font_count, font_cap;

	rvn_pdf_image_t* images;
	int image_count, image_cap;

	/** Opacities used (ExtGState /GsN), in 1/255. */
	uint8_t alphas[256];
	int alpha_count;

	/* state of drawing */
	float fill[4], stroke[4];
	float line_width;
	int font;
	float font_size;
	Ruint text_style;
} rvn_pdf_doc_t;

static struct {
	rvn_pool_t pool;
	int ready;
} g_pdf_docs;

static rvn_pdf_doc_t* rvn_pdf_doc_get(Rid id, const char* function) {
	rvn_pdf_doc_t* doc = g_pdf_docs.ready ? (rvn_pdf_doc_t*)rvn_pool_get(&g_pdf_docs.pool, id) : NULL;
	if (!doc) rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "%s: not a PDF document", function);
	return doc;
}

#define RVN_PDF_DOC(result) \
	rvn_pdf_doc_t* doc; \
	RVN_API_GUARD(result); \
	doc = rvn_pdf_doc_get(id, __func__); \
	if (!doc) return result

#define RVN_PDF_DOC_VOID() \
	rvn_pdf_doc_t* doc; \
	RVN_API_GUARD_VOID(); \
	doc = rvn_pdf_doc_get(id, __func__); \
	if (!doc) return

/* --- TrueType tables ------------------------------------------------------ */

static uint32_t rvn_pdf_be32(const uint8_t* p) {
	return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static int rvn_pdf_be16(const uint8_t* p) {
	return p[0] << 8 | p[1];
}
static int rvn_pdf_bes16(const uint8_t* p) {
	int v = rvn_pdf_be16(p);
	return v >= 0x8000 ? v - 0x10000 : v;
}

/* Offset of a table of the font, 0 when it has none (or the directory lies). */
static size_t rvn_pdf_ttf_table(const uint8_t* data, size_t size, const char* tag, size_t min_size) {
	int count, i;
	if (size < 12) return 0;
	count = rvn_pdf_be16(data + 4);
	if (12 + (size_t)count * 16 > size) return 0;
	for (i = 0; i < count; i++) {
		const uint8_t* entry = data + 12 + i * 16;
		if (memcmp(entry, tag, 4) == 0) {
			size_t offset = rvn_pdf_be32(entry + 8);
			size_t length = rvn_pdf_be32(entry + 12);
			if (offset > size || length > size - offset || length < min_size) return 0;
			return offset;
		}
	}
	return 0;
}

/* Reads what the file needs of a TrueType font. R_FALSE: not one (an OpenType font of CFF outlines, a collection...). */
static int rvn_pdf_font_init(rvn_pdf_font_t* font, uint8_t* data, size_t size, int owned) {
	size_t head, maxp;
	uint32_t version;
	memset(font, 0, sizeof(*font));
	if (size < 12) return R_FALSE;
	version = rvn_pdf_be32(data);
	if (version != 0x00010000u && version != 0x74727565u /* 'true' */) return R_FALSE;
	head = rvn_pdf_ttf_table(data, size, "head", 54);
	maxp = rvn_pdf_ttf_table(data, size, "maxp", 6);
	if (!head || !maxp || !rvn_pdf_ttf_table(data, size, "glyf", 1)) return R_FALSE;
	font->ttf = rvn_ttf_open(data, size);
	if (!font->ttf) return R_FALSE;
	font->data = data;
	font->size = size;
	font->owned = owned;
	font->units_per_em = (float)rvn_pdf_be16(data + head + 18);
	if (font->units_per_em < 16.0f) font->units_per_em = 1000.0f;
	font->bbox[0] = (float)rvn_pdf_bes16(data + head + 36) * 1000.0f / font->units_per_em;
	font->bbox[1] = (float)rvn_pdf_bes16(data + head + 38) * 1000.0f / font->units_per_em;
	font->bbox[2] = (float)rvn_pdf_bes16(data + head + 40) * 1000.0f / font->units_per_em;
	font->bbox[3] = (float)rvn_pdf_bes16(data + head + 42) * 1000.0f / font->units_per_em;
	font->glyph_count = rvn_pdf_be16(data + maxp + 4);
	rvn_ttf_vmetrics(font->ttf, &font->ascent, &font->descent, &font->line_gap);
	font->used = (uint32_t*)rvn_calloc(sizeof(uint32_t) * (size_t)(font->glyph_count > 0 ? font->glyph_count : 1));
	return R_TRUE;
}

static void rvn_pdf_font_free(rvn_pdf_font_t* font) {
	if (font->ttf) rvn_ttf_close(font->ttf);
	if (font->owned) rvn_free(font->data);
	rvn_free(font->used);
}

static int rvn_pdf_add_font(rvn_pdf_doc_t* doc, uint8_t* data, size_t size, int owned) {
	rvn_pdf_font_t font;
	if (!rvn_pdf_font_init(&font, data, size, owned)) return -1;
	if (doc->font_count == doc->font_cap) {
		doc->font_cap = doc->font_cap ? doc->font_cap * 2 : 4;
		doc->fonts = (rvn_pdf_font_t*)rvn_realloc(doc->fonts, sizeof(rvn_pdf_font_t) * (size_t)doc->font_cap);
	}
	doc->fonts[doc->font_count] = font;
	return doc->font_count++;
}

/* --- Creation ------------------------------------------------------------ */

Rid Rpdf_document_create(void) {
	rvn_pdf_doc_t* doc;
	RVN_API_GUARD(R_INVALID_ID);
	if (!g_pdf_docs.ready) {
		rvn_pool_init(&g_pdf_docs.pool, RVN_HANDLE_PDF_DOCUMENT);
		g_pdf_docs.ready = R_TRUE;
	}
	doc = (rvn_pdf_doc_t*)rvn_calloc(sizeof(rvn_pdf_doc_t));
	/* the fonts of Raven are R_PDF_FONT_SANS (0) and R_PDF_FONT_MONO (1) */
	rvn_pdf_add_font(doc, (uint8_t*)rvn_default_font_data, rvn_default_font_size, R_FALSE);
	rvn_pdf_add_font(doc, (uint8_t*)rvn_mono_font_data, rvn_mono_font_size, R_FALSE);
	doc->fill[3] = 1.0f;
	doc->stroke[3] = 1.0f;
	doc->line_width = 1.0f;
	doc->font = R_PDF_FONT_SANS;
	doc->font_size = 12.0f;
	doc->id = rvn_pool_add(&g_pdf_docs.pool, doc);
	if (doc->id == R_INVALID_ID) {
		int i;
		for (i = 0; i < doc->font_count; i++) rvn_pdf_font_free(&doc->fonts[i]);
		rvn_free(doc->fonts);
		rvn_free(doc);
		return R_INVALID_ID;
	}
	return doc->id;
}

static void rvn_pdf_doc_free(rvn_pdf_doc_t* doc) {
	int i;
	for (i = 0; i < doc->page_count; i++) rvn_free(doc->pages[i].content.data);
	for (i = 0; i < doc->font_count; i++) rvn_pdf_font_free(&doc->fonts[i]);
	for (i = 0; i < doc->image_count; i++) {
		rvn_free(doc->images[i].color.data);
		rvn_free(doc->images[i].alpha.data);
	}
	rvn_free(doc->pages);
	rvn_free(doc->fonts);
	rvn_free(doc->images);
	rvn_free(doc->title);
	rvn_free(doc->author);
	rvn_free(doc->subject);
	rvn_free(doc);
}

void Rpdf_document_delete(Rid id) {
	RVN_PDF_DOC_VOID();
	rvn_pool_remove(&g_pdf_docs.pool, id);
	rvn_pdf_doc_free(doc);
}

/** Rshutdown: the documents the application did not delete. */
void rvn_pdf_documents_shutdown(void) {
	uint32_t i;
	if (!g_pdf_docs.ready) return;
	for (i = 0; i < g_pdf_docs.pool.used; i++) {
		rvn_pdf_doc_t* doc = (rvn_pdf_doc_t*)rvn_pool_at(&g_pdf_docs.pool, i);
		if (doc) rvn_pdf_doc_free(doc);
	}
	rvn_pool_free(&g_pdf_docs.pool);
	g_pdf_docs.ready = R_FALSE;
}

static void rvn_pdf_set_string(char** field, const char* value) {
	rvn_free(*field);
	*field = (value && value[0]) ? rvn_strdup(value) : NULL;
}

void Rpdf_document_set_title(Rid id, const char* title) {
	RVN_PDF_DOC_VOID();
	rvn_pdf_set_string(&doc->title, title);
}

void Rpdf_document_set_author(Rid id, const char* author) {
	RVN_PDF_DOC_VOID();
	rvn_pdf_set_string(&doc->author, author);
}

void Rpdf_document_set_subject(Rid id, const char* subject) {
	RVN_PDF_DOC_VOID();
	rvn_pdf_set_string(&doc->subject, subject);
}

int Rpdf_document_add_font(Rid id, const char* data, size_t data_size) {
	uint8_t* copy;
	int index;
	RVN_PDF_DOC(-1);
	if (!data || data_size < 12) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rpdf_document_add_font: no font data");
		return -1;
	}
	copy = (uint8_t*)rvn_alloc(data_size);
	memcpy(copy, data, data_size);
	index = rvn_pdf_add_font(doc, copy, data_size, R_TRUE);
	if (index < 0) {
		rvn_free(copy);
		rvn_error(R_ERROR_INVALID_FILE_FORMAT, R_SEVERITY_ERROR, "Rpdf_document_add_font: not a TrueType font (.ttf)");
	}
	return index;
}

int Rpdf_document_add_font_from_file(Rid id, const char* path) {
	size_t size = 0;
	char* data;
	int index;
	RVN_PDF_DOC(-1);
	data = path ? Rfile_read(path, &size) : NULL;
	if (!data) {
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rpdf_document_add_font_from_file: cannot read '%s'", path ? path : "");
		return -1;
	}
	index = Rpdf_document_add_font(id, data, size);
	Rmemory_free(data);
	return index;
}

/* --- Pages ---------------------------------------------------------------- */

int Rpdf_document_add_page(Rid id, float width, float height) {
	rvn_pdf_page_out_t* page;
	RVN_PDF_DOC(-1);
	if (!(width > 0.0f) || !(height > 0.0f) || width > 14400.0f || height > 14400.0f) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rpdf_document_add_page: the size must be between 1 and 14400 points");
		return -1;
	}
	if (doc->page_count == doc->page_cap) {
		doc->page_cap = doc->page_cap ? doc->page_cap * 2 : 8;
		doc->pages = (rvn_pdf_page_out_t*)rvn_realloc(doc->pages, sizeof(rvn_pdf_page_out_t) * (size_t)doc->page_cap);
	}
	page = &doc->pages[doc->page_count];
	memset(page, 0, sizeof(*page));
	page->width = width;
	page->height = height;
	return doc->page_count++;
}

int Rpdf_document_get_page_count(Rid id) {
	RVN_PDF_DOC(0);
	return doc->page_count;
}

/* The page drawn on: the last one added, an A4 page when there is none yet. */
static rvn_pdf_page_out_t* rvn_pdf_current_page(rvn_pdf_doc_t* doc) {
	if (doc->page_count == 0) Rpdf_document_add_page(doc->id, 595.276f, 841.89f);
	return doc->page_count ? &doc->pages[doc->page_count - 1] : NULL;
}

/* --- State ---------------------------------------------------------------- */

static float rvn_pdf_unit_clamp(float value) {
	if (!(value > 0.0f)) return 0.0f;
	return value > 1.0f ? 1.0f : value;
}

void Rpdf_document_set_fill_color(Rid id, float r, float g, float b, float a) {
	RVN_PDF_DOC_VOID();
	doc->fill[0] = rvn_pdf_unit_clamp(r);
	doc->fill[1] = rvn_pdf_unit_clamp(g);
	doc->fill[2] = rvn_pdf_unit_clamp(b);
	doc->fill[3] = rvn_pdf_unit_clamp(a);
}

void Rpdf_document_set_stroke_color(Rid id, float r, float g, float b, float a) {
	RVN_PDF_DOC_VOID();
	doc->stroke[0] = rvn_pdf_unit_clamp(r);
	doc->stroke[1] = rvn_pdf_unit_clamp(g);
	doc->stroke[2] = rvn_pdf_unit_clamp(b);
	doc->stroke[3] = rvn_pdf_unit_clamp(a);
}

void Rpdf_document_set_line_width(Rid id, float width) {
	RVN_PDF_DOC_VOID();
	doc->line_width = width > 0.0f ? width : 0.0f;
}

void Rpdf_document_set_font(Rid id, int font, float size, Ruint style) {
	RVN_PDF_DOC_VOID();
	if (font < 0 || font >= doc->font_count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rpdf_document_set_font: no font %d in the document", font);
		return;
	}
	if (!(size > 0.0f) || size > 2000.0f) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rpdf_document_set_font: the size must be between 0 and 2000 points");
		return;
	}
	doc->font = font;
	doc->font_size = size;
	doc->text_style = style;
}

float Rpdf_document_get_line_height(Rid id) {
	const rvn_pdf_font_t* font;
	RVN_PDF_DOC(0.0f);
	font = &doc->fonts[doc->font];
	return (font->ascent + font->descent + font->line_gap) * doc->font_size;
}

/* Name of the graphics state of an opacity, added to the document the first time. */
static int rvn_pdf_alpha_state(rvn_pdf_doc_t* doc, float alpha) {
	int value = (int)(alpha * 255.0f + 0.5f), i;
	for (i = 0; i < doc->alpha_count; i++) {
		if (doc->alphas[i] == value) return i;
	}
	doc->alphas[doc->alpha_count] = (uint8_t)value;
	return doc->alpha_count++;
}

/* Emits the colors (and the opacity) of a path or a text: inside q ... Q. */
static void rvn_pdf_emit_paint(rvn_pdf_doc_t* doc, rvn_pdf_buf_t* out, int fill, int stroke) {
	double values[3];
	if (fill) {
		values[0] = doc->fill[0];
		values[1] = doc->fill[1];
		values[2] = doc->fill[2];
		rvn_pdf_buf_op(out, values, 3, "rg");
	}
	if (stroke) {
		values[0] = doc->stroke[0];
		values[1] = doc->stroke[1];
		values[2] = doc->stroke[2];
		rvn_pdf_buf_op(out, values, 3, "RG");
		values[0] = doc->line_width;
		rvn_pdf_buf_op(out, values, 1, "w");
	}
	if ((fill && doc->fill[3] < 1.0f) || (stroke && doc->stroke[3] < 1.0f)) {
		/* one state for both: the opacity of what is painted */
		float alpha = fill ? doc->fill[3] : doc->stroke[3];
		rvn_pdf_buf_str(out, "/Gs");
		rvn_pdf_buf_uint(out, (unsigned long)rvn_pdf_alpha_state(doc, alpha));
		rvn_pdf_buf_str(out, " gs\n");
	}
}

/* --- Shapes --------------------------------------------------------------- */

static int rvn_pdf_paint_ok(rvn_pdf_doc_t* doc, Ruint paint) {
	int fill = (paint & R_PDF_FILL) && doc->fill[3] > 0.0f;
	int stroke = (paint & R_PDF_STROKE) && doc->stroke[3] > 0.0f && doc->line_width > 0.0f;
	return fill || stroke;
}

static void rvn_pdf_finish_path(rvn_pdf_doc_t* doc, rvn_pdf_buf_t* out, Ruint paint) {
	int fill = (paint & R_PDF_FILL) && doc->fill[3] > 0.0f;
	int stroke = (paint & R_PDF_STROKE) && doc->stroke[3] > 0.0f && doc->line_width > 0.0f;
	/* a stroke of its own opacity cannot share the state of the fill: drawn twice */
	if (fill && stroke && doc->fill[3] != doc->stroke[3]) {
		rvn_pdf_buf_str(out, "f\nQ\n");
		return;
	}
	rvn_pdf_buf_str(out, fill && stroke ? "B\nQ\n" : fill ? "f\nQ\n" : "S\nQ\n");
}

/* A rectangle with rounded corners as a path (PDF coordinates). */
static void rvn_pdf_rect_path(rvn_pdf_buf_t* out, double x, double y, double w, double h, double r) {
	double v[6];
	if (r <= 0.0) {
		v[0] = x; v[1] = y; v[2] = w; v[3] = h;
		rvn_pdf_buf_op(out, v, 4, "re");
		return;
	}
	if (r > w / 2) r = w / 2;
	if (r > h / 2) r = h / 2;
	{
		const double k = 0.5523 * r;
		v[0] = x + r; v[1] = y;
		rvn_pdf_buf_op(out, v, 2, "m");
		v[0] = x + w - r; v[1] = y;
		rvn_pdf_buf_op(out, v, 2, "l");
		v[0] = x + w - r + k; v[1] = y; v[2] = x + w; v[3] = y + r - k; v[4] = x + w; v[5] = y + r;
		rvn_pdf_buf_op(out, v, 6, "c");
		v[0] = x + w; v[1] = y + h - r;
		rvn_pdf_buf_op(out, v, 2, "l");
		v[0] = x + w; v[1] = y + h - r + k; v[2] = x + w - r + k; v[3] = y + h; v[4] = x + w - r; v[5] = y + h;
		rvn_pdf_buf_op(out, v, 6, "c");
		v[0] = x + r; v[1] = y + h;
		rvn_pdf_buf_op(out, v, 2, "l");
		v[0] = x + r - k; v[1] = y + h; v[2] = x; v[3] = y + h - r + k; v[4] = x; v[5] = y + h - r;
		rvn_pdf_buf_op(out, v, 6, "c");
		v[0] = x; v[1] = y + r;
		rvn_pdf_buf_op(out, v, 2, "l");
		v[0] = x; v[1] = y + r - k; v[2] = x + r - k; v[3] = y; v[4] = x + r; v[5] = y;
		rvn_pdf_buf_op(out, v, 6, "c");
		rvn_pdf_buf_str(out, "h\n");
	}
}

static int rvn_pdf_finite(float v) {
	return v == v && v < 1e7f && v > -1e7f;
}

void Rpdf_document_draw_rect(Rid id, float x, float y, float width, float height, float radius, Ruint paint) {
	rvn_pdf_page_out_t* page;
	RVN_PDF_DOC_VOID();
	if (!rvn_pdf_finite(x) || !rvn_pdf_finite(y) || !rvn_pdf_finite(width) || !rvn_pdf_finite(height) || !rvn_pdf_finite(radius)) return;
	if (width <= 0.0f || height <= 0.0f || !rvn_pdf_paint_ok(doc, paint)) return;
	page = rvn_pdf_current_page(doc);
	if (!page) return;
	rvn_pdf_buf_str(&page->content, "q\n");
	rvn_pdf_emit_paint(doc, &page->content, (paint & R_PDF_FILL) != 0, (paint & R_PDF_STROKE) != 0);
	rvn_pdf_rect_path(&page->content, x, page->height - y - height, width, height, radius);
	rvn_pdf_finish_path(doc, &page->content, paint);
	if ((paint & R_PDF_FILL) && (paint & R_PDF_STROKE) && doc->fill[3] > 0.0f && doc->stroke[3] > 0.0f &&
		doc->line_width > 0.0f && doc->fill[3] != doc->stroke[3]) {
		Rpdf_document_draw_rect(id, x, y, width, height, radius, R_PDF_STROKE);
	}
}

void Rpdf_document_draw_ellipse(Rid id, float x, float y, float width, float height, Ruint paint) {
	rvn_pdf_page_out_t* page;
	double cx, cy, rx, ry, kx, ky, v[6];
	RVN_PDF_DOC_VOID();
	if (!rvn_pdf_finite(x) || !rvn_pdf_finite(y) || !rvn_pdf_finite(width) || !rvn_pdf_finite(height)) return;
	if (width <= 0.0f || height <= 0.0f || !rvn_pdf_paint_ok(doc, paint)) return;
	page = rvn_pdf_current_page(doc);
	if (!page) return;
	rx = width / 2.0;
	ry = height / 2.0;
	cx = x + rx;
	cy = page->height - y - ry;
	kx = 0.5523 * rx;
	ky = 0.5523 * ry;
	rvn_pdf_buf_str(&page->content, "q\n");
	rvn_pdf_emit_paint(doc, &page->content, (paint & R_PDF_FILL) != 0, (paint & R_PDF_STROKE) != 0);
	v[0] = cx + rx; v[1] = cy;
	rvn_pdf_buf_op(&page->content, v, 2, "m");
	v[0] = cx + rx; v[1] = cy + ky; v[2] = cx + kx; v[3] = cy + ry; v[4] = cx; v[5] = cy + ry;
	rvn_pdf_buf_op(&page->content, v, 6, "c");
	v[0] = cx - kx; v[1] = cy + ry; v[2] = cx - rx; v[3] = cy + ky; v[4] = cx - rx; v[5] = cy;
	rvn_pdf_buf_op(&page->content, v, 6, "c");
	v[0] = cx - rx; v[1] = cy - ky; v[2] = cx - kx; v[3] = cy - ry; v[4] = cx; v[5] = cy - ry;
	rvn_pdf_buf_op(&page->content, v, 6, "c");
	v[0] = cx + kx; v[1] = cy - ry; v[2] = cx + rx; v[3] = cy - ky; v[4] = cx + rx; v[5] = cy;
	rvn_pdf_buf_op(&page->content, v, 6, "c");
	rvn_pdf_buf_str(&page->content, "h\n");
	rvn_pdf_finish_path(doc, &page->content, paint);
	if ((paint & R_PDF_FILL) && (paint & R_PDF_STROKE) && doc->fill[3] > 0.0f && doc->stroke[3] > 0.0f &&
		doc->line_width > 0.0f && doc->fill[3] != doc->stroke[3]) {
		Rpdf_document_draw_ellipse(id, x, y, width, height, R_PDF_STROKE);
	}
}

void Rpdf_document_draw_line(Rid id, float x1, float y1, float x2, float y2) {
	rvn_pdf_page_out_t* page;
	double v[2];
	RVN_PDF_DOC_VOID();
	if (!rvn_pdf_finite(x1) || !rvn_pdf_finite(y1) || !rvn_pdf_finite(x2) || !rvn_pdf_finite(y2)) return;
	if (!rvn_pdf_paint_ok(doc, R_PDF_STROKE)) return;
	page = rvn_pdf_current_page(doc);
	if (!page) return;
	rvn_pdf_buf_str(&page->content, "q\n");
	rvn_pdf_emit_paint(doc, &page->content, R_FALSE, R_TRUE);
	v[0] = x1; v[1] = page->height - y1;
	rvn_pdf_buf_op(&page->content, v, 2, "m");
	v[0] = x2; v[1] = page->height - y2;
	rvn_pdf_buf_op(&page->content, v, 2, "l");
	rvn_pdf_buf_str(&page->content, "S\nQ\n");
}

/* --- Text ----------------------------------------------------------------- */

/* Width of `length` bytes of UTF-8 in the current font, in points. */
static float rvn_pdf_measure(rvn_pdf_doc_t* doc, const char* text, size_t length) {
	rvn_pdf_font_t* font = &doc->fonts[doc->font];
	const char* end = text + length;
	float width = 0.0f;
	while (text < end) {
		uint32_t code = rvn_utf8_decode(&text, end);
		if (code == '\n' || code == '\r') continue;
		width += rvn_ttf_advance(font->ttf, rvn_ttf_glyph_index(font->ttf, code));
	}
	width *= doc->font_size;
	if (doc->text_style & R_PDF_TEXT_BOLD) width += doc->font_size * 0.04f;
	return width;
}

float Rpdf_document_measure_text(Rid id, const char* text) {
	RVN_PDF_DOC(0.0f);
	return text ? rvn_pdf_measure(doc, text, strlen(text)) : 0.0f;
}

/* One line of text with the top of its box at y. */
static float rvn_pdf_text_line(rvn_pdf_doc_t* doc, float x, float y, const char* text, size_t length) {
	rvn_pdf_page_out_t* page = rvn_pdf_current_page(doc);
	rvn_pdf_font_t* font = &doc->fonts[doc->font];
	const char* p = text;
	const char* end = text + length;
	float width, baseline;
	double v[6];
	int bold = (doc->text_style & R_PDF_TEXT_BOLD) != 0;

	if (!page || length == 0) return 0.0f;
	width = rvn_pdf_measure(doc, text, length);
	if (doc->fill[3] <= 0.0f) return width;
	baseline = page->height - y - font->ascent * doc->font_size;

	rvn_pdf_buf_str(&page->content, "q\n");
	rvn_pdf_emit_paint(doc, &page->content, R_TRUE, R_FALSE);
	if (bold) {
		/* thicker strokes: the outline stroked in the color of the text */
		v[0] = doc->fill[0];
		v[1] = doc->fill[1];
		v[2] = doc->fill[2];
		rvn_pdf_buf_op(&page->content, v, 3, "RG");
		v[0] = doc->font_size * 0.04;
		rvn_pdf_buf_op(&page->content, v, 1, "w");
	}
	font->drawn = R_TRUE;
	rvn_pdf_buf_str(&page->content, "BT\n/F");
	rvn_pdf_buf_uint(&page->content, (unsigned long)doc->font);
	rvn_pdf_buf_byte(&page->content, ' ');
	rvn_pdf_buf_num(&page->content, doc->font_size);
	rvn_pdf_buf_str(&page->content, bold ? " Tf\n2 Tr\n" : " Tf\n");
	v[0] = 1;
	v[1] = 0;
	v[2] = (doc->text_style & R_PDF_TEXT_ITALIC) ? 0.2 : 0.0;
	v[3] = 1;
	v[4] = x;
	v[5] = baseline;
	rvn_pdf_buf_op(&page->content, v, 6, "Tm");
	rvn_pdf_buf_byte(&page->content, '<');
	while (p < end) {
		uint32_t code = rvn_utf8_decode(&p, end);
		int glyph;
		if (code == '\n' || code == '\r') continue;
		glyph = rvn_ttf_glyph_index(font->ttf, code);
		if (glyph < 0 || glyph >= font->glyph_count) glyph = 0;
		if (glyph && !font->used[glyph]) font->used[glyph] = code;
		rvn_pdf_buf_hex4(&page->content, (unsigned)glyph);
	}
	rvn_pdf_buf_str(&page->content, "> Tj\nET\n");
	if (doc->text_style & R_PDF_TEXT_UNDERLINE) {
		double thickness = doc->font_size * 0.06;
		v[0] = x;
		v[1] = baseline - doc->font_size * 0.12 - thickness;
		v[2] = width;
		v[3] = thickness;
		rvn_pdf_buf_op(&page->content, v, 4, "re");
		rvn_pdf_buf_str(&page->content, "f\n");
	}
	rvn_pdf_buf_str(&page->content, "Q\n");
	return width;
}

float Rpdf_document_draw_text(Rid id, float x, float y, const char* text) {
	float line, widest = 0.0f;
	RVN_PDF_DOC(0.0f);
	if (!text || !rvn_pdf_finite(x) || !rvn_pdf_finite(y)) return 0.0f;
	line = Rpdf_document_get_line_height(id);
	/* a '\n' starts a line below */
	for (;;) {
		const char* stop = strchr(text, '\n');
		size_t length = stop ? (size_t)(stop - text) : strlen(text);
		float width = rvn_pdf_text_line(doc, x, y, text, length);
		if (width > widest) widest = width;
		if (!stop) break;
		text = stop + 1;
		y += line;
	}
	return widest;
}

static int rvn_pdf_is_space(char c) {
	return c == ' ' || c == '\t';
}

float Rpdf_document_draw_paragraph(Rid id, float x, float y, float width, const char* text, RHAlign align) {
	float line_height, top = y;
	const char* p;
	RVN_PDF_DOC(0.0f);
	if (!text || !rvn_pdf_finite(x) || !rvn_pdf_finite(y) || !rvn_pdf_finite(width) || width <= 0.0f) return 0.0f;
	line_height = Rpdf_document_get_line_height(id);
	p = text;
	while (*p) {
		/* the longest run of whole words that fits, at least one character */
		const char* start = p;
		const char* line_end = NULL;
		const char* q = p;
		float line_width;
		while (*q && *q != '\n') {
			const char* word_end = q;
			while (*word_end && *word_end != '\n' && !rvn_pdf_is_space(*word_end)) word_end++;
			if (rvn_pdf_measure(doc, start, (size_t)(word_end - start)) > width && line_end) break;
			if (rvn_pdf_measure(doc, start, (size_t)(word_end - start)) > width) {
				/* one word wider than the line: cut it where it overflows */
				const char* cut = start;
				int length = (int)(word_end - start);
				int next = rvn_utf8_next(start, length, 0);
				while (next < length && rvn_pdf_measure(doc, start, (size_t)next) <= width) next = rvn_utf8_next(start, length, next);
				if (next > 1 && rvn_pdf_measure(doc, start, (size_t)next) > width) next = rvn_utf8_prev(start, next);
				if (next <= 0) next = rvn_utf8_next(start, length, 0);
				cut = start + next;
				line_end = cut;
				q = cut;
				break;
			}
			line_end = word_end;
			q = word_end;
			while (rvn_pdf_is_space(*q)) q++;
		}
		if (!line_end) line_end = q;
		line_width = rvn_pdf_measure(doc, start, (size_t)(line_end - start));
		if (line_end > start) {
			float lx = x;
			if (align == R_HALIGN_CENTER) lx = x + (width - line_width) / 2.0f;
			else if (align == R_HALIGN_RIGHT) lx = x + width - line_width;
			rvn_pdf_text_line(doc, lx, y, start, (size_t)(line_end - start));
		}
		y += line_height;
		p = q;
		while (rvn_pdf_is_space(*p)) p++;
		if (*p == '\n') p++;
	}
	return y - top;
}

/* --- Pictures ------------------------------------------------------------- */

static int rvn_pdf_add_image(rvn_pdf_doc_t* doc, const uint8_t* rgba, int width, int height) {
	rvn_pdf_image_t* image;
	size_t count = (size_t)width * (size_t)height, i;
	uint8_t* plane = (uint8_t*)rvn_alloc(count * 3);
	int opaque = R_TRUE;
	if (!plane) return -1;
	if (doc->image_count == doc->image_cap) {
		doc->image_cap = doc->image_cap ? doc->image_cap * 2 : 4;
		doc->images = (rvn_pdf_image_t*)rvn_realloc(doc->images, sizeof(rvn_pdf_image_t) * (size_t)doc->image_cap);
	}
	image = &doc->images[doc->image_count];
	memset(image, 0, sizeof(*image));
	image->width = width;
	image->height = height;
	for (i = 0; i < count; i++) {
		plane[i * 3 + 0] = rgba[i * 4 + 0];
		plane[i * 3 + 1] = rgba[i * 4 + 1];
		plane[i * 3 + 2] = rgba[i * 4 + 2];
		if (rgba[i * 4 + 3] != 255) opaque = R_FALSE;
	}
	rvn_deflate(&image->color, plane, count * 3);
	if (!opaque) {
		for (i = 0; i < count; i++) plane[i] = rgba[i * 4 + 3];
		rvn_deflate(&image->alpha, plane, count);
	}
	rvn_free(plane);
	return doc->image_count++;
}

void Rpdf_document_draw_image(Rid id, float x, float y, float width, float height, const uint8_t* rgba, int pixels_width, int pixels_height) {
	rvn_pdf_page_out_t* page;
	int image;
	double v[6];
	RVN_PDF_DOC_VOID();
	if (!rgba || pixels_width <= 0 || pixels_height <= 0 || pixels_width > 16384 || pixels_height > 16384) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rpdf_document_draw_image: no pixels, or more than 16384 on a side");
		return;
	}
	if (!rvn_pdf_finite(x) || !rvn_pdf_finite(y) || !rvn_pdf_finite(width) || !rvn_pdf_finite(height)) return;
	if (width <= 0.0f && height <= 0.0f) {
		width = (float)pixels_width * 0.75f;
		height = (float)pixels_height * 0.75f;
	} else if (height <= 0.0f) {
		height = width * (float)pixels_height / (float)pixels_width;
	} else if (width <= 0.0f) {
		width = height * (float)pixels_width / (float)pixels_height;
	}
	page = rvn_pdf_current_page(doc);
	if (!page) return;
	image = rvn_pdf_add_image(doc, rgba, pixels_width, pixels_height);
	if (image < 0) return;
	rvn_pdf_buf_str(&page->content, "q\n");
	v[0] = width; v[1] = 0; v[2] = 0; v[3] = height; v[4] = x; v[5] = page->height - y - height;
	rvn_pdf_buf_op(&page->content, v, 6, "cm");
	rvn_pdf_buf_str(&page->content, "/Im");
	rvn_pdf_buf_uint(&page->content, (unsigned long)image);
	rvn_pdf_buf_str(&page->content, " Do\nQ\n");
}

int Rpdf_document_draw_window(Rid id, Rid window, float x, float y, float width, float height) {
	uint8_t* pixels;
	int pw = 0, ph = 0;
	RVN_PDF_DOC(R_FALSE);
	pixels = Rwindow_capture(window, &pw, &ph);
	if (!pixels) return R_FALSE;
	if (width <= 0.0f && height <= 0.0f) {
		/* the size it has on the screen: 96 logical units per inch */
		float scale = Rwindow_get_scale(window);
		if (!(scale > 0.0f)) scale = 1.0f;
		width = (float)pw / scale * 0.75f;
	}
	Rpdf_document_draw_image(id, x, y, width, height, pixels, pw, ph);
	Rpixels_free(pixels);
	return R_TRUE;
}

/* --- Writing the file ----------------------------------------------------- */

typedef struct rvn_pdf_writer_t {
	rvn_pdf_buf_t out;
	size_t* offsets;   /* offset of each object, from 1 */
	int count;
	int cap;
} rvn_pdf_writer_t;

static int rvn_pdf_reserve_object(rvn_pdf_writer_t* w) {
	if (w->count + 2 > w->cap) {
		w->cap = w->cap ? w->cap * 2 : 64;
		w->offsets = (size_t*)rvn_realloc(w->offsets, sizeof(size_t) * (size_t)w->cap);
	}
	w->count++;
	w->offsets[w->count] = 0;
	return w->count;
}

static void rvn_pdf_begin_object(rvn_pdf_writer_t* w, int number) {
	w->offsets[number] = w->out.size;
	rvn_pdf_buf_uint(&w->out, (unsigned long)number);
	rvn_pdf_buf_str(&w->out, " 0 obj\n");
}

static void rvn_pdf_ref(rvn_pdf_buf_t* out, int number) {
	rvn_pdf_buf_uint(out, (unsigned long)number);
	rvn_pdf_buf_str(out, " 0 R");
}

/* A stream object: the dictionary entries given (without << >>), then the data. */
static void rvn_pdf_write_stream(rvn_pdf_writer_t* w, int number, const char* entries, const void* data, size_t size, int deflated) {
	rvn_pdf_begin_object(w, number);
	rvn_pdf_buf_str(&w->out, "<<");
	if (entries) rvn_pdf_buf_str(&w->out, entries);
	if (deflated) rvn_pdf_buf_str(&w->out, "/Filter/FlateDecode");
	rvn_pdf_buf_str(&w->out, "/Length ");
	rvn_pdf_buf_uint(&w->out, (unsigned long)size);
	rvn_pdf_buf_str(&w->out, ">>\nstream\n");
	rvn_pdf_buf_add(&w->out, data, size);
	rvn_pdf_buf_str(&w->out, "\nendstream\nendobj\n");
}

static void rvn_pdf_write_compressed(rvn_pdf_writer_t* w, int number, const char* entries, const void* data, size_t size) {
	rvn_pdf_buf_t packed = { 0 };
	rvn_deflate(&packed, (const uint8_t*)data, size);
	rvn_pdf_write_stream(w, number, entries, packed.data, packed.size, R_TRUE);
	rvn_free(packed.data);
}

/* The objects of an embedded font; returns the number of the Type0 font. */
static int rvn_pdf_write_font(rvn_pdf_writer_t* w, const rvn_pdf_font_t* font, int index) {
	int type0 = rvn_pdf_reserve_object(w);
	int cid = rvn_pdf_reserve_object(w);
	int descriptor = rvn_pdf_reserve_object(w);
	int file = rvn_pdf_reserve_object(w);
	int unicode = rvn_pdf_reserve_object(w);
	rvn_pdf_buf_t text = { 0 };
	char name[32];
	int g, mapped = 0;

	/* BaseFont: a name of our own (the font is embedded whole, not a subset) */
	name[0] = '\0';
	strcpy(name, index == R_PDF_FONT_SANS ? "Roboto" : index == R_PDF_FONT_MONO ? "RobotoMono" : "RavenFont");
	if (index > R_PDF_FONT_MONO) {
		size_t length = strlen(name);
		int n = index;
		char digits[12];
		int count = 0;
		do {
			digits[count++] = (char)('0' + n % 10);
			n /= 10;
		} while (n);
		while (count) name[length++] = digits[--count];
		name[length] = '\0';
	}

	rvn_pdf_begin_object(w, type0);
	rvn_pdf_buf_str(&w->out, "<</Type/Font/Subtype/Type0/BaseFont/");
	rvn_pdf_buf_str(&w->out, name);
	rvn_pdf_buf_str(&w->out, "/Encoding/Identity-H/DescendantFonts[");
	rvn_pdf_ref(&w->out, cid);
	rvn_pdf_buf_str(&w->out, "]/ToUnicode ");
	rvn_pdf_ref(&w->out, unicode);
	rvn_pdf_buf_str(&w->out, ">>\nendobj\n");

	rvn_pdf_begin_object(w, cid);
	rvn_pdf_buf_str(&w->out, "<</Type/Font/Subtype/CIDFontType2/BaseFont/");
	rvn_pdf_buf_str(&w->out, name);
	rvn_pdf_buf_str(&w->out, "/CIDSystemInfo<</Registry(Adobe)/Ordering(Identity)/Supplement 0>>/FontDescriptor ");
	rvn_pdf_ref(&w->out, descriptor);
	rvn_pdf_buf_str(&w->out, "/CIDToGIDMap/Identity/DW ");
	rvn_pdf_buf_num(&w->out, rvn_ttf_advance(font->ttf, 0) * 1000.0f);
	rvn_pdf_buf_str(&w->out, "/W[");
	for (g = 1; g < font->glyph_count; g++) {
		if (!font->used[g]) continue;
		rvn_pdf_buf_uint(&w->out, (unsigned long)g);
		rvn_pdf_buf_byte(&w->out, '[');
		rvn_pdf_buf_num(&w->out, rvn_ttf_advance(font->ttf, g) * 1000.0f);
		rvn_pdf_buf_str(&w->out, "]");
	}
	rvn_pdf_buf_str(&w->out, "]>>\nendobj\n");

	rvn_pdf_begin_object(w, descriptor);
	rvn_pdf_buf_str(&w->out, "<</Type/FontDescriptor/FontName/");
	rvn_pdf_buf_str(&w->out, name);
	rvn_pdf_buf_str(&w->out, "/Flags 32/FontBBox[");
	rvn_pdf_buf_num(&w->out, font->bbox[0]);
	rvn_pdf_buf_byte(&w->out, ' ');
	rvn_pdf_buf_num(&w->out, font->bbox[1]);
	rvn_pdf_buf_byte(&w->out, ' ');
	rvn_pdf_buf_num(&w->out, font->bbox[2]);
	rvn_pdf_buf_byte(&w->out, ' ');
	rvn_pdf_buf_num(&w->out, font->bbox[3]);
	rvn_pdf_buf_str(&w->out, "]/ItalicAngle 0/Ascent ");
	rvn_pdf_buf_num(&w->out, font->ascent * 1000.0f);
	rvn_pdf_buf_str(&w->out, "/Descent ");
	rvn_pdf_buf_num(&w->out, -font->descent * 1000.0f);
	rvn_pdf_buf_str(&w->out, "/CapHeight ");
	rvn_pdf_buf_num(&w->out, font->ascent * 1000.0f * 0.75f);
	rvn_pdf_buf_str(&w->out, "/StemV 80/FontFile2 ");
	rvn_pdf_ref(&w->out, file);
	rvn_pdf_buf_str(&w->out, ">>\nendobj\n");

	{
		char entries[48];
		rvn_pdf_buf_t length = { 0 };
		rvn_pdf_buf_str(&length, "/Length1 ");
		rvn_pdf_buf_uint(&length, (unsigned long)font->size);
		rvn_pdf_buf_byte(&length, '\0');
		memcpy(entries, length.data, length.size);
		rvn_free(length.data);
		rvn_pdf_write_compressed(w, file, entries, font->data, font->size);
	}

	/* ToUnicode: the character each glyph stands for, so that the text can be found and copied */
	rvn_pdf_buf_str(&text,
		"/CIDInit/ProcSet findresource begin\n12 dict begin\nbegincmap\n"
		"/CIDSystemInfo<</Registry(Adobe)/Ordering(UCS)/Supplement 0>>def\n"
		"/CMapName/Adobe-Identity-UCS def\n/CMapType 2 def\n"
		"1 begincodespacerange\n<0000><FFFF>\nendcodespacerange\n");
	for (g = 1; g < font->glyph_count; g++) {
		uint32_t code = font->used[g];
		if (!code) continue;
		if (mapped % 100 == 0) {
			int remaining = 0, h;
			if (mapped) rvn_pdf_buf_str(&text, "endbfchar\n");
			for (h = g; h < font->glyph_count && remaining < 100; h++) {
				if (font->used[h]) remaining++;
			}
			rvn_pdf_buf_uint(&text, (unsigned long)remaining);
			rvn_pdf_buf_str(&text, " beginbfchar\n");
		}
		rvn_pdf_buf_byte(&text, '<');
		rvn_pdf_buf_hex4(&text, (unsigned)g);
		rvn_pdf_buf_str(&text, "><");
		if (code >= 0x10000) {
			code -= 0x10000;
			rvn_pdf_buf_hex4(&text, 0xD800 + (code >> 10));
			rvn_pdf_buf_hex4(&text, 0xDC00 + (code & 0x3FF));
		} else {
			rvn_pdf_buf_hex4(&text, code);
		}
		rvn_pdf_buf_str(&text, ">\n");
		mapped++;
	}
	if (mapped) rvn_pdf_buf_str(&text, "endbfchar\n");
	rvn_pdf_buf_str(&text, "endcmap\nCMapName currentdict/CMap defineresource pop\nend\nend\n");
	rvn_pdf_write_compressed(w, unicode, NULL, text.data, text.size);
	rvn_free(text.data);
	return type0;
}

static void rvn_pdf_date(char out[24]) {
	time_t now = time(NULL);
	struct tm parts;
#if defined(_WIN32)
	if (gmtime_s(&parts, &now) != 0) memset(&parts, 0, sizeof(parts));
#else
	if (!gmtime_r(&now, &parts)) memset(&parts, 0, sizeof(parts));
#endif
	strftime(out, 24, "D:%Y%m%d%H%M%SZ", &parts);
}

/* The whole file, in a buffer owned by the caller. */
static int rvn_pdf_write(rvn_pdf_doc_t* doc, rvn_pdf_buf_t* result) {
	rvn_pdf_writer_t w;
	int catalog, pages, resources, info, i;
	int* font_objects;
	int* image_objects;
	int* page_objects;
	size_t xref;
	char date[24];

	memset(&w, 0, sizeof(w));
	if (doc->page_count == 0) rvn_pdf_current_page(doc);
	rvn_pdf_buf_str(&w.out, "%PDF-1.7\n%\xE2\xE3\xCF\xD3\n");

	catalog = rvn_pdf_reserve_object(&w);
	pages = rvn_pdf_reserve_object(&w);
	resources = rvn_pdf_reserve_object(&w);
	info = rvn_pdf_reserve_object(&w);
	font_objects = (int*)rvn_calloc(sizeof(int) * (size_t)(doc->font_count + 1));
	image_objects = (int*)rvn_calloc(sizeof(int) * (size_t)(doc->image_count + 1));
	page_objects = (int*)rvn_calloc(sizeof(int) * (size_t)(doc->page_count + 1));

	rvn_pdf_begin_object(&w, catalog);
	rvn_pdf_buf_str(&w.out, "<</Type/Catalog/Pages ");
	rvn_pdf_ref(&w.out, pages);
	if (doc->title) rvn_pdf_buf_str(&w.out, "/ViewerPreferences<</DisplayDocTitle true>>");
	rvn_pdf_buf_str(&w.out, ">>\nendobj\n");

	/* the fonts and pictures, shared by every page */
	for (i = 0; i < doc->font_count; i++) {
		if (doc->fonts[i].drawn) font_objects[i] = rvn_pdf_write_font(&w, &doc->fonts[i], i);
	}
	for (i = 0; i < doc->image_count; i++) {
		rvn_pdf_image_t* image = &doc->images[i];
		rvn_pdf_buf_t entries = { 0 };
		int mask = 0;
		if (image->alpha.size) {
			mask = rvn_pdf_reserve_object(&w);
			rvn_pdf_buf_str(&entries, "/Type/XObject/Subtype/Image/Width ");
			rvn_pdf_buf_uint(&entries, (unsigned long)image->width);
			rvn_pdf_buf_str(&entries, "/Height ");
			rvn_pdf_buf_uint(&entries, (unsigned long)image->height);
			rvn_pdf_buf_str(&entries, "/ColorSpace/DeviceGray/BitsPerComponent 8");
			rvn_pdf_buf_byte(&entries, '\0');
			rvn_pdf_write_stream(&w, mask, entries.data, image->alpha.data, image->alpha.size, R_TRUE);
			entries.size = 0;
		}
		image_objects[i] = rvn_pdf_reserve_object(&w);
		rvn_pdf_buf_str(&entries, "/Type/XObject/Subtype/Image/Width ");
		rvn_pdf_buf_uint(&entries, (unsigned long)image->width);
		rvn_pdf_buf_str(&entries, "/Height ");
		rvn_pdf_buf_uint(&entries, (unsigned long)image->height);
		rvn_pdf_buf_str(&entries, "/ColorSpace/DeviceRGB/BitsPerComponent 8");
		if (mask) {
			rvn_pdf_buf_str(&entries, "/SMask ");
			rvn_pdf_ref(&entries, mask);
		}
		rvn_pdf_buf_byte(&entries, '\0');
		rvn_pdf_write_stream(&w, image_objects[i], entries.data, image->color.data, image->color.size, R_TRUE);
		rvn_free(entries.data);
	}

	rvn_pdf_begin_object(&w, resources);
	rvn_pdf_buf_str(&w.out, "<</ProcSet[/PDF/Text/ImageB/ImageC]");
	rvn_pdf_buf_str(&w.out, "/Font<<");
	for (i = 0; i < doc->font_count; i++) {
		if (!font_objects[i]) continue;
		rvn_pdf_buf_str(&w.out, "/F");
		rvn_pdf_buf_uint(&w.out, (unsigned long)i);
		rvn_pdf_buf_byte(&w.out, ' ');
		rvn_pdf_ref(&w.out, font_objects[i]);
	}
	rvn_pdf_buf_str(&w.out, ">>/XObject<<");
	for (i = 0; i < doc->image_count; i++) {
		rvn_pdf_buf_str(&w.out, "/Im");
		rvn_pdf_buf_uint(&w.out, (unsigned long)i);
		rvn_pdf_buf_byte(&w.out, ' ');
		rvn_pdf_ref(&w.out, image_objects[i]);
	}
	rvn_pdf_buf_str(&w.out, ">>/ExtGState<<");
	for (i = 0; i < doc->alpha_count; i++) {
		rvn_pdf_buf_str(&w.out, "/Gs");
		rvn_pdf_buf_uint(&w.out, (unsigned long)i);
		rvn_pdf_buf_str(&w.out, "<</ca ");
		rvn_pdf_buf_num(&w.out, doc->alphas[i] / 255.0);
		rvn_pdf_buf_str(&w.out, "/CA ");
		rvn_pdf_buf_num(&w.out, doc->alphas[i] / 255.0);
		rvn_pdf_buf_str(&w.out, ">>");
	}
	rvn_pdf_buf_str(&w.out, ">>>>\nendobj\n");

	for (i = 0; i < doc->page_count; i++) {
		rvn_pdf_page_out_t* page = &doc->pages[i];
		int content = rvn_pdf_reserve_object(&w);
		page_objects[i] = rvn_pdf_reserve_object(&w);
		rvn_pdf_write_compressed(&w, content, NULL, page->content.data ? page->content.data : "", page->content.size);
		rvn_pdf_begin_object(&w, page_objects[i]);
		rvn_pdf_buf_str(&w.out, "<</Type/Page/Parent ");
		rvn_pdf_ref(&w.out, pages);
		rvn_pdf_buf_str(&w.out, "/MediaBox[0 0 ");
		rvn_pdf_buf_num(&w.out, page->width);
		rvn_pdf_buf_byte(&w.out, ' ');
		rvn_pdf_buf_num(&w.out, page->height);
		rvn_pdf_buf_str(&w.out, "]/Resources ");
		rvn_pdf_ref(&w.out, resources);
		rvn_pdf_buf_str(&w.out, "/Contents ");
		rvn_pdf_ref(&w.out, content);
		rvn_pdf_buf_str(&w.out, ">>\nendobj\n");
	}

	rvn_pdf_begin_object(&w, pages);
	rvn_pdf_buf_str(&w.out, "<</Type/Pages/Count ");
	rvn_pdf_buf_uint(&w.out, (unsigned long)doc->page_count);
	rvn_pdf_buf_str(&w.out, "/Kids[");
	for (i = 0; i < doc->page_count; i++) {
		if (i) rvn_pdf_buf_byte(&w.out, ' ');
		rvn_pdf_ref(&w.out, page_objects[i]);
	}
	rvn_pdf_buf_str(&w.out, "]>>\nendobj\n");

	rvn_pdf_date(date);
	rvn_pdf_begin_object(&w, info);
	rvn_pdf_buf_str(&w.out, "<</Producer(Raven)/CreationDate(");
	rvn_pdf_buf_str(&w.out, date);
	rvn_pdf_buf_byte(&w.out, ')');
	if (doc->title) {
		rvn_pdf_buf_str(&w.out, "/Title");
		rvn_pdf_buf_text_string(&w.out, doc->title);
	}
	if (doc->author) {
		rvn_pdf_buf_str(&w.out, "/Author");
		rvn_pdf_buf_text_string(&w.out, doc->author);
	}
	if (doc->subject) {
		rvn_pdf_buf_str(&w.out, "/Subject");
		rvn_pdf_buf_text_string(&w.out, doc->subject);
	}
	rvn_pdf_buf_str(&w.out, ">>\nendobj\n");

	xref = w.out.size;
	rvn_pdf_buf_str(&w.out, "xref\n0 ");
	rvn_pdf_buf_uint(&w.out, (unsigned long)(w.count + 1));
	rvn_pdf_buf_str(&w.out, "\n0000000000 65535 f \n");
	for (i = 1; i <= w.count; i++) {
		char entry[21];
		size_t offset = w.offsets[i];
		int d;
		for (d = 9; d >= 0; d--) {
			entry[d] = (char)('0' + offset % 10);
			offset /= 10;
		}
		memcpy(entry + 10, " 00000 n \n", 10);
		rvn_pdf_buf_add(&w.out, entry, 20);
	}
	rvn_pdf_buf_str(&w.out, "trailer\n<</Size ");
	rvn_pdf_buf_uint(&w.out, (unsigned long)(w.count + 1));
	rvn_pdf_buf_str(&w.out, "/Root ");
	rvn_pdf_ref(&w.out, catalog);
	rvn_pdf_buf_str(&w.out, "/Info ");
	rvn_pdf_ref(&w.out, info);
	rvn_pdf_buf_str(&w.out, ">>\nstartxref\n");
	rvn_pdf_buf_uint(&w.out, (unsigned long)xref);
	rvn_pdf_buf_str(&w.out, "\n%%EOF\n");

	rvn_free(font_objects);
	rvn_free(image_objects);
	rvn_free(page_objects);
	rvn_free(w.offsets);
	*result = w.out;
	return R_TRUE;
}

char* Rpdf_document_save_memory(Rid id, size_t* size) {
	rvn_pdf_buf_t out;
	if (size) *size = 0;
	{
		RVN_PDF_DOC(NULL);
		if (!rvn_pdf_write(doc, &out)) return NULL;
	}
	/* followed by a '\0' (not counted), as Rfile_read */
	rvn_pdf_buf_byte(&out, 0);
	out.size--;
	if (size) *size = out.size;
	return out.data;
}

int Rpdf_document_save(Rid id, const char* path) {
	size_t size = 0;
	char* data;
	int ok;
	if (!path || !path[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rpdf_document_save: no path");
		return R_FALSE;
	}
	data = Rpdf_document_save_memory(id, &size);
	if (!data) return R_FALSE;
	ok = Rfile_write(path, data, size);
	rvn_free(data);
	return ok;
}
