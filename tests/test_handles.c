/* Generational handles must never name a second object after slot reuse. */
#include "test.h"
#include "../src/core/handle.h"

static void test_pool_retirement(void) {
	int object, type, generation;
	for (type = RVN_HANDLE_WINDOW; type <= RVN_HANDLE_TRAY; type++) {
		rvn_pool_t pool;
		Rid stale[256], live;
		rvn_pool_init(&pool, (rvn_handle_type_t)type);
		for (generation = 0; generation < 256; generation++) {
			stale[generation] = rvn_pool_add(&pool, &object);
			CHECK(rvn_handle_index(stale[generation]) == 0);
			CHECK(rvn_handle_generation(stale[generation]) == (uint32_t)generation);
			CHECK(rvn_pool_get(&pool, stale[generation]) == &object);
			rvn_pool_remove(&pool, stale[generation]);
			CHECK(pool.live == 0);
		}
		live = rvn_pool_add(&pool, &object);
		CHECK(rvn_handle_index(live) == 1);
		CHECK(pool.live == 1 && pool.used == 2 && pool.free_count == 0);
		CHECK(rvn_pool_at(&pool, 0) == NULL);
		for (generation = 0; generation < 256; generation++) {
			CHECK(rvn_pool_get(&pool, stale[generation]) == NULL);
			rvn_pool_remove(&pool, stale[generation]); /* stale removes cannot retire the new object */
		}
		CHECK(rvn_pool_get(&pool, live) == &object && pool.live == 1);
		rvn_pool_remove(&pool, live);
		CHECK(pool.live == 0 && pool.free_count == 1);
		CHECK(rvn_handle_index(rvn_pool_add(&pool, &object)) == 1);
		rvn_pool_free(&pool);
	}
}

static void test_pool_exhaustion(void) {
	rvn_pool_t pool;
	int object;
	Rid live;
	rvn_pool_init(&pool, RVN_HANDLE_WIDGET);
	live = rvn_pool_add(&pool, &object);
	rvn_pool_remove(&pool, live);
	/* Simulate the index limit without allocating a million entries. Free slots remain usable. */
	pool.used = RVN_HANDLE_MAX_INDEX + 1;
	live = rvn_pool_add(&pool, &object);
	CHECK(rvn_handle_index(live) == 0 && pool.live == 1);
	EXPECT_ERRORS_BEGIN();
	CHECK(rvn_pool_add(&pool, &object) == R_INVALID_ID);
	EXPECT_ERRORS_END(1);
	CHECK(pool.live == 1 && rvn_pool_get(&pool, live) == &object);
	rvn_pool_free(&pool);
}

static void test_widget_churn(void) {
	Rid victim = Rwidget_create(R_WIDGET_LABEL);
	int i;
	Rwidget_destroy(victim);
	for (i = 0; i < 1024; i++) {
		Rid live = Rwidget_create(R_WIDGET_LABEL);
		CHECK(Rwidget_is_valid(live));
		CHECK(!Rwidget_is_valid(victim));
		Rwidget_destroy(live);
	}
}

int main(void) {
	RInitDesc desc = {0};
	desc.backend = R_BACKEND_NULL;
	desc.headless = R_TRUE;
	desc.worker_threads = -1;
	Rerror_set_callback(test_error_callback);
	if (!Rinit_ex(&desc)) return 1;
	RUN(test_pool_retirement);
	RUN(test_pool_exhaustion);
	RUN(test_widget_churn);
	Rshutdown();
	return test_report("test_handles");
}
