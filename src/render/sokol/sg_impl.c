/**
 * Raven - render/sokol/sg_impl.c
 * The one translation unit that compiles sokol_gfx.h. Its functions are renamed (sg_prefix.h) so that
 * an application linking its own sokol_gfx cannot clash with this copy.
 *
 * The graphics API is chosen when Raven is built (RAVEN_SOKOL_API), not at run time: sokol_gfx
 * compiles one backend per library.
 */

#include "sg_prefix.h"

#define SOKOL_IMPL
#if defined(RAVEN_SOKOL_GLES3)
	#define SOKOL_GLES3
#elif defined(RAVEN_SOKOL_METAL)
	#define SOKOL_METAL
#elif defined(RAVEN_SOKOL_D3D11)
	#define SOKOL_D3D11
#elif defined(RAVEN_SOKOL_WGPU)
	#define SOKOL_WGPU
#else
	#error "Raven: no sokol graphics API selected (RAVEN_SOKOL_API)"
#endif
#include "../../../third_party/sokol/sokol_gfx.h"

/*
 * Log items that are not a problem, known here because the ids of sokol_gfx are only public in its
 * implementation: the GL compiler drops the samplers an effect does not use, and sokol_gfx then warns that
 * their name was not found (effect bundles will record which samplers a variant uses).
 */
int rvn_sg_log_is_harmless(uint32_t item) {
#if defined(RAVEN_SOKOL_GLES3)
	return item == SG_LOGITEM_GL_IMAGE_SAMPLER_NAME_NOT_FOUND_IN_SHADER;
#else
	(void)item;
	return 0;
#endif
}

