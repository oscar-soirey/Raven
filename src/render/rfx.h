/**
 * Raven - render/rfx.h
 * Effect bundles (.rfx): one effect compiled for every graphics API, in one file (tools/rfx/rfx.py makes them).
 *
 *   offset  size
 *   0       4     "RFX1"
 *   4       4     flags (RShaderFlags: backdrop, animated), little endian
 *   8       4     number of chunks (at most 16)
 *   12      16 each: tag (4 characters), offset, size, reserved (little endian); then the chunks, 4-byte aligned
 *
 * Chunks: "SPV " the SPIR-V of the effect (the Vulkan backend), "ES30" its GLSL ES 3.00 fragment shader (GLES 3, WebGL 2),
 * "MSL " its Metal Shading Language fragment shader (entry point main0). Later: "HLSL", "WGSL", "GL41". A backend reads the
 * chunk of its own API; a bundle without it cannot be used with that backend.
 *
 * Rshader_create and Rshader_create_from_file take a bundle wherever they take SPIR-V.
 */

#ifndef RVN_RFX_H
#define RVN_RFX_H

#include "../core/base.h"

#define RVN_RFX_HEADER_SIZE 12
#define RVN_RFX_CHUNK_SIZE 16
#define RVN_RFX_MAX_CHUNKS 16

/** R_TRUE when the data starts like a bundle. */
int rvn_rfx_is_bundle(const void* data, size_t size);
/** Checks the structure of a bundle (every chunk inside the data). R_FALSE: damaged. */
int rvn_rfx_validate(const void* data, size_t size);
/** The flags (RShaderFlags) of a validated bundle. */
Ruint rvn_rfx_flags(const void* data);
/** A chunk of a validated bundle by its tag ("SPV ", "ES30", "MSL "), or NULL; *chunk_size is its size. */
const void* rvn_rfx_chunk(const void* data, size_t size, const char tag[4], size_t* chunk_size);

#endif /* RVN_RFX_H */
