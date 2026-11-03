/**
 * Raven Web - web_browser.c
 * The browser process and the conversation with it.
 *
 * The DevTools protocol over a pipe: JSON messages, each ended by a zero
 * byte. Commands carry a number ("id") that their answer repeats; events
 * come unasked ("method"). Both name the page they are about ("sessionId").
 *
 * A thread reads. The pictures of the pages (Page.screencastFrame) are
 * decoded there, away from the UI thread, and acknowledged at once so that
 * the next one can come; everything else is handed over as text. The UI
 * thread is woken with Rtask_post and gives each thing to the views.
 */

#include "web_internal.h"

#include <stdio.h>
#include <stdlib.h>

char* g_rw_browser_path = NULL;
char* g_rw_profile_folder = NULL;
char* g_rw_arguments = NULL;

static struct {
	rw_process_t* process;
	rw_thread_t* reader;
	/** Guards the writes to the pipe, the numbers of the commands, and the list below. */
	rw_mutex_t* mutex;
	rw_incoming_t* first;
	rw_incoming_t* last;
	/** A task is on its way to the UI thread. */
	int posted;
	int next_id;
	char* profile;
	int profile_temporary;
	/** Where the browser writes its own log (in its profile). */
	char* log_path;
	char error[256];
} g_rw;

/* ---------------------------------------------------------------------------
 *  From the reading thread to the UI thread
 * ------------------------------------------------------------------------- */

static void rw_incoming_free(rw_incoming_t* incoming) {
	rw_free(incoming->message);
	rw_free(incoming->session);
	if (incoming->pixels) Rpixels_free(incoming->pixels);
	rw_free(incoming);
}

/* UI thread: gives the views what the browser said since the last time. */
static void rw_drain(void* unused) {
	rw_incoming_t* list;
	(void)unused;
	/* (stopped since the task was posted) */
	if (!g_rw.mutex) return;
	rw_mutex_lock(g_rw.mutex);
	list = g_rw.first;
	g_rw.first = g_rw.last = NULL;
	g_rw.posted = 0;
	rw_mutex_unlock(g_rw.mutex);
	while (list) {
		rw_incoming_t* next = list->next;
		/* (a view may stop the browser from here: the rest is then only released) */
		if (g_rw.mutex) rw_views_incoming(list);
		rw_incoming_free(list);
		list = next;
	}
}

/* Reading thread. */
static void rw_hand_over(rw_incoming_t* incoming) {
	int post;
	rw_mutex_lock(g_rw.mutex);
	if (incoming->pixels) {
		/* a picture the UI thread has not taken yet is replaced: only the last one matters */
		rw_incoming_t* waiting;
		for (waiting = g_rw.first; waiting; waiting = waiting->next) {
			if (waiting->pixels && strcmp(waiting->session, incoming->session) == 0) {
				uint8_t* old = waiting->pixels;
				waiting->pixels = incoming->pixels;
				waiting->width = incoming->width;
				waiting->height = incoming->height;
				incoming->pixels = old;
				rw_mutex_unlock(g_rw.mutex);
				rw_incoming_free(incoming);
				return;
			}
		}
	}
	incoming->next = NULL;
	if (g_rw.last) g_rw.last->next = incoming;
	else g_rw.first = incoming;
	g_rw.last = incoming;
	post = !g_rw.posted;
	g_rw.posted = 1;
	rw_mutex_unlock(g_rw.mutex);
	if (post) Rtask_post(rw_drain, NULL);
}

/* Reading thread: one message of the browser. The text is ours to change. */
static void rw_reader_message(char* text, size_t size) {
	static const char frame[] = "\"method\":\"Page.screencastFrame\"";
	rw_incoming_t* incoming = (rw_incoming_t*)rw_alloc(sizeof(rw_incoming_t));
	size_t head = size < 80 ? size : 80, i;
	int is_frame = 0;

	if (!incoming) return;
	memset(incoming, 0, sizeof(*incoming));
	for (i = 0; i + sizeof(frame) - 1 <= head && !is_frame; i++) is_frame = memcmp(text + i, frame, sizeof(frame) - 1) == 0;
	if (is_frame) {
		rw_json_document_t document;
		if (rw_json_parse(&document, text, size)) {
			const rw_json_t* data = rw_json_get(document.root, "params.data");
			const char* session = rw_json_string(document.root, "sessionId", NULL);
			char params[64];
			/* tell the browser at once that this one arrived: it sends the next when the page changes again */
			snprintf(params, sizeof(params), "{\"sessionId\":%d}", (int)rw_json_number(document.root, "params.sessionId", 0.0));
			rw_browser_send(session, "Page.screencastFrameAck", params);
			if (data && data->type == RW_JSON_STRING && session) {
				size_t bytes = 0;
				uint8_t* file = rw_base64_decode(data->string, data->length, &bytes);
				if (file) {
					incoming->pixels = Rimage_decode(file, bytes, &incoming->width, &incoming->height);
					rw_free(file);
				}
				if (incoming->pixels) incoming->session = rw_strdup(session);
			}
			rw_json_free(&document);
		}
		if (!incoming->pixels || !incoming->session) {
			rw_incoming_free(incoming);
			return;
		}
	} else {
		incoming->message = (char*)rw_alloc(size + 1);
		if (!incoming->message) {
			rw_free(incoming);
			return;
		}
		memcpy(incoming->message, text, size);
		incoming->message[size] = 0;
		incoming->size = size;
	}
	rw_hand_over(incoming);
}

static void rw_reader_main(void* unused) {
	char* buffer = NULL;
	size_t length = 0, capacity = 0;
	rw_incoming_t* closed;
	(void)unused;

	for (;;) {
		size_t scanned = length;
		char* end;
		int got;
		if (capacity - length < 65536) {
			size_t larger = capacity ? capacity * 2 : (size_t)1 << 18;
			char* grown = (char*)rw_realloc(buffer, larger);
			if (!grown) break;
			buffer = grown;
			capacity = larger;
		}
		got = rw_process_read(g_rw.process, buffer + length, capacity - length);
		if (got <= 0) break;
		length += (size_t)got;
		/* the messages that are complete now */
		while ((end = (char*)memchr(buffer + scanned, 0, length - scanned)) != NULL) {
			size_t size = (size_t)(end - buffer);
			if (size > 0) rw_reader_message(buffer, size);
			memmove(buffer, end + 1, length - size - 1);
			length -= size + 1;
			scanned = 0;
		}
	}
	rw_free(buffer);
	closed = (rw_incoming_t*)rw_alloc(sizeof(rw_incoming_t));
	if (closed) {
		memset(closed, 0, sizeof(*closed));
		closed->closed = 1;
		rw_hand_over(closed);
	}
}

/* ---------------------------------------------------------------------------
 *  Commands
 * ------------------------------------------------------------------------- */

int rw_browser_send(const char* session, const char* method, const char* params) {
	rw_text_t text;
	int id, sent;
	if (!g_rw.process || !g_rw.mutex) return 0;
	memset(&text, 0, sizeof(text));
	rw_mutex_lock(g_rw.mutex);
	id = ++g_rw.next_id;
	rw_text_printf(&text, "{\"id\":%d,\"method\":", id);
	rw_text_json(&text, method);
	if (params && params[0]) {
		rw_text_puts(&text, ",\"params\":");
		rw_text_puts(&text, params);
	}
	if (session && session[0]) {
		rw_text_puts(&text, ",\"sessionId\":");
		rw_text_json(&text, session);
	}
	rw_text_puts(&text, "}");
	/* (the zero that ends the text is the end of the message) */
	sent = text.data && rw_process_write(g_rw.process, text.data, text.length + 1);
	rw_mutex_unlock(g_rw.mutex);
	rw_text_free(&text);
	return sent ? id : 0;
}

/* ---------------------------------------------------------------------------
 *  Start and stop
 * ------------------------------------------------------------------------- */

const char* rw_browser_error(void) {
	return g_rw.error;
}

/*
 * The log of a browser that stopped by itself is kept out of its profile
 * (which goes away with it): "raven_web_browser.log" in the temporary folder.
 * Returns where it went, or "" when there was none.
 */
static const char* rw_keep_log(void) {
	static char kept[600];
	const char* base = getenv("TEMP");
	FILE* from;
	FILE* to;
	char buffer[8192];
	size_t got;
	kept[0] = 0;
	if (!g_rw.log_path || !(from = fopen(g_rw.log_path, "rb"))) return kept;
	if (!base || !base[0]) base = getenv("TMPDIR");
	if (!base || !base[0]) base = getenv("TMP");
#if defined(_WIN32)
	snprintf(kept, sizeof(kept), "%s\\raven_web_browser.log", base && base[0] ? base : ".");
#else
	snprintf(kept, sizeof(kept), "%s/raven_web_browser.log", base && base[0] ? base : "/tmp");
#endif
	to = fopen(kept, "wb");
	if (!to) {
		fclose(from);
		kept[0] = 0;
		return kept;
	}
	while ((got = fread(buffer, 1, sizeof(buffer), from)) > 0) fwrite(buffer, 1, got, to);
	fclose(from);
	fclose(to);
	return kept;
}

const char* rw_browser_stop_reason(void) {
	static char reason[900];
	const char* log;
	long code = 0;
	int length;
	length = snprintf(reason, sizeof(reason), "The browser stopped");
	/* (it closed its pipe: it is ending, or has ended) */
	if (g_rw.process && rw_process_exit_code(g_rw.process, 1500, &code)) {
#if defined(_WIN32)
		length += snprintf(reason + length, sizeof(reason) - (size_t)length, " (exit code %ld, 0x%08lX)", code, (unsigned long)code);
#else
		if (code < 0) length += snprintf(reason + length, sizeof(reason) - (size_t)length, " (signal %ld)", -code);
		else length += snprintf(reason + length, sizeof(reason) - (size_t)length, " (exit code %ld)", code);
#endif
	}
	/* what it said about it */
	log = rw_keep_log();
	if (log[0]) snprintf(reason + length, sizeof(reason) - (size_t)length, ". Its log: %s", log);
	else snprintf(reason + length, sizeof(reason) - (size_t)length, ".");
	fprintf(stderr, "[raven_web] %s\n", reason);
	return reason;
}

int rw_browser_running(void) {
	return g_rw.process != NULL;
}

/* The program to run: the one that was given, the one of the environment, one that is installed. To release with free. */
static char* rw_browser_program(void) {
	const char* environment = getenv("RAVEN_WEB_BROWSER");
	char* found;
	char* copy;
	if (g_rw_browser_path && g_rw_browser_path[0]) environment = g_rw_browser_path;
	if (environment && environment[0]) {
		copy = (char*)malloc(strlen(environment) + 1);
		if (copy) strcpy(copy, environment);
		return copy;
	}
	found = rw_find_browser();
	if (!found) return NULL;
	copy = (char*)malloc(strlen(found) + 1);
	if (copy) strcpy(copy, found);
	rw_free(found);
	return copy;
}

int rw_browser_start(void) {
	const char* arguments[64];
	char* extra = NULL;
	char* program;
	rw_text_t profile, log;
	int count = 0;

	if (g_rw.process) return 1;
	g_rw.error[0] = 0;
	program = rw_browser_program();
	if (!program || !rw_is_program(program)) {
		if (program) snprintf(g_rw.error, sizeof(g_rw.error), "The browser \"%.180s\" was not found.", program);
		else snprintf(g_rw.error, sizeof(g_rw.error), "No browser of the Chromium family (Edge, Chrome, Chromium) was found on this computer.");
		free(program);
		return 0;
	}

	if (g_rw_profile_folder && g_rw_profile_folder[0]) {
		g_rw.profile = rw_strdup(g_rw_profile_folder);
		g_rw.profile_temporary = 0;
	} else {
		g_rw.profile = rw_temp_folder();
		g_rw.profile_temporary = 1;
	}
	if (!g_rw.profile) {
		snprintf(g_rw.error, sizeof(g_rw.error), "No folder could be made for the profile of the browser.");
		free(program);
		return 0;
	}
	memset(&profile, 0, sizeof(profile));
	rw_text_printf(&profile, "--user-data-dir=%s", g_rw.profile);
	memset(&log, 0, sizeof(log));
	rw_free(g_rw.log_path);
	g_rw.log_path = NULL;
	{
		rw_text_t path;
		memset(&path, 0, sizeof(path));
#if defined(_WIN32)
		rw_text_printf(&path, "%s\\browser.log", g_rw.profile);
#else
		rw_text_printf(&path, "%s/browser.log", g_rw.profile);
#endif
		g_rw.log_path = path.data;
	}
	rw_text_printf(&log, "--log-file=%s", g_rw.log_path ? g_rw.log_path : "");

	arguments[count++] = program;
	/* hidden, driven through the pipe, with a profile that is not the one of the user */
	arguments[count++] = "--headless";
	arguments[count++] = "--remote-debugging-pipe";
	arguments[count++] = profile.data;
	/* its own log, in its profile: what it says when it stops by itself (rw_browser_stop_reason) */
	arguments[count++] = "--enable-logging";
	arguments[count++] = "--v=0";
	arguments[count++] = log.data;
	arguments[count++] = "--no-first-run";
	arguments[count++] = "--no-default-browser-check";
	arguments[count++] = "--disable-extensions";
	arguments[count++] = "--disable-component-update";
	arguments[count++] = "--disable-sync";
	arguments[count++] = "--disable-default-apps";
	/*
	 * It only talks to the sites the application shows: no updates, no
	 * reports, no account, no messaging service, no lists of dangerous sites,
	 * no clock, no warm-up of the search engine. (The services that have no
	 * switch are given an address where nobody answers.)
	 */
	arguments[count++] = "--disable-background-networking";
	arguments[count++] = "--disable-client-side-phishing-detection";
	arguments[count++] = "--disable-domain-reliability";
	arguments[count++] = "--disable-breakpad";
	arguments[count++] = "--metrics-recording-only";
	arguments[count++] = "--no-pings";
	arguments[count++] = "--no-service-autorun";
	arguments[count++] = "--disable-features=Translate,MediaRouter,OptimizationHints,PreconnectToSearch,NetworkTimeServiceQuerying,LensOverlay,"
		"AimServerEligibilityEnabled,AimServerRequestOnStartupEnabled,AutofillServerCommunication,CertificateTransparencyComponentUpdater";
	arguments[count++] = "--gaia-url=http://127.0.0.1:9";
	arguments[count++] = "--google-apis-url=http://127.0.0.1:9";
	arguments[count++] = "--lso-url=http://127.0.0.1:9";
	arguments[count++] = "--oauth-account-manager-url=http://127.0.0.1:9";
	arguments[count++] = "--gcm-checkin-url=http://127.0.0.1:9/";
	arguments[count++] = "--gcm-mcs-endpoint=127.0.0.1:9";
	arguments[count++] = "--gcm-registration-url=http://127.0.0.1:9/";
	arguments[count++] = "--password-store=basic";
	arguments[count++] = "--use-mock-keychain";
	arguments[count++] = "--window-size=1280,800";
#if !defined(_WIN32)
	/* containers give little shared memory */
	arguments[count++] = "--disable-dev-shm-usage";
#endif
	if (rw_needs_no_sandbox() || getenv("RAVEN_WEB_NO_SANDBOX")) arguments[count++] = "--no-sandbox";
	if ((g_rw_arguments && g_rw_arguments[0]) || getenv("RAVEN_WEB_ARGUMENTS")) {
		/* what the application adds (or, without it, the environment), cut at the spaces */
		char* cursor;
		extra = rw_strdup(g_rw_arguments && g_rw_arguments[0] ? g_rw_arguments : getenv("RAVEN_WEB_ARGUMENTS"));
		for (cursor = extra; cursor && *cursor && count < 60;) {
			while (*cursor == ' ') cursor++;
			if (!*cursor) break;
			arguments[count++] = cursor;
			while (*cursor && *cursor != ' ') cursor++;
			if (*cursor) *cursor++ = 0;
		}
	}
	arguments[count++] = "about:blank";
	arguments[count] = NULL;

	g_rw.mutex = rw_mutex_create();
	g_rw.process = g_rw.mutex ? rw_process_start(arguments) : NULL;
	if (g_rw.process) g_rw.reader = rw_thread_start(rw_reader_main, NULL);
	rw_text_free(&profile);
	rw_text_free(&log);
	rw_free(extra);
	if (!g_rw.process || !g_rw.reader) {
		snprintf(g_rw.error, sizeof(g_rw.error), "The browser \"%.180s\" could not be started.", program);
		free(program);
		rw_browser_stop();
		return 0;
	}
	free(program);
	g_rw.next_id = 0;
	/* to hear about the titles of the pages, and about the windows they open */
	rw_browser_send(NULL, "Target.setDiscoverTargets", "{\"discover\":true}");
	return 1;
}

void rw_browser_stop(void) {
	rw_incoming_t* list;
	if (g_rw.process) {
		rw_browser_send(NULL, "Browser.close", NULL);
		rw_process_hang_up(g_rw.process);
	}
	if (g_rw.reader) rw_thread_join(g_rw.reader);
	g_rw.reader = NULL;
	if (g_rw.process) rw_process_stop(g_rw.process);
	g_rw.process = NULL;
	list = g_rw.first;
	g_rw.first = g_rw.last = NULL;
	g_rw.posted = 0;
	while (list) {
		rw_incoming_t* next = list->next;
		rw_incoming_free(list);
		list = next;
	}
	if (g_rw.mutex) rw_mutex_destroy(g_rw.mutex);
	g_rw.mutex = NULL;
	if (g_rw.profile && g_rw.profile_temporary) rw_remove_folder(g_rw.profile);
	rw_free(g_rw.profile);
	g_rw.profile = NULL;
	rw_free(g_rw.log_path);
	g_rw.log_path = NULL;
}

/* ---------------------------------------------------------------------------
 *  Settings
 * ------------------------------------------------------------------------- */

/* (kept with malloc: they outlive Rshutdown, and are not a leak of the allocator of Raven) */
static void rw_setting(char** setting, const char* value) {
	free(*setting);
	*setting = NULL;
	if (value && value[0]) {
		*setting = (char*)malloc(strlen(value) + 1);
		if (*setting) strcpy(*setting, value);
	}
}

static char* g_rw_shown_path = NULL;

int Rweb_is_available(void) {
	char* program = rw_browser_program();
	int found = program && rw_is_program(program);
	free(program);
	return found;
}

void Rweb_set_browser_path(const char* path) {
	rw_setting(&g_rw_browser_path, path);
}

const char* Rweb_get_browser_path(void) {
	char* program = rw_browser_program();
	free(g_rw_shown_path);
	g_rw_shown_path = program && rw_is_program(program) ? program : NULL;
	if (!g_rw_shown_path) free(program);
	return g_rw_shown_path ? g_rw_shown_path : "";
}

void Rweb_set_profile_folder(const char* path) {
	rw_setting(&g_rw_profile_folder, path);
}

void Rweb_set_browser_arguments(const char* arguments) {
	rw_setting(&g_rw_arguments, arguments);
}

void Rweb_shutdown(void) {
	rw_incoming_t closed;
	if (g_rw.process) {
		/* the views learn that nothing will come any more */
		memset(&closed, 0, sizeof(closed));
		closed.closed = 2;
		rw_views_incoming(&closed);
	}
	rw_browser_stop();
	rw_setting(&g_rw_browser_path, NULL);
	rw_setting(&g_rw_profile_folder, NULL);
	rw_setting(&g_rw_arguments, NULL);
	free(g_rw_shown_path);
	g_rw_shown_path = NULL;
}
