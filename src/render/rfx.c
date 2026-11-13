/**
 * Raven - render/rfx.c
 * Effect bundles (rfx.h).
 */

#include "rfx.h"

static uint32_t rvn_rfx_u32(const uint8_t* at) {
	return (uint32_t)at[0] | ((uint32_t)at[1] << 8) | ((uint32_t)at[2] << 16) | ((uint32_t)at[3] << 24);
}

int rvn_rfx_is_bundle(const void* data, size_t size) {
	return data && size >= RVN_RFX_HEADER_SIZE && memcmp(data, "RFX1", 4) == 0;
}

int rvn_rfx_validate(const void* data, size_t size) {
	const uint8_t* bytes = (const uint8_t*)data;
	uint32_t count, i;
	if (!rvn_rfx_is_bundle(data, size)) return R_FALSE;
	count = rvn_rfx_u32(bytes + 8);
	if (count == 0 || count > RVN_RFX_MAX_CHUNKS) return R_FALSE;
	if (size < RVN_RFX_HEADER_SIZE + (size_t)count * RVN_RFX_CHUNK_SIZE) return R_FALSE;
	for (i = 0; i < count; i++) {
		const uint8_t* entry = bytes + RVN_RFX_HEADER_SIZE + (size_t)i * RVN_RFX_CHUNK_SIZE;
		uint32_t offset = rvn_rfx_u32(entry + 4), chunk_size = rvn_rfx_u32(entry + 8);
		/* (compared without adding: offset + size must not wrap) */
		if (offset > size || chunk_size > size - offset) return R_FALSE;
	}
	return R_TRUE;
}

Ruint rvn_rfx_flags(const void* data) {
	return (Ruint)rvn_rfx_u32((const uint8_t*)data + 4);
}

const void* rvn_rfx_chunk(const void* data, size_t size, const char tag[4], size_t* chunk_size) {
	const uint8_t* bytes = (const uint8_t*)data;
	uint32_t count, i;
	if (chunk_size) *chunk_size = 0;
	if (!rvn_rfx_validate(data, size)) return NULL;
	count = rvn_rfx_u32(bytes + 8);
	for (i = 0; i < count; i++) {
		const uint8_t* entry = bytes + RVN_RFX_HEADER_SIZE + (size_t)i * RVN_RFX_CHUNK_SIZE;
		if (memcmp(entry, tag, 4) != 0) continue;
		if (chunk_size) *chunk_size = rvn_rfx_u32(entry + 8);
		return bytes + rvn_rfx_u32(entry + 4);
	}
	return NULL;
}
