/**
 * raven_script: Raven Script on the command line.
 *
 *     raven_script check FILE...            checks scripts (.rvs) and the scripts of XML documents
 *     raven_script format [--write] [--indent N|tab] FILE...
 *                                           formats scripts, and the <Script> elements of XML documents
 *     raven_script run FILE [FUNCTION]      loads a script and calls a function (app.main by default)
 *
 * It uses the library itself (without a window: the null backend), so it
 * reports exactly what an application would. Exit code: 0 when everything
 * is fine, 1 when there are errors, 2 for a wrong command line.
 */

#include <Raven.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_errors = 0;
static int g_warnings = 0;

static void on_diagnostic(RSeverity severity, const char* code, const char* file, int line, int column, const char* text, void* user_data) {
	(void)code;
	(void)file;
	(void)line;
	(void)column;
	(void)user_data;
	if (severity == R_SEVERITY_ERROR || severity == R_SEVERITY_FATAL) g_errors++;
	else if (severity == R_SEVERITY_WARNING) g_warnings++;
	fprintf(stderr, "%s\n\n", text);
}

static void on_error(RError code, RSeverity severity, const char* detail) {
	if (code == R_ERROR_SCRIPT) return;
	if (severity == R_SEVERITY_ERROR || severity == R_SEVERITY_FATAL) g_errors++;
	else g_warnings++;
	fprintf(stderr, "%s: %s\n\n", severity == R_SEVERITY_WARNING || severity == R_SEVERITY_WEAK_WARNING ? "warning" : "error", detail);
}

static void on_log(const char* message, void* user_data) {
	(void)user_data;
	printf("%s\n", message);
	fflush(stdout);
}

static int usage(void) {
	fprintf(stderr,
		"Raven Script %s\n"
		"\n"
		"usage:\n"
		"  raven_script check FILE...                        check scripts (.rvs) and XML documents\n"
		"  raven_script format [--write] [--indent N|tab] FILE...\n"
		"                                                    format scripts and the <Script> of XML documents\n"
		"                                                    (printed, or written back with --write)\n"
		"  raven_script run FILE [FUNCTION]                  load a script, call FUNCTION (app.main)\n",
		Rget_version());
	return 2;
}

static char* read_file(const char* path, size_t* size) {
	FILE* file = fopen(path, "rb");
	char* data;
	long length;
	if (!file) return NULL;
	if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return NULL;
	}
	data = (char*)malloc((size_t)length + 1);
	if (!data) {
		fclose(file);
		return NULL;
	}
	*size = fread(data, 1, (size_t)length, file);
	data[*size] = '\0';
	fclose(file);
	return data;
}

static int write_file(const char* path, const char* data, size_t size) {
	FILE* file = fopen(path, "wb");
	int ok;
	if (!file) return 0;
	ok = fwrite(data, 1, size, file) == size;
	return fclose(file) == 0 && ok;
}

static int is_xml(const char* text, size_t size) {
	size_t i = 0;
	if (size >= 3 && memcmp(text, "\xEF\xBB\xBF", 3) == 0) i = 3;
	while (i < size && (text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\n')) i++;
	return i < size && text[i] == '<';
}

/* ---------------------------------------------------------------------------
 *  format
 * ------------------------------------------------------------------------- */

typedef struct text_t {
	char* data;
	size_t length;
	size_t capacity;
} text_t;

static void text_put(text_t* text, const char* bytes, size_t count) {
	if (text->length + count + 1 > text->capacity) {
		size_t capacity = text->capacity ? text->capacity : 1024;
		while (capacity < text->length + count + 1) capacity *= 2;
		text->data = (char*)realloc(text->data, capacity);
		text->capacity = capacity;
	}
	memcpy(text->data + text->length, bytes, count);
	text->length += count;
	text->data[text->length] = '\0';
}

static int ascii_lower(int c) {
	return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
}

static int starts_with_tag(const char* p, const char* end, const char* tag) {
	size_t i, length = strlen(tag);
	if ((size_t)(end - p) < length) return 0;
	for (i = 0; i < length; i++) {
		if (ascii_lower((unsigned char)p[i]) != ascii_lower((unsigned char)tag[i])) return 0;
	}
	return 1;
}

/* The <Script> elements of an XML document, formatted in place (indented one level under their tag). */
static char* format_xml(const char* source, size_t size, const char* indent, size_t* out_size, int* failed) {
	text_t out = { 0 };
	const char* p = source;
	const char* end = source + size;
	const char* copied = source;
	while (p < end) {
		const char* open;
		const char* body;
		const char* close;
		const char* line_start;
		char prefix[256];
		size_t prefix_length = 0;
		/* skip comments */
		if (end - p >= 4 && memcmp(p, "<!--", 4) == 0) {
			const char* stop = strstr(p + 4, "-->");
			p = stop ? stop + 3 : end;
			continue;
		}
		if (!(starts_with_tag(p, end, "<script") && p + 7 < end && (p[7] == '>' || p[7] == ' ' || p[7] == '\t' || p[7] == '\n' || p[7] == '\r'))) {
			p++;
			continue;
		}
		open = p;
		body = memchr(p, '>', (size_t)(end - p));
		if (!body) break;
		if (body[-1] == '/') {
			/* <Script Src="..."/> */
			p = body + 1;
			continue;
		}
		body++;
		close = body;
		while (close < end && !starts_with_tag(close, end, "</script")) close++;
		if (close >= end) break;
		/* the indentation of the tag, plus one level */
		line_start = open;
		while (line_start > source && line_start[-1] != '\n') line_start--;
		while (line_start < open && (*line_start == ' ' || *line_start == '\t') && prefix_length + 1 < sizeof(prefix)) prefix[prefix_length++] = *line_start++;
		prefix[prefix_length] = '\0';
		{
			char inner_prefix[300];
			const char* formatted;
			const char* script = body;
			size_t script_length = (size_t)(close - body);
			/* a CDATA section around the code: kept around the formatted code */
			const char* cdata_start = NULL;
			const char* cdata_end = NULL;
			{
				const char* a = body;
				const char* b = close;
				while (a < b && (*a == ' ' || *a == '\t' || *a == '\r' || *a == '\n')) a++;
				while (b > a && (b[-1] == ' ' || b[-1] == '\t' || b[-1] == '\r' || b[-1] == '\n')) b--;
				if (b - a >= 12 && memcmp(a, "<![CDATA[", 9) == 0 && memcmp(b - 3, "]]>", 3) == 0) {
					cdata_start = a;
					cdata_end = b;
					script = a + 9;
					script_length = (size_t)(b - 3 - script);
				}
			}
			snprintf(inner_prefix, sizeof(inner_prefix), "%s%s", prefix, indent ? indent : "    ");
			formatted = Rscript_format(script, script_length, indent);
			if (!formatted) {
				*failed = 1;
				p = close;
				continue;
			}
			text_put(&out, copied, (size_t)(body - copied));
			text_put(&out, "\n", 1);
			if (cdata_start) {
				text_put(&out, inner_prefix, strlen(inner_prefix));
				text_put(&out, "<![CDATA[\n", 10);
			}
			{
				/* each line of the code under the tag */
				const char* line = formatted;
				while (*line) {
					const char* stop = strchr(line, '\n');
					size_t length = stop ? (size_t)(stop - line) : strlen(line);
					if (length > 0) {
						text_put(&out, inner_prefix, strlen(inner_prefix));
						text_put(&out, line, length);
					}
					text_put(&out, "\n", 1);
					if (!stop) break;
					line = stop + 1;
				}
			}
			if (cdata_end) {
				text_put(&out, inner_prefix, strlen(inner_prefix));
				text_put(&out, "]]>\n", 4);
			}
			text_put(&out, prefix, prefix_length);
			copied = close;
			p = close;
		}
	}
	text_put(&out, copied, (size_t)(end - copied));
	*out_size = out.length;
	return out.data;
}

static int command_format(int argc, char** argv) {
	int write = 0, i, status = 0, files = 0;
	const char* indent = NULL;
	char spaces[17];
	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--write") == 0 || strcmp(argv[i], "-w") == 0) write = 1;
		else if (strcmp(argv[i], "--indent") == 0 && i + 1 < argc) {
			const char* value = argv[++i];
			if (strcmp(value, "tab") == 0) indent = "\t";
			else {
				int n = atoi(value);
				if (n < 1 || n > 16) return usage();
				memset(spaces, ' ', (size_t)n);
				spaces[n] = '\0';
				indent = spaces;
			}
		}
	}
	for (i = 0; i < argc; i++) {
		size_t size = 0, out_size = 0;
		char* text;
		char* result = NULL;
		int failed = 0;
		if (argv[i][0] == '-') {
			if (strcmp(argv[i], "--indent") == 0) i++;
			continue;
		}
		files++;
		text = read_file(argv[i], &size);
		if (!text) {
			fprintf(stderr, "error: cannot read '%s'\n", argv[i]);
			status = 1;
			continue;
		}
		if (is_xml(text, size)) {
			result = format_xml(text, size, indent, &out_size, &failed);
		} else {
			const char* formatted = Rscript_format(text, size, indent);
			if (formatted) {
				out_size = strlen(formatted);
				result = (char*)malloc(out_size + 1);
				memcpy(result, formatted, out_size + 1);
			} else {
				failed = 1;
			}
		}
		if (failed) {
			fprintf(stderr, "error: '%s' has a syntax error: run raven_script check on it\n", argv[i]);
			status = 1;
		}
		if (result && !failed) {
			if (write) {
				if (out_size != size || memcmp(result, text, size) != 0) {
					if (!write_file(argv[i], result, out_size)) {
						fprintf(stderr, "error: cannot write '%s'\n", argv[i]);
						status = 1;
					}
				}
			} else {
				fwrite(result, 1, out_size, stdout);
			}
		}
		free(result);
		free(text);
	}
	if (files == 0) return usage();
	return status;
}

/* ---------------------------------------------------------------------------
 *  main
 * ------------------------------------------------------------------------- */

int main(int argc, char** argv) {
	RInitDesc desc;
	int status = 0, i;
	if (argc < 2) return usage();
	if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		usage();
		return 0;
	}
	if (strcmp(argv[1], "--version") == 0) {
		printf("raven_script %s\n", Rget_version());
		return 0;
	}
	if (strcmp(argv[1], "check") != 0 && strcmp(argv[1], "format") != 0 && strcmp(argv[1], "run") != 0) return usage();
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_NULL;
	desc.headless = 1;
	Rerror_set_callback(on_error);
	if (!Rinit_ex(&desc)) {
		fprintf(stderr, "error: the library could not start\n");
		return 1;
	}
	Rscript_set_diagnostic_callback(on_diagnostic, NULL);
	Rscript_set_log_callback(on_log, NULL);
	if (strcmp(argv[1], "check") == 0) {
		if (argc < 3) status = usage();
		for (i = 2; i < argc; i++) {
			int errors = Rscript_check_file(argv[i]);
			if (errors != 0) status = 1;
		}
		if (status == 0 && argc >= 3) fprintf(stderr, "%d file%s checked: no error%s.\n", argc - 2, argc - 2 == 1 ? "" : "s", g_warnings ? "" : ", no warning");
		else if (status == 1) fprintf(stderr, "%d error%s, %d warning%s.\n", g_errors, g_errors == 1 ? "" : "s", g_warnings, g_warnings == 1 ? "" : "s");
	} else if (strcmp(argv[1], "format") == 0) {
		status = command_format(argc - 2, argv + 2);
	} else {
		const char* function = argc >= 4 ? argv[3] : "app.main";
		if (argc < 3) status = usage();
		else if (!Rscript_load_file(argv[2])) status = 1;
		else if (!Rscript_has_function(function)) {
			fprintf(stderr, "error: '%s' has no function '%s'\n", argv[2], function);
			status = 1;
		} else {
			Raction_fire(function, R_INVALID_ID);
			if (g_errors) status = 1;
		}
	}
	Rshutdown();
	return status;
}
