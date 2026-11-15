/**
 * Raven - core/handle.c
 */

#include "handle.h"

void rvn_pool_init(rvn_pool_t* pool, rvn_handle_type_t type) {
	memset(pool, 0, sizeof(*pool));
	pool->type = (uint32_t)type;
}

void rvn_pool_free(rvn_pool_t* pool) {
	rvn_free(pool->items);
	rvn_free(pool->generations);
	rvn_free(pool->free_list);
	memset(pool, 0, sizeof(*pool));
}

Rid rvn_pool_add(rvn_pool_t* pool, void* object) {
	uint32_t index;
	if (pool->free_count > 0) {
		index = pool->free_list[--pool->free_count];
	} else {
		if (pool->used > RVN_HANDLE_MAX_INDEX) {
			rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "handle pool exhausted (live or retired slots)");
			return R_INVALID_ID;
		}
		if (pool->used == pool->cap) {
			uint32_t old_cap = pool->cap;
			uint32_t new_cap = old_cap ? old_cap * 2 : 64;
			pool->items = (void**)rvn_realloc(pool->items, sizeof(void*) * new_cap);
			pool->generations = (uint8_t*)rvn_realloc(pool->generations, new_cap);
			memset(pool->items + old_cap, 0, sizeof(void*) * (new_cap - old_cap));
			memset(pool->generations + old_cap, 0, new_cap - old_cap);
			pool->cap = new_cap;
		}
		index = pool->used++;
	}
	pool->items[index] = object;
	pool->live++;
	return rvn_handle_make(pool->type, pool->generations[index], index);
}

void* rvn_pool_get(const rvn_pool_t* pool, Rid id) {
	uint32_t index = rvn_handle_index(id);
	if (rvn_handle_type(id) != pool->type || index >= pool->used) return NULL;
	if (pool->generations[index] != rvn_handle_generation(id)) return NULL;
	return pool->items[index];
}

void rvn_pool_remove(rvn_pool_t* pool, Rid id) {
	uint32_t index = rvn_handle_index(id);
	if (!rvn_pool_get(pool, id)) return;
	pool->items[index] = NULL;
	pool->generations[index]++;
	pool->live--;
	/* Retire the slot on wrap: no (index, generation) pair may be issued twice. */
	if (pool->generations[index] == 0) return;
	RVN_ARRAY_RESERVE(pool->free_list, pool->free_cap, pool->free_count + 1);
	pool->free_list[pool->free_count++] = index;
}

void* rvn_pool_at(const rvn_pool_t* pool, uint32_t index) {
	return index < pool->used ? pool->items[index] : NULL;
}
