/**
 * Raven - core/handle.h
 * Generational handle pools: Rid <-> object pointer.
 *
 * Handle layout (32 bits):   [ type : 4 | generation : 8 | index : 20 ]
 *
 * The type tag makes a font handle invalid where a widget is expected, the
 * generation makes a deleted handle invalid even after its slot is reused.
 * A slot is permanently retired when its generation wraps, rather than reused.
 * Type 0 and type 15 are never used, so 0 and R_INVALID_ID never resolve.
 */

#ifndef RVN_HANDLE_H
#define RVN_HANDLE_H

#include "base.h"

typedef enum rvn_handle_type_t {
	RVN_HANDLE_WINDOW = 1,
	RVN_HANDLE_WIDGET,
	RVN_HANDLE_FONT,
	RVN_HANDLE_TEXTURE,
	RVN_HANDLE_TIMER,
	RVN_HANDLE_TREE_ITEM,
	RVN_HANDLE_XML_DOCUMENT,
	RVN_HANDLE_XML_NODE,
	/** Pseudo-handle naming a glyph atlas page inside draw lists (never in a pool). */
	RVN_HANDLE_ATLAS_PAGE,
	/** A fragment shader of the application or a built-in effect (raven_shader.h). */
	RVN_HANDLE_SHADER,
	/** An icon beside the clock (raven_shell.h, platform/tray.c). */
	RVN_HANDLE_TRAY,
	/** A PDF document written by the application (widgets/pdf_document.c). */
	RVN_HANDLE_PDF_DOCUMENT
} rvn_handle_type_t;

#define RVN_HANDLE_INDEX_BITS 20u
#define RVN_HANDLE_INDEX_MASK 0xFFFFFu
#define RVN_HANDLE_MAX_INDEX  RVN_HANDLE_INDEX_MASK

RVN_INLINE uint32_t rvn_handle_index(Rid id) { return id & RVN_HANDLE_INDEX_MASK; }
RVN_INLINE uint32_t rvn_handle_generation(Rid id) { return (id >> RVN_HANDLE_INDEX_BITS) & 0xFFu; }
RVN_INLINE uint32_t rvn_handle_type(Rid id) { return id >> 28; }
RVN_INLINE Rid rvn_handle_make(uint32_t type, uint32_t generation, uint32_t index) {
	return (type << 28) | ((generation & 0xFFu) << RVN_HANDLE_INDEX_BITS) | (index & RVN_HANDLE_INDEX_MASK);
}

typedef struct rvn_pool_t {
	void** items;
	uint8_t* generations;
	uint32_t* free_list;
	uint32_t cap;
	/** Number of slots ever used (high-water mark). */
	uint32_t used;
	uint32_t free_count;
	uint32_t free_cap;
	uint32_t live;
	uint32_t type;
} rvn_pool_t;

void   rvn_pool_init(rvn_pool_t* pool, rvn_handle_type_t type);
void   rvn_pool_free(rvn_pool_t* pool);
/** Registers an object and returns its handle (R_INVALID_ID when the pool is full). */
Rid rvn_pool_add(rvn_pool_t* pool, void* object);
/** Returns the object, or NULL when the handle is stale, deleted or of another type. */
void*  rvn_pool_get(const rvn_pool_t* pool, Rid id);
void   rvn_pool_remove(rvn_pool_t* pool, Rid id);
/** Iteration over live objects: returns the object at slot `index` or NULL. */
void*  rvn_pool_at(const rvn_pool_t* pool, uint32_t index);

#endif /* RVN_HANDLE_H */
