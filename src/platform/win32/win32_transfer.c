/**
 * Raven - platform/win32/win32_transfer.c
 * Data for the other applications on Windows: the formats of the clipboard
 * and the drags out of the windows.
 *
 * The formats Raven names by MIME types (RDataItem) become the formats of
 * the clipboard the programs of Windows read:
 *
 *   text/plain      CF_UNICODETEXT
 *   text/html       "HTML Format" (the header of CF_HTML before the markup)
 *   text/uri-list   CF_HDROP: the files, as the Explorer copies them
 *   image/png       "PNG", and the bitmap CF_DIBV5 for the older programs;
 *                   read back from "PNG", else from CF_DIBV5 or CF_DIB
 *   text/rtf        "Rich Text Format"
 *   anything else   a format registered under its MIME type
 *
 * A drag is OLE's: DoDragDrop with a data object and a drop source made
 * here, the picture that follows the pointer given to the drag helper of the
 * shell. The data object also keeps what the targets give it (SetData): the
 * helper stores its picture there, and the Explorer what it did.
 */

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef COBJMACROS
#define COBJMACROS
#endif
#include <windows.h>
#include <ole2.h>
#include <shlobj.h>
#include <shellapi.h>
#include <string.h>

#include "win32_transfer.h"
#include "../../core/transfer.h"

/* the identifiers, here rather than from uuid.lib, which not every toolchain has with them */
static const GUID rvn_tr_iid_unknown = { 0x00000000, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID rvn_tr_iid_data_object = { 0x0000010e, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID rvn_tr_iid_drop_source = { 0x00000121, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID rvn_tr_clsid_drag_helper = { 0x4657278a, 0x411b, 0x11d2, { 0x83, 0x9a, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0xd0 } };
static const GUID rvn_tr_iid_drag_source_helper = { 0xde5bf786, 0x477a, 0x11d2, { 0x83, 0x9d, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0xd0 } };

/* OLE, initialized on the UI thread for the first drag */
static int g_rvn_ole;

/* One format of the clipboard and its bytes. */
typedef struct rvn_win32_format_t {
	UINT format;
	char* bytes;
	size_t size;
} rvn_win32_format_t;

typedef struct rvn_win32_formats_t {
	rvn_win32_format_t* items;
	uint32_t count, cap;
} rvn_win32_formats_t;

static void rvn_win32_formats_put(rvn_win32_formats_t* list, UINT format, char* bytes, size_t size) {
	uint32_t i;
	/* (the same format again replaces it) */
	for (i = 0; i < list->count; i++) {
		if (list->items[i].format == format) {
			rvn_free(list->items[i].bytes);
			list->items[i].bytes = bytes;
			list->items[i].size = size;
			return;
		}
	}
	RVN_ARRAY_RESERVE(list->items, list->cap, list->count + 1);
	list->items[list->count].format = format;
	list->items[list->count].bytes = bytes;
	list->items[list->count].size = size;
	list->count++;
}

static void rvn_win32_formats_free(rvn_win32_formats_t* list) {
	uint32_t i;
	for (i = 0; i < list->count; i++) rvn_free(list->items[i].bytes);
	rvn_free(list->items);
	memset(list, 0, sizeof(*list));
}

static UINT rvn_win32_register(const char* name) {
	WCHAR wide[256];
	int length = MultiByteToWideChar(CP_UTF8, 0, name, -1, wide, (int)(sizeof(wide) / sizeof(wide[0])));
	return length > 0 ? RegisterClipboardFormatW(wide) : 0;
}

static UINT rvn_win32_cf_html(void) {
	static UINT format;
	if (!format) format = RegisterClipboardFormatW(L"HTML Format");
	return format;
}

static UINT rvn_win32_cf_png(void) {
	static UINT format;
	if (!format) format = RegisterClipboardFormatW(L"PNG");
	return format;
}

static UINT rvn_win32_cf_rtf(void) {
	static UINT format;
	if (!format) format = RegisterClipboardFormatW(L"Rich Text Format");
	return format;
}

/* The native format a MIME type is kept under when it has no conversion. */
static UINT rvn_win32_format_of(const char* mime) {
	if (rvn_mime_is(mime, "text/rtf")) return rvn_win32_cf_rtf();
	return rvn_win32_register(mime);
}

/* UTF-8 bytes -> UTF-16 text and its '\0' (rvn_alloc'ed), `size` in bytes. */
static char* rvn_win32_utf16(const char* text, size_t length, size_t* size) {
	int count = length ? MultiByteToWideChar(CP_UTF8, 0, text, (int)length, NULL, 0) : 0;
	WCHAR* wide = (WCHAR*)rvn_alloc(sizeof(WCHAR) * (size_t)(count + 1));
	if (count > 0) MultiByteToWideChar(CP_UTF8, 0, text, (int)length, wide, count);
	wide[count > 0 ? count : 0] = 0;
	*size = sizeof(WCHAR) * (size_t)((count > 0 ? count : 0) + 1);
	return (char*)wide;
}

/* UTF-16 text (at most `count` characters, ends at its '\0') -> UTF-8 (rvn_alloc'ed). */
static char* rvn_win32_utf8(const WCHAR* wide, size_t count, size_t* size) {
	size_t length = 0;
	int bytes;
	char* text;
	while (length < count && wide[length]) length++;
	bytes = length ? WideCharToMultiByte(CP_UTF8, 0, wide, (int)length, NULL, 0, NULL, NULL) : 0;
	text = (char*)rvn_alloc((size_t)(bytes > 0 ? bytes : 0) + 1);
	if (bytes > 0) WideCharToMultiByte(CP_UTF8, 0, wide, (int)length, text, bytes, NULL, NULL);
	text[bytes > 0 ? bytes : 0] = '\0';
	*size = (size_t)(bytes > 0 ? bytes : 0);
	return text;
}

/*
 * CF_HTML: a header that tells where the document and the part copied are,
 * in bytes from the start. A whole document is given as it is; a part is put
 * in one.
 */
static char* rvn_win32_cf_html_make(const char* html, size_t length, size_t* size) {
	static const char header[] =
		"Version:0.9\r\nStartHTML:%010u\r\nEndHTML:%010u\r\nStartFragment:%010u\r\nEndFragment:%010u\r\n";
	static const char before[] = "<html><body>\r\n<!--StartFragment-->";
	static const char after[] = "<!--EndFragment-->\r\n</body></html>";
	size_t header_size = 105, total, i;
	int whole = R_FALSE;
	char* out;
	unsigned start_html, end_html, start_fragment, end_fragment;
	for (i = 0; i + 5 <= length; i++) {
		if (rvn_strnicmp(html + i, "<html", 5) == 0) {
			whole = R_TRUE;
			break;
		}
	}
	total = header_size + (whole ? 0 : sizeof(before) - 1) + length + (whole ? 0 : sizeof(after) - 1);
	out = (char*)rvn_alloc(total + 1);
	start_html = (unsigned)header_size;
	end_html = (unsigned)total;
	start_fragment = (unsigned)(header_size + (whole ? 0 : sizeof(before) - 1));
	end_fragment = (unsigned)(start_fragment + length);
	snprintf(out, header_size + 1, header, start_html, end_html, start_fragment, end_fragment);
	i = header_size;
	if (!whole) { memcpy(out + i, before, sizeof(before) - 1); i += sizeof(before) - 1; }
	memcpy(out + i, html, length);
	i += length;
	if (!whole) { memcpy(out + i, after, sizeof(after) - 1); i += sizeof(after) - 1; }
	out[i] = '\0';
	*size = i + 1;
	return out;
}

static long rvn_win32_cf_html_field(const char* data, size_t size, const char* name) {
	size_t length = strlen(name), i;
	for (i = 0; i + length < size && i < 1024; i++) {
		if (memcmp(data + i, name, length) == 0) return strtol(data + i + length, NULL, 10);
	}
	return -1;
}

/* The markup of CF_HTML: the document it holds (the part copied when it does not say). */
static char* rvn_win32_cf_html_read(const char* data, size_t size, size_t* out_size) {
	long start = rvn_win32_cf_html_field(data, size, "StartHTML:");
	long end = rvn_win32_cf_html_field(data, size, "EndHTML:");
	if (start < 0 || end < 0) {
		start = rvn_win32_cf_html_field(data, size, "StartFragment:");
		end = rvn_win32_cf_html_field(data, size, "EndFragment:");
	}
	if (start < 0 || end < start || (size_t)end > size) return NULL;
	while (end > start && data[end - 1] == '\0') end--;
	*out_size = (size_t)(end - start);
	return rvn_bytes_dup(data + start, (size_t)(end - start));
}

/* CF_HDROP of the local files of a text/uri-list, or NULL when it names none. */
static char* rvn_win32_hdrop_make(const char* list, size_t length, size_t* size) {
	char** paths = NULL;
	int count = rvn_uri_list_paths(list, length, &paths), i;
	size_t chars = 1, used;
	DROPFILES* drop;
	WCHAR* names;
	if (count <= 0) return NULL;
	for (i = 0; i < count; i++) chars += (size_t)MultiByteToWideChar(CP_UTF8, 0, paths[i], -1, NULL, 0);
	*size = sizeof(DROPFILES) + chars * sizeof(WCHAR);
	drop = (DROPFILES*)rvn_calloc(*size);
	drop->pFiles = sizeof(DROPFILES);
	drop->fWide = TRUE;
	names = (WCHAR*)(drop + 1);
	used = 0;
	for (i = 0; i < count; i++) {
		int n = MultiByteToWideChar(CP_UTF8, 0, paths[i], -1, names + used, (int)(chars - used));
		if (n > 0) used += (size_t)n;
	}
	names[used] = 0;
	rvn_free(paths);
	return (char*)drop;
}

/* RGBA8 -> CF_DIBV5: a bitmap with its alpha, rows from the bottom, BGRA. */
static char* rvn_win32_dib_make(const uint8_t* rgba, int width, int height, size_t* size) {
	BITMAPV5HEADER* header;
	uint8_t* pixels;
	int x, y;
	*size = sizeof(BITMAPV5HEADER) + (size_t)width * (size_t)height * 4u;
	header = (BITMAPV5HEADER*)rvn_calloc(*size);
	header->bV5Size = sizeof(BITMAPV5HEADER);
	header->bV5Width = width;
	header->bV5Height = height;
	header->bV5Planes = 1;
	header->bV5BitCount = 32;
	header->bV5Compression = BI_BITFIELDS;
	header->bV5SizeImage = (DWORD)((size_t)width * (size_t)height * 4u);
	header->bV5RedMask = 0x00FF0000u;
	header->bV5GreenMask = 0x0000FF00u;
	header->bV5BlueMask = 0x000000FFu;
	header->bV5AlphaMask = 0xFF000000u;
	header->bV5CSType = 0x73524742; /* LCS_sRGB */
	header->bV5Intent = LCS_GM_IMAGES;
	pixels = (uint8_t*)(header + 1);
	for (y = 0; y < height; y++) {
		const uint8_t* from = rgba + (size_t)y * (size_t)width * 4u;
		uint8_t* to = pixels + (size_t)(height - 1 - y) * (size_t)width * 4u;
		for (x = 0; x < width; x++) {
			to[x * 4 + 0] = from[x * 4 + 2];
			to[x * 4 + 1] = from[x * 4 + 1];
			to[x * 4 + 2] = from[x * 4 + 0];
			to[x * 4 + 3] = from[x * 4 + 3];
		}
	}
	return (char*)header;
}

static int rvn_win32_mask_shift(DWORD mask, int* bits) {
	int shift = 0, count = 0;
	if (!mask) { *bits = 0; return 0; }
	while (!(mask & 1u)) { mask >>= 1; shift++; }
	while (mask & 1u) { mask >>= 1; count++; }
	*bits = count;
	return shift;
}

static uint8_t rvn_win32_channel(DWORD value, DWORD mask, int shift, int bits) {
	DWORD v;
	if (!mask || bits <= 0) return 255;
	v = (value & mask) >> shift;
	if (bits >= 8) return (uint8_t)(v >> (bits - 8));
	return (uint8_t)((v * 255u) / ((1u << bits) - 1u));
}

/* CF_DIB / CF_DIBV5 (24 or 32 bits a pixel) -> a PNG file. */
static char* rvn_win32_dib_to_png(const char* data, size_t size, size_t* png_size) {
	const BITMAPINFOHEADER* info = (const BITMAPINFOHEADER*)data;
	DWORD red = 0x00FF0000u, green = 0x0000FF00u, blue = 0x000000FFu, alpha = 0;
	int width, height, top_down, bits, x, y, rs, gs, bs, as, rb, gb, bb, ab, any_alpha = 0;
	size_t offset, stride;
	uint8_t* rgba;
	char* png;
	if (size < sizeof(BITMAPINFOHEADER) || info->biSize < sizeof(BITMAPINFOHEADER) || info->biSize > size) return NULL;
	width = info->biWidth;
	height = info->biHeight < 0 ? -info->biHeight : info->biHeight;
	top_down = info->biHeight < 0;
	bits = info->biBitCount;
	if (width <= 0 || height <= 0 || width > 32767 || height > 32767 || (bits != 24 && bits != 32)) return NULL;
	if (info->biCompression != BI_RGB && info->biCompression != BI_BITFIELDS) return NULL;
	offset = info->biSize;
	if (info->biCompression == BI_BITFIELDS) {
		if (info->biSize >= sizeof(BITMAPV4HEADER)) {
			const BITMAPV4HEADER* v4 = (const BITMAPV4HEADER*)data;
			red = v4->bV4RedMask;
			green = v4->bV4GreenMask;
			blue = v4->bV4BlueMask;
			alpha = v4->bV4AlphaMask;
		} else {
			const DWORD* masks = (const DWORD*)(data + info->biSize);
			if (size < info->biSize + 12) return NULL;
			red = masks[0];
			green = masks[1];
			blue = masks[2];
			offset += 12;
		}
	} else if (bits == 32) {
		/* (BI_RGB: the fourth byte may be alpha, or nothing: told below) */
		alpha = 0xFF000000u;
	}
	offset += (size_t)info->biClrUsed * 4u;
	stride = (((size_t)width * (size_t)bits + 31u) / 32u) * 4u;
	if (offset > size || stride * (size_t)height > size - offset) return NULL;
	rs = rvn_win32_mask_shift(red, &rb);
	gs = rvn_win32_mask_shift(green, &gb);
	bs = rvn_win32_mask_shift(blue, &bb);
	as = rvn_win32_mask_shift(alpha, &ab);
	rgba = (uint8_t*)rvn_alloc((size_t)width * (size_t)height * 4u);
	for (y = 0; y < height; y++) {
		const uint8_t* row = (const uint8_t*)data + offset + stride * (size_t)(top_down ? y : height - 1 - y);
		uint8_t* to = rgba + (size_t)y * (size_t)width * 4u;
		for (x = 0; x < width; x++) {
			if (bits == 24) {
				to[x * 4 + 0] = row[x * 3 + 2];
				to[x * 4 + 1] = row[x * 3 + 1];
				to[x * 4 + 2] = row[x * 3 + 0];
				to[x * 4 + 3] = 255;
			} else {
				DWORD value;
				memcpy(&value, row + x * 4, 4);
				to[x * 4 + 0] = rvn_win32_channel(value, red, rs, rb);
				to[x * 4 + 1] = rvn_win32_channel(value, green, gs, gb);
				to[x * 4 + 2] = rvn_win32_channel(value, blue, bs, bb);
				to[x * 4 + 3] = rvn_win32_channel(value, alpha, as, ab);
				if (alpha && (value & alpha)) any_alpha = 1;
			}
		}
	}
	/* an alpha that is 0 everywhere is no alpha: the bitmap is opaque */
	if (bits == 32 && alpha && !any_alpha) {
		size_t i;
		for (i = 3; i < (size_t)width * (size_t)height * 4u; i += 4) rgba[i] = 255;
	}
	png = (char*)Rimage_encode_png(rgba, width, height, png_size);
	rvn_free(rgba);
	return png;
}

/* The native formats of `items`, converted. */
static void rvn_win32_formats_from(rvn_win32_formats_t* list, const RDataItem* items, int count) {
	int i;
	for (i = 0; i < count; i++) {
		const RDataItem* item = &items[i];
		const char* bytes = (const char*)item->data;
		size_t size = 0;
		char* made;
		if (!item->mime) continue;
		if (rvn_mime_is(item->mime, "text/plain")) {
			made = rvn_win32_utf16(bytes, item->size, &size);
			rvn_win32_formats_put(list, CF_UNICODETEXT, made, size);
		} else if (rvn_mime_is(item->mime, "text/html")) {
			made = rvn_win32_cf_html_make(bytes, item->size, &size);
			rvn_win32_formats_put(list, rvn_win32_cf_html(), made, size);
		} else if (rvn_mime_is(item->mime, "text/uri-list")) {
			made = rvn_win32_hdrop_make(bytes, item->size, &size);
			if (made) rvn_win32_formats_put(list, CF_HDROP, made, size);
		} else if (rvn_mime_is(item->mime, "image/png")) {
			int width = 0, height = 0;
			uint8_t* rgba = Rimage_decode(bytes, item->size, &width, &height);
			rvn_win32_formats_put(list, rvn_win32_cf_png(), rvn_bytes_dup(bytes, item->size), item->size);
			if (rgba) {
				made = rvn_win32_dib_make(rgba, width, height, &size);
				rvn_win32_formats_put(list, CF_DIBV5, made, size);
				Rpixels_free(rgba);
			}
		} else {
			UINT format = rvn_win32_format_of(item->mime);
			if (format) rvn_win32_formats_put(list, format, rvn_bytes_dup(bytes, item->size), item->size);
		}
	}
}

static HGLOBAL rvn_win32_global(const char* bytes, size_t size) {
	HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, size ? size : 1);
	void* target;
	if (!memory) return NULL;
	target = GlobalLock(memory);
	if (!target) {
		GlobalFree(memory);
		return NULL;
	}
	if (size) memcpy(target, bytes, size);
	GlobalUnlock(memory);
	return memory;
}

/* ---------------------------------------------------------------------------
 *  Clipboard
 * ------------------------------------------------------------------------- */

int rvn_win32_clipboard_set_data(const RDataItem* items, int count) {
	rvn_win32_formats_t list;
	uint32_t i;
	int all = R_TRUE;
	memset(&list, 0, sizeof(list));
	rvn_win32_formats_from(&list, items, count);
	if (!OpenClipboard(NULL)) {
		rvn_win32_formats_free(&list);
		return R_FALSE;
	}
	EmptyClipboard();
	for (i = 0; i < list.count; i++) {
		HGLOBAL memory = rvn_win32_global(list.items[i].bytes, list.items[i].size);
		/* on success the clipboard owns the memory */
		if (!memory || !SetClipboardData(list.items[i].format, memory)) {
			if (memory) GlobalFree(memory);
			all = R_FALSE;
		}
	}
	CloseClipboard();
	/* (a text/uri-list without local files has no format here) */
	if ((int)list.count == 0 && count > 0) all = R_FALSE;
	rvn_win32_formats_free(&list);
	return all;
}

int rvn_win32_clipboard_has_data(const char* mime) {
	if (rvn_mime_is(mime, "text/plain")) return IsClipboardFormatAvailable(CF_UNICODETEXT) || IsClipboardFormatAvailable(CF_TEXT);
	if (rvn_mime_is(mime, "text/html")) return IsClipboardFormatAvailable(rvn_win32_cf_html());
	if (rvn_mime_is(mime, "text/uri-list")) return IsClipboardFormatAvailable(CF_HDROP);
	if (rvn_mime_is(mime, "image/png")) {
		return IsClipboardFormatAvailable(rvn_win32_cf_png()) || IsClipboardFormatAvailable(CF_DIBV5) || IsClipboardFormatAvailable(CF_DIB);
	}
	return IsClipboardFormatAvailable(rvn_win32_format_of(mime));
}

/* The bytes of one format of the clipboard (open). */
static char* rvn_win32_clipboard_bytes(UINT format, size_t* size) {
	HANDLE memory = format ? GetClipboardData(format) : NULL;
	const char* bytes;
	char* copy;
	if (!memory) return NULL;
	bytes = (const char*)GlobalLock(memory);
	if (!bytes) return NULL;
	*size = (size_t)GlobalSize(memory);
	copy = rvn_bytes_dup(bytes, *size);
	GlobalUnlock(memory);
	return copy;
}

/* The files of CF_HDROP as a text/uri-list. */
static char* rvn_win32_hdrop_read(HDROP drop, size_t* size) {
	UINT count = DragQueryFileW(drop, 0xFFFFFFFFu, NULL, 0), i;
	rvn_text_t list;
	memset(&list, 0, sizeof(list));
	for (i = 0; i < count; i++) {
		UINT length = DragQueryFileW(drop, i, NULL, 0);
		WCHAR* wide = (WCHAR*)rvn_alloc(sizeof(WCHAR) * (length + 1));
		size_t bytes;
		char* path;
		DragQueryFileW(drop, i, wide, length + 1);
		path = rvn_win32_utf8(wide, length + 1, &bytes);
		rvn_uri_list_add(&list, path);
		rvn_free(path);
		rvn_free(wide);
	}
	*size = list.length;
	return list.data;
}

char* rvn_win32_clipboard_get_data(const char* mime, size_t* size) {
	char* result = NULL;
	size_t length = 0;
	if (!OpenClipboard(NULL)) return NULL;
	if (rvn_mime_is(mime, "text/plain")) {
		HANDLE memory = GetClipboardData(CF_UNICODETEXT);
		const WCHAR* wide = memory ? (const WCHAR*)GlobalLock(memory) : NULL;
		if (wide) {
			result = rvn_win32_utf8(wide, (size_t)GlobalSize(memory) / sizeof(WCHAR), &length);
			GlobalUnlock(memory);
		}
	} else if (rvn_mime_is(mime, "text/html")) {
		size_t raw_size = 0;
		char* raw = rvn_win32_clipboard_bytes(rvn_win32_cf_html(), &raw_size);
		if (raw) {
			result = rvn_win32_cf_html_read(raw, raw_size, &length);
			rvn_free(raw);
		}
	} else if (rvn_mime_is(mime, "text/uri-list")) {
		HANDLE memory = GetClipboardData(CF_HDROP);
		if (memory) result = rvn_win32_hdrop_read((HDROP)memory, &length);
	} else if (rvn_mime_is(mime, "image/png")) {
		result = rvn_win32_clipboard_bytes(rvn_win32_cf_png(), &length);
		if (!result) {
			size_t raw_size = 0;
			char* raw = rvn_win32_clipboard_bytes(CF_DIBV5, &raw_size);
			if (!raw) raw = rvn_win32_clipboard_bytes(CF_DIB, &raw_size);
			if (raw) {
				result = rvn_win32_dib_to_png(raw, raw_size, &length);
				rvn_free(raw);
			}
		}
	} else {
		result = rvn_win32_clipboard_bytes(rvn_win32_format_of(mime), &length);
	}
	CloseClipboard();
	if (result && size) *size = length;
	return result;
}

/* ---------------------------------------------------------------------------
 *  The data object of a drag
 * ------------------------------------------------------------------------- */

typedef struct rvn_win32_data_object_t {
	IDataObject iface;
	LONG refs;
	rvn_win32_formats_t formats;
} rvn_win32_data_object_t;

static rvn_win32_format_t* rvn_win32_data_find(rvn_win32_data_object_t* self, const FORMATETC* format) {
	uint32_t i;
	if (!format || !(format->tymed & TYMED_HGLOBAL) || format->dwAspect != DVASPECT_CONTENT) return NULL;
	for (i = 0; i < self->formats.count; i++) {
		if (self->formats.items[i].format == format->cfFormat) return &self->formats.items[i];
	}
	return NULL;
}

static HRESULT STDMETHODCALLTYPE rvn_do_query_interface(IDataObject* iface, REFIID riid, void** out) {
	if (!out) return E_POINTER;
	if (IsEqualIID(riid, &rvn_tr_iid_unknown) || IsEqualIID(riid, &rvn_tr_iid_data_object)) {
		*out = iface;
		IDataObject_AddRef(iface);
		return S_OK;
	}
	*out = NULL;
	return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE rvn_do_add_ref(IDataObject* iface) {
	return (ULONG)InterlockedIncrement(&((rvn_win32_data_object_t*)iface)->refs);
}

static ULONG STDMETHODCALLTYPE rvn_do_release(IDataObject* iface) {
	rvn_win32_data_object_t* self = (rvn_win32_data_object_t*)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	if (refs == 0) {
		rvn_win32_formats_free(&self->formats);
		rvn_free(self);
	}
	return (ULONG)refs;
}

static HRESULT STDMETHODCALLTYPE rvn_do_get_data(IDataObject* iface, FORMATETC* format, STGMEDIUM* medium) {
	rvn_win32_format_t* found = rvn_win32_data_find((rvn_win32_data_object_t*)iface, format);
	if (!medium) return E_POINTER;
	if (!found) return DV_E_FORMATETC;
	medium->tymed = TYMED_HGLOBAL;
	medium->hGlobal = rvn_win32_global(found->bytes, found->size);
	medium->pUnkForRelease = NULL;
	return medium->hGlobal ? S_OK : E_OUTOFMEMORY;
}

static HRESULT STDMETHODCALLTYPE rvn_do_get_data_here(IDataObject* iface, FORMATETC* format, STGMEDIUM* medium) {
	(void)iface;
	(void)format;
	(void)medium;
	return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE rvn_do_query_get_data(IDataObject* iface, FORMATETC* format) {
	return rvn_win32_data_find((rvn_win32_data_object_t*)iface, format) ? S_OK : DV_E_FORMATETC;
}

static HRESULT STDMETHODCALLTYPE rvn_do_get_canonical(IDataObject* iface, FORMATETC* in, FORMATETC* out) {
	(void)iface;
	(void)in;
	if (out) out->ptd = NULL;
	return E_NOTIMPL;
}

/* What the targets and the drag helper give: kept with the rest. */
static HRESULT STDMETHODCALLTYPE rvn_do_set_data(IDataObject* iface, FORMATETC* format, STGMEDIUM* medium, BOOL release) {
	rvn_win32_data_object_t* self = (rvn_win32_data_object_t*)iface;
	const char* bytes;
	size_t size;
	if (!format || !medium || medium->tymed != TYMED_HGLOBAL || !(format->tymed & TYMED_HGLOBAL)) return E_NOTIMPL;
	bytes = (const char*)GlobalLock(medium->hGlobal);
	if (!bytes) return E_INVALIDARG;
	size = (size_t)GlobalSize(medium->hGlobal);
	rvn_win32_formats_put(&self->formats, format->cfFormat, rvn_bytes_dup(bytes, size), size);
	GlobalUnlock(medium->hGlobal);
	if (release) ReleaseStgMedium(medium);
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE rvn_do_enum_format_etc(IDataObject* iface, DWORD direction, IEnumFORMATETC** out) {
	rvn_win32_data_object_t* self = (rvn_win32_data_object_t*)iface;
	FORMATETC* formats;
	HRESULT result;
	uint32_t i;
	if (!out) return E_POINTER;
	*out = NULL;
	if (direction != DATADIR_GET) return E_NOTIMPL;
	formats = (FORMATETC*)rvn_calloc(sizeof(FORMATETC) * (self->formats.count ? self->formats.count : 1));
	for (i = 0; i < self->formats.count; i++) {
		formats[i].cfFormat = (CLIPFORMAT)self->formats.items[i].format;
		formats[i].dwAspect = DVASPECT_CONTENT;
		formats[i].lindex = -1;
		formats[i].tymed = TYMED_HGLOBAL;
	}
	result = SHCreateStdEnumFmtEtc(self->formats.count, formats, out);
	rvn_free(formats);
	return result;
}

static HRESULT STDMETHODCALLTYPE rvn_do_dadvise(IDataObject* iface, FORMATETC* format, DWORD flags, IAdviseSink* sink, DWORD* connection) {
	(void)iface;
	(void)format;
	(void)flags;
	(void)sink;
	(void)connection;
	return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE rvn_do_dunadvise(IDataObject* iface, DWORD connection) {
	(void)iface;
	(void)connection;
	return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE rvn_do_enum_dadvise(IDataObject* iface, IEnumSTATDATA** out) {
	(void)iface;
	if (out) *out = NULL;
	return OLE_E_ADVISENOTSUPPORTED;
}

static IDataObjectVtbl g_rvn_data_object_vtbl = {
	rvn_do_query_interface,
	rvn_do_add_ref,
	rvn_do_release,
	rvn_do_get_data,
	rvn_do_get_data_here,
	rvn_do_query_get_data,
	rvn_do_get_canonical,
	rvn_do_set_data,
	rvn_do_enum_format_etc,
	rvn_do_dadvise,
	rvn_do_dunadvise,
	rvn_do_enum_dadvise
};

/* ---------------------------------------------------------------------------
 *  The drop source: the drag goes on while a button is held
 * ------------------------------------------------------------------------- */

typedef struct rvn_win32_drop_source_t {
	IDropSource iface;
	LONG refs;
} rvn_win32_drop_source_t;

static HRESULT STDMETHODCALLTYPE rvn_ds_query_interface(IDropSource* iface, REFIID riid, void** out) {
	if (!out) return E_POINTER;
	if (IsEqualIID(riid, &rvn_tr_iid_unknown) || IsEqualIID(riid, &rvn_tr_iid_drop_source)) {
		*out = iface;
		IDropSource_AddRef(iface);
		return S_OK;
	}
	*out = NULL;
	return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE rvn_ds_add_ref(IDropSource* iface) {
	return (ULONG)InterlockedIncrement(&((rvn_win32_drop_source_t*)iface)->refs);
}

static ULONG STDMETHODCALLTYPE rvn_ds_release(IDropSource* iface) {
	rvn_win32_drop_source_t* self = (rvn_win32_drop_source_t*)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	if (refs == 0) rvn_free(self);
	return (ULONG)refs;
}

static HRESULT STDMETHODCALLTYPE rvn_ds_query_continue(IDropSource* iface, BOOL escape, DWORD keys) {
	(void)iface;
	if (escape) return DRAGDROP_S_CANCEL;
	if (!(keys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON))) return DRAGDROP_S_DROP;
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE rvn_ds_give_feedback(IDropSource* iface, DWORD effect) {
	(void)iface;
	(void)effect;
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

static IDropSourceVtbl g_rvn_drop_source_vtbl = {
	rvn_ds_query_interface,
	rvn_ds_add_ref,
	rvn_ds_release,
	rvn_ds_query_continue,
	rvn_ds_give_feedback
};

/* ---------------------------------------------------------------------------
 *  The drag
 * ------------------------------------------------------------------------- */

/* The picture that follows the pointer, to the drag helper of the shell: premultiplied BGRA, rows from the top. */
static void rvn_win32_drag_image(IDataObject* data, const uint8_t* rgba, int width, int height) {
	IDragSourceHelper* helper = NULL;
	SHDRAGIMAGE image;
	BITMAPINFO info;
	void* bits = NULL;
	HBITMAP bitmap;
	int i;
	if (FAILED(CoCreateInstance(&rvn_tr_clsid_drag_helper, NULL, CLSCTX_INPROC_SERVER, &rvn_tr_iid_drag_source_helper, (void**)&helper)) || !helper) return;
	memset(&info, 0, sizeof(info));
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = width;
	info.bmiHeader.biHeight = -height;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	bitmap = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, &bits, NULL, 0);
	if (bitmap && bits) {
		uint8_t* to = (uint8_t*)bits;
		for (i = 0; i < width * height; i++) {
			unsigned a = rgba[i * 4 + 3];
			to[i * 4 + 0] = (uint8_t)(rgba[i * 4 + 2] * a / 255u);
			to[i * 4 + 1] = (uint8_t)(rgba[i * 4 + 1] * a / 255u);
			to[i * 4 + 2] = (uint8_t)(rgba[i * 4 + 0] * a / 255u);
			to[i * 4 + 3] = (uint8_t)a;
		}
		memset(&image, 0, sizeof(image));
		image.sizeDragImage.cx = width;
		image.sizeDragImage.cy = height;
		image.ptOffset.x = width / 2;
		image.ptOffset.y = height / 2;
		image.hbmpDragImage = bitmap;
		image.crColorKey = CLR_NONE;
		/* (on success the helper owns the bitmap) */
		if (FAILED(IDragSourceHelper_InitializeFromBitmap(helper, &image, data))) DeleteObject(bitmap);
	} else if (bitmap) {
		DeleteObject(bitmap);
	}
	IDragSourceHelper_Release(helper);
}

/* A picture at most `limit` pixels wide and high (rvn_alloc'ed), from a PNG. */
static uint8_t* rvn_win32_thumbnail(const RDataItem* png, int limit, int* width, int* height) {
	int w = 0, h = 0, x, y, tw, th;
	uint8_t* full = Rimage_decode(png->data, png->size, &w, &h);
	uint8_t* scaled;
	if (!full) return NULL;
	if (w <= limit && h <= limit) {
		*width = w;
		*height = h;
		return full;
	}
	tw = w >= h ? limit : (int)((long)w * limit / h);
	th = h >= w ? limit : (int)((long)h * limit / w);
	if (tw < 1) tw = 1;
	if (th < 1) th = 1;
	scaled = (uint8_t*)rvn_alloc((size_t)tw * (size_t)th * 4u);
	for (y = 0; y < th; y++) {
		for (x = 0; x < tw; x++) {
			const uint8_t* from = full + ((size_t)(y * h / th) * (size_t)w + (size_t)(x * w / tw)) * 4u;
			memcpy(scaled + ((size_t)y * (size_t)tw + (size_t)x) * 4u, from, 4);
		}
	}
	Rpixels_free(full);
	*width = tw;
	*height = th;
	return scaled;
}

int rvn_win32_drag(HWND window, const RDataItem* items, int count, uint32_t operations,
	const uint8_t* rgba, int width, int height, int* operation) {

	rvn_win32_data_object_t* data;
	rvn_win32_drop_source_t* source;
	DWORD allowed = 0, effect = DROPEFFECT_NONE;
	HRESULT result;
	(void)window;
	*operation = R_DRAG_OPERATION_NONE;
	if (!g_rvn_ole) {
		HRESULT init = OleInitialize(NULL);
		/* (a thread already in the multithreaded apartment cannot drag) */
		if (FAILED(init)) return R_FALSE;
		g_rvn_ole = R_TRUE;
	}
	data = (rvn_win32_data_object_t*)rvn_calloc(sizeof(*data));
	data->iface.lpVtbl = &g_rvn_data_object_vtbl;
	data->refs = 1;
	rvn_win32_formats_from(&data->formats, items, count);
	if (!data->formats.count) {
		IDataObject_Release(&data->iface);
		return R_FALSE;
	}
	source = (rvn_win32_drop_source_t*)rvn_calloc(sizeof(*source));
	source->iface.lpVtbl = &g_rvn_drop_source_vtbl;
	source->refs = 1;

	if (rgba && width > 0 && height > 0) {
		rvn_win32_drag_image(&data->iface, rgba, width, height);
	} else {
		const RDataItem* png = rvn_data_find(items, count, "image/png");
		int w = 0, h = 0;
		uint8_t* thumbnail = png ? rvn_win32_thumbnail(png, 128, &w, &h) : NULL;
		if (thumbnail) {
			rvn_win32_drag_image(&data->iface, thumbnail, w, h);
			rvn_free(thumbnail);
		}
	}

	if (operations & R_DRAG_OPERATION_COPY) allowed |= DROPEFFECT_COPY;
	if (operations & R_DRAG_OPERATION_MOVE) allowed |= DROPEFFECT_MOVE;
	if (operations & R_DRAG_OPERATION_LINK) allowed |= DROPEFFECT_LINK;
	result = DoDragDrop(&data->iface, &source->iface, allowed, &effect);
	if (result == DRAGDROP_S_DROP) {
		/*
		 * A move the Explorer made itself (an "optimized move") reports no
		 * effect: what it really did is in what it gave the data object.
		 */
		FORMATETC logical;
		rvn_win32_format_t* told;
		memset(&logical, 0, sizeof(logical));
		logical.cfFormat = (CLIPFORMAT)RegisterClipboardFormatW(L"Logical Performed DropEffect");
		logical.dwAspect = DVASPECT_CONTENT;
		logical.lindex = -1;
		logical.tymed = TYMED_HGLOBAL;
		told = rvn_win32_data_find(data, &logical);
		if (effect == DROPEFFECT_NONE && told && told->size >= sizeof(DWORD)) memcpy(&effect, told->bytes, sizeof(DWORD));
		if (effect & DROPEFFECT_MOVE) *operation = R_DRAG_OPERATION_MOVE;
		else if (effect & DROPEFFECT_COPY) *operation = R_DRAG_OPERATION_COPY;
		else if (effect & DROPEFFECT_LINK) *operation = R_DRAG_OPERATION_LINK;
	}
	IDropSource_Release(&source->iface);
	IDataObject_Release(&data->iface);
	return SUCCEEDED(result) ? R_TRUE : R_FALSE;
}

void rvn_win32_transfer_shutdown(void) {
	if (g_rvn_ole) OleUninitialize();
	g_rvn_ole = R_FALSE;
}

#endif /* _WIN32 */
