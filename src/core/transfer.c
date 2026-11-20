/**
 * Raven - core/transfer.c
 * Data given to the other applications (RDataItem): the formats and the
 * file:// URLs of text/uri-list, shared by the platforms.
 */

#include "transfer.h"

int rvn_mime_is(const char* mime, const char* want) {
	size_t length, i;
	if (!mime || !want) return R_FALSE;
	while (*mime == ' ') mime++;
	for (length = 0; mime[length] && mime[length] != ';' && mime[length] != ' '; length++) {}
	while (*want == ' ') want++;
	for (i = 0; want[i] && want[i] != ';' && want[i] != ' '; i++) {}
	if (i != length) return R_FALSE;
	for (i = 0; i < length; i++) {
		char a = mime[i], b = want[i];
		if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
		if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
		if (a != b) return R_FALSE;
	}
	return R_TRUE;
}

const RDataItem* rvn_data_find(const RDataItem* items, int count, const char* mime) {
	int i;
	for (i = 0; items && i < count; i++) {
		if (rvn_mime_is(items[i].mime, mime)) return &items[i];
	}
	return NULL;
}

char* rvn_bytes_dup(const void* data, size_t size) {
	char* copy = (char*)rvn_alloc(size + 1);
	if (size && data) memcpy(copy, data, size);
	copy[size] = '\0';
	return copy;
}

RDataItem* rvn_data_copy(const RDataItem* items, int count, int* copied) {
	size_t bytes = 0;
	int i, kept = 0;
	RDataItem* copy;
	char* cursor;
	if (copied) *copied = 0;
	for (i = 0; items && i < count; i++) {
		if (!items[i].mime || !items[i].mime[0]) continue;
		bytes += sizeof(RDataItem) + strlen(items[i].mime) + 1 + (items[i].data ? items[i].size : 0) + 1;
		kept++;
	}
	if (!kept) return NULL;
	copy = (RDataItem*)rvn_alloc(bytes);
	cursor = (char*)(copy + kept);
	kept = 0;
	for (i = 0; i < count; i++) {
		size_t length, size;
		if (!items[i].mime || !items[i].mime[0]) continue;
		length = strlen(items[i].mime) + 1;
		size = items[i].data ? items[i].size : 0;
		memcpy(cursor, items[i].mime, length);
		copy[kept].mime = cursor;
		cursor += length;
		if (size) memcpy(cursor, items[i].data, size);
		cursor[size] = '\0';
		copy[kept].data = cursor;
		copy[kept].size = size;
		cursor += size + 1;
		kept++;
	}
	if (copied) *copied = kept;
	return copy;
}

static int rvn_hex_value(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

/* The local path of one file:// URL (`length` bytes), or NULL. */
static char* rvn_uri_to_path(const char* uri, size_t length) {
	const char* end = uri + length;
	const char* p;
	char* path;
	size_t used = 0;
	if (length < 7 || rvn_strnicmp(uri, "file://", 7) != 0) return NULL;
	p = uri + 7;
	/* file://localhost/path is file:///path; another host: a share of the network */
	if ((size_t)(end - p) >= 9 && rvn_strnicmp(p, "localhost", 9) == 0 && (p + 9 == end || p[9] == '/')) p += 9;
	path = (char*)rvn_alloc((size_t)(end - p) + 3);
	if (p < end && *p != '/') {
#if defined(_WIN32)
		path[used++] = '\\';
		path[used++] = '\\';
#else
		rvn_free(path);
		return NULL;
#endif
	}
#if defined(_WIN32)
	/* "/C:/folder" is "C:\folder" */
	if (end - p >= 3 && p[0] == '/' && ((p[1] >= 'A' && p[1] <= 'Z') || (p[1] >= 'a' && p[1] <= 'z')) && (p[2] == ':' || p[2] == '|')) p++;
#endif
	while (p < end) {
		char c = *p++;
		if (c == '%' && end - p >= 2 && rvn_hex_value(p[0]) >= 0 && rvn_hex_value(p[1]) >= 0) {
			c = (char)(rvn_hex_value(p[0]) * 16 + rvn_hex_value(p[1]));
			p += 2;
		}
#if defined(_WIN32)
		if (c == '/') c = '\\';
#endif
		path[used++] = c;
	}
	path[used] = '\0';
	if (!used || memchr(path, '\0', used)) {
		rvn_free(path);
		return NULL;
	}
	return path;
}

int rvn_uri_list_paths(const char* list, size_t size, char*** paths) {
	const char* end = list ? list + size : NULL;
	const char* line = list;
	char** found = NULL;
	uint32_t count = 0, cap = 0, i;
	size_t bytes = 0;
	char** block;
	char* cursor;
	*paths = NULL;
	while (line && line < end) {
		const char* stop = line;
		const char* next;
		while (stop < end && *stop != '\n' && *stop != '\r' && *stop != '\0') stop++;
		next = stop;
		while (next < end && (*next == '\n' || *next == '\r' || *next == '\0')) next++;
		while (line < stop && (*line == ' ' || *line == '\t')) line++;
		while (stop > line && (stop[-1] == ' ' || stop[-1] == '\t')) stop--;
		if (stop > line && *line != '#') {
			char* path = rvn_uri_to_path(line, (size_t)(stop - line));
			if (path) {
				RVN_ARRAY_RESERVE(found, cap, count + 1);
				found[count++] = path;
				bytes += strlen(path) + 1;
			}
		}
		line = next;
	}
	if (!count) {
		rvn_free(found);
		return 0;
	}
	block = (char**)rvn_alloc(sizeof(char*) * count + bytes);
	cursor = (char*)(block + count);
	for (i = 0; i < count; i++) {
		size_t length = strlen(found[i]) + 1;
		memcpy(cursor, found[i], length);
		block[i] = cursor;
		cursor += length;
		rvn_free(found[i]);
	}
	rvn_free(found);
	*paths = block;
	return (int)count;
}

void rvn_uri_list_add(rvn_text_t* list, const char* path) {
	static const char hex[] = "0123456789ABCDEF";
	const unsigned char* p = (const unsigned char*)path;
	if (!path || !path[0]) return;
	rvn_text_add(list, "file://");
#if defined(_WIN32)
	if ((p[0] == '\\' || p[0] == '/') && (p[1] == '\\' || p[1] == '/')) {
		/* \\server\share\file: file://server/share/file */
		p += 2;
	} else {
		rvn_text_add(list, "/");
	}
#endif
	for (; *p; p++) {
		unsigned char c = *p;
		char escaped[3];
#if defined(_WIN32)
		if (c == '\\') c = '/';
#endif
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
			c == '-' || c == '.' || c == '_' || c == '~' || c == '/' || c == ':') {
			rvn_text_append(list, (const char*)&c, 1);
			continue;
		}
		escaped[0] = '%';
		escaped[1] = hex[c >> 4];
		escaped[2] = hex[c & 15];
		rvn_text_append(list, escaped, 3);
	}
	rvn_text_add(list, "\r\n");
}
