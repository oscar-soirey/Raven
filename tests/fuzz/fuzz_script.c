/**
 * Raven fuzzers - Raven Script (src/script/rs_*.c): lexer, parser, type
 * checker, compiler, the virtual machine within its limits, the analysis of
 * the editor (symbols, completion) and the formatter.
 *
 * Each input is a script: compiled; when it compiles, installed, its
 * globals initialized and each of its functions called with default
 * arguments. The host knows no function (a call to one fails as an error
 * of the script).
 *
 * Invariants: the virtual machine ends every call with an empty stack; the
 * formatter's output formats to itself.
 */

#include "fuzz.h"
#include "../../src/script/rs_internal.h"

static rs_call_status fuzz_host_call(void* user_data, const char* name, const rs_value* args, int count, rs_value* result, rs_buf* error) {
	(void)user_data;
	(void)name;
	(void)args;
	(void)count;
	(void)result;
	(void)error;
	return RS_CALL_NOT_FOUND;
}

int LLVMFuzzerInitialize(int* argc, char*** argv) {
	(void)argc;
	(void)argv;
	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	const char* text = (const char*)data;
	rs_host host;
	rs_env* env;
	rs_batch* batch;
	char* json;
	char* formatted;

	memset(&host, 0, sizeof(host));
	host.call = fuzz_host_call;
	env = rs_env_create(&host);
	/* (small limits: a loop without end stops soon) */
	rs_env_set_limits(env, 20000, 30, 1u << 20);
	batch = rs_batch_create(env, RS_BATCH_ANALYSIS);
	rs_batch_add(batch, "fuzz.rvs", text, size, 1, 1);
	if (rs_batch_compile(batch) == 0 && rs_batch_install(batch)) {
		int i;
		rs_batch_initialize(batch);
		for (i = 0; i < rs_env_function_count(env); i++) {
			rs_value result;
			rs_value args[RS_MAX_PARAMS];
			rs_sig sig;
			int p;
			const char* name = rs_env_function_name(env, i);
			if (!rs_env_find_function(env, name, &sig)) continue;
			for (p = 0; p < sig.count; p++) args[p] = rs_value_default(NULL, sig.params[p]);
			rs_env_call(env, name, args, sig.count, RS_CALL_STRICT, NULL, &result);
			rs_value_release(result);
			for (p = 0; p < sig.count; p++) rs_value_release(args[p]);
			FUZZ_CHECK(env->stack_top == 0 && env->frame_count == 0 && env->depth == 0);
		}
	}
	json = rs_batch_analysis_json(batch);
	rs_free(json);
	json = rs_batch_complete_json(batch, 0, (uint32_t)(size ? data[0] % (size + 1) : 0));
	rs_free(json);
	rs_batch_destroy(batch);
	rs_env_destroy(env);

	formatted = rs_format(text, size, NULL);
	if (formatted) {
		char* again = rs_format(formatted, strlen(formatted), NULL);
		FUZZ_CHECK(again != NULL);
		FUZZ_CHECK(strcmp(again, formatted) == 0);
		rs_free(again);
		rs_free(formatted);
	}
	return 0;
}
