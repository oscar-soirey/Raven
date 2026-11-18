/**
 * Raven - core/error.c
 * Last-error record and error callback. Usable before Rinit and from any thread.
 */

#include "thread.h"

/* (the diagnostics of the scripts are a few lines, with excerpts of the source) */
#define RVN_ERROR_TEXT_SIZE 4096

static rvn_atomic_t g_error_lock;
static RError g_error_code = R_NO_ERROR;
static RSeverity g_error_severity = R_SEVERITY_WEAK_WARNING;
static char g_error_detail[RVN_ERROR_TEXT_SIZE];
static char g_error_returned[RVN_ERROR_TEXT_SIZE];
static RErrorCallback g_error_callback;

/* A spin lock: errors are rare and this must work with no initialization. */
static void rvn_error_lock(void) {
	while (rvn_atomic_exchange(&g_error_lock, 1) != 0) {}
}

static void rvn_error_unlock(void) {
	rvn_atomic_store(&g_error_lock, 0);
}

void rvn_error(RError code, RSeverity severity, const char* format, ...) {
	char text[RVN_ERROR_TEXT_SIZE];
	RErrorCallback callback;
	va_list args;

	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);

	rvn_error_lock();
	g_error_code = code;
	g_error_severity = severity;
	memcpy(g_error_detail, text, sizeof(text));
	callback = g_error_callback;
	rvn_error_unlock();

	if (callback) callback(code, severity, text);
}

RError Rerror_get_last(const char** detail, RSeverity* severity) {
	RError code;
	rvn_error_lock();
	code = g_error_code;
	if (severity) *severity = g_error_severity;
	memcpy(g_error_returned, g_error_detail, sizeof(g_error_returned));
	g_error_code = R_NO_ERROR;
	g_error_detail[0] = '\0';
	rvn_error_unlock();
	if (detail) *detail = g_error_returned;
	return code;
}

void Rerror_set_callback(RErrorCallback callback) {
	rvn_error_lock();
	g_error_callback = callback;
	rvn_error_unlock();
}

const char* Rerror_to_string(RError error) {
	switch (error) {
		case R_NO_ERROR: return "R_NO_ERROR";
		case R_ERROR_INVALID_ID: return "R_ERROR_INVALID_ID";
		case R_ERROR_INVALID_ENUM: return "R_ERROR_INVALID_ENUM";
		case R_ERROR_INVALID_VALUE: return "R_ERROR_INVALID_VALUE";
		case R_ERROR_INVALID_OPERATION: return "R_ERROR_INVALID_OPERATION";
		case R_ERROR_BACKEND: return "R_ERROR_BACKEND";
		case R_ERROR_PLATFORM: return "R_ERROR_PLATFORM";
		case R_ERROR_OUT_OF_MEMORY: return "R_ERROR_OUT_OF_MEMORY";
		case R_ERROR_INVALID_FILE_FORMAT: return "R_ERROR_INVALID_FILE_FORMAT";
		case R_ERROR_FILE_NOT_FOUND: return "R_ERROR_FILE_NOT_FOUND";
		case R_ERROR_XML: return "R_ERROR_XML";
		case R_ERROR_SCRIPT: return "R_ERROR_SCRIPT";
		case R_ERROR_WRONG_THREAD: return "R_ERROR_WRONG_THREAD";
	}
	return "RVN_ERROR_UNKNOWN";
}

const char* Rseverity_to_string(RSeverity severity) {
	switch (severity) {
		case R_SEVERITY_WEAK_WARNING: return "R_SEVERITY_WEAK_WARNING";
		case R_SEVERITY_WARNING: return "R_SEVERITY_WARNING";
		case R_SEVERITY_ERROR: return "R_SEVERITY_ERROR";
		case R_SEVERITY_FATAL: return "R_SEVERITY_FATAL";
	}
	return "RVN_SEVERITY_UNKNOWN";
}
