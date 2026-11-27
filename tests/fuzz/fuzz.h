/**
 * Raven fuzzers - what the targets share.
 *
 * Each fuzz_*.c defines LLVMFuzzerTestOneInput. Built with
 * RAVEN_BUILD_FUZZERS=ON (clang), it is a libFuzzer program; otherwise
 * fuzz_main.c gives it a main() that runs the files of its corpus once,
 * so that every input kept in tests/fuzz/corpus is a test of every build
 * (ctest: fuzz_<name>_corpus).
 *
 * A target aborts when an invariant breaks: libFuzzer reports it like a
 * crash, with the input.
 */

#ifndef RAVEN_FUZZ_H
#define RAVEN_FUZZ_H

#include <Raven.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
int LLVMFuzzerInitialize(int* argc, char*** argv);
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);
#ifdef __cplusplus
}
#endif

/* The documents of the fuzzer are wrong on purpose: their errors are expected. */
static void fuzz_quiet_errors(RError code, RSeverity severity, const char* detail) {
	(void)code;
	(void)severity;
	(void)detail;
}

/* Starts the library once, without a window system and without a GPU (the targets that need it). */
static void fuzz_start_raven(void) {
	static int started = 0;
	RInitDesc desc;
	if (started) return;
	started = 1;
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_NULL;
	desc.headless = R_TRUE;
	desc.worker_threads = 0;
	Rerror_set_callback(fuzz_quiet_errors);
	if (!Rinit_ex(&desc)) {
		fprintf(stderr, "fuzz: cannot initialize Raven\n");
		abort();
	}
	Rset_animations_enabled(R_FALSE);
}

#define FUZZ_CHECK(condition) do { \
	if (!(condition)) { \
		fprintf(stderr, "fuzz: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
		abort(); \
	} \
} while (0)

#endif /* RAVEN_FUZZ_H */
