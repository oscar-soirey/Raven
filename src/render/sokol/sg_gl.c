/**
 * Raven - render/sokol/sg_gl.c
 * What the GLES 3.0 context needs besides its creation: waiting for the GPU and reading an image back. Shared by
 * the contexts of sg_egl.c (Linux) and sg_web.c (WebGL 2 in a browser).
 */

#include "sg_backend.h"

#include <GLES3/gl3.h>

void rvn_sg_context_finish(void) {
	glFinish();
}

uint8_t* rvn_sg_read_target(const rvn_sg_target_t* target) {
	sg_gl_image_info info = sg_gl_query_image_info(target->image);
	size_t count = (size_t)target->width * (size_t)target->height;
	uint8_t* pixels = Rpixels_alloc(count * 4);
	GLuint framebuffer = 0;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, info.tex[info.active_slot], 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	/* the UI top is framebuffer row 0 (see sg_backend.h): the rows come out top first */
	glReadPixels(0, 0, target->width, target->height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &framebuffer);
	/* sokol caches the GL state: tell it the framebuffer binding changed behind its back */
	sg_reset_state_cache();
	return pixels;
}

void* rvn_sg_pool_push(void) {
	return NULL;
}

void rvn_sg_pool_pop(void* pool) {
	RVN_UNUSED(pool);
}
