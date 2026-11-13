/**
 * Raven - render/sokol/sg_backend.c
 * The RBackendVTable of the sokol_gfx backend: surfaces and frames.
 *
 * Status (docs/SOKOL_BACKEND_PLAN.md): the frame of a window is rendered into an offscreen image that
 * Rwindow_capture reads back, which is what headless runs and tests need. Presenting to the window
 * (swapchains, vsync, the flip on GL) is phase 3; viewport draw callbacks (R_DRAW_CALLBACK) are phase 5.
 *
 * A frame is recorded like the Vulkan one (vulkan/vk_frame.c): one instanced draw per command, in one
 * pass; a frame with glass goes to the offscreen scene, with a pause at each backdrop command.
 */

#include "sg_backend.h"

rvn_sg_state_t g_sg;

bool rvn_sg_origin(void) {
	/* false on GL, true on Metal / D3D11 / WebGPU */
	return sg_query_features().origin_top_left;
}

bool rvn_sg_flip_y(void) {
	return rvn_sg_origin();
}

static void rvn_sg_log(const char* tag, uint32_t level, uint32_t item, const char* message, uint32_t line, const char* file, void* user) {
	RVN_UNUSED(tag);
	RVN_UNUSED(line);
	RVN_UNUSED(file);
	RVN_UNUSED(user);
	/* errors and warnings only (level 3 is information); some warnings are not a problem */
	if (!message || level >= 3 || rvn_sg_log_is_harmless(item)) return;
	rvn_error(R_ERROR_BACKEND, level <= 1 ? R_SEVERITY_ERROR : R_SEVERITY_WARNING, "sokol_gfx: %s", message);
}

/* ---------------------------------------------------------------------------
 *  Init / shutdown
 * ------------------------------------------------------------------------- */

static int rvn_sg_init(const RBackendInitInfo* info) {
	sg_desc desc;
	g_sg.info = *info;
	memset(&desc, 0, sizeof(desc));
	/* the pixel format of the swapchains: the native API may change it (Metal: BGRA8) */
	desc.environment.defaults.color_format = SG_PIXELFORMAT_RGBA8;
	desc.environment.defaults.depth_format = SG_PIXELFORMAT_NONE;
	desc.environment.defaults.sample_count = 1;
	if (!rvn_sg_context_init(info, &desc.environment)) return R_FALSE;
	desc.logger.func = rvn_sg_log;
	sg_setup(&desc);
	if (!sg_isvalid()) {
		rvn_sg_context_shutdown();
		return R_FALSE;
	}
	if (!rvn_sg_pipelines_init() || !rvn_sg_textures_init()) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Sokol: cannot create the pipelines of the renderer");
		sg_shutdown();
		rvn_sg_context_shutdown();
		return R_FALSE;
	}
	g_sg.initialized = R_TRUE;
	return R_TRUE;
}

static void rvn_sg_surface_release(rvn_sg_surface_t* surface) {
	/* (a resize keeps the native target: only the images follow the size) */
	rvn_sg_glass_release(surface);
	rvn_sg_target_destroy(&surface->frame);
}

static void rvn_sg_shutdown(void) {
	uint32_t i;
	if (!g_sg.initialized) return;
	for (i = 0; i < RVN_SG_MAX_SURFACES; i++) {
		if (!g_sg.surfaces[i].used) continue;
		rvn_sg_present_destroy(&g_sg.surfaces[i]);
		rvn_sg_surface_release(&g_sg.surfaces[i]);
	}
	rvn_sg_textures_shutdown();
	rvn_sg_pipelines_shutdown();
	sg_shutdown();
	rvn_sg_context_shutdown();
	memset(&g_sg, 0, sizeof(g_sg));
}

static void rvn_sg_wait_idle(void) {
	rvn_sg_context_finish();
}

/* ---------------------------------------------------------------------------
 *  Surfaces
 * ------------------------------------------------------------------------- */

static int rvn_sg_surface_create(Rid window, const RNativeWindow* native, Ruint width, Ruint height) {
	uint32_t index = R_ID_INDEX(window);
	rvn_sg_surface_t* surface;
	if (index >= RVN_SG_MAX_SURFACES) return R_FALSE;
	surface = &g_sg.surfaces[index];
	memset(surface, 0, sizeof(*surface));
	surface->used = R_TRUE;
	surface->window = window;
	surface->transparent = native && native->transparent;
	surface->width = (int)rvn_maxu(width, 1);
	surface->height = (int)rvn_maxu(height, 1);
	if (!rvn_sg_target_create(&surface->frame, surface->width, surface->height, "raven-frame")) {
		surface->used = R_FALSE;
		return R_FALSE;
	}
	/* a window of the window system is presented; without one (headless runs) frames are only rendered and captured */
	if (native && native->system != R_WINDOW_SYSTEM_NONE && !rvn_sg_present_create(surface, native)) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_WARNING,
			"Sokol: this window cannot be presented by the graphics API of the backend; it is rendered offscreen only");
	}
	return R_TRUE;
}

static void rvn_sg_surface_destroy(Rid window) {
	uint32_t index = R_ID_INDEX(window);
	if (index >= RVN_SG_MAX_SURFACES || !g_sg.surfaces[index].used) return;
	rvn_sg_present_destroy(&g_sg.surfaces[index]);
	rvn_sg_surface_release(&g_sg.surfaces[index]);
	g_sg.surfaces[index].used = R_FALSE;
}

static rvn_sg_surface_t* rvn_sg_surface_find(Rid window) {
	uint32_t index = R_ID_INDEX(window);
	if (index >= RVN_SG_MAX_SURFACES || !g_sg.surfaces[index].used || g_sg.surfaces[index].window != window) return NULL;
	return &g_sg.surfaces[index];
}

/* ---------------------------------------------------------------------------
 *  Frame
 * ------------------------------------------------------------------------- */

/* Pixels to clip space (2 / size, -1), the pointer for the effects: the push constants of the Vulkan shaders. */
static void rvn_sg_apply_transform(const rvn_sg_surface_t* surface, const RDrawData* data) {
	float transform[8];
	sg_range range;
	transform[0] = 2.0f / (float)surface->width;
	transform[2] = -1.0f;
	/* the UI top is NDC y = -1 (Vulkan), or +1 where the origin is the top left and NDC y points up */
	transform[1] = (rvn_sg_flip_y() ? -2.0f : 2.0f) / (float)surface->height;
	transform[3] = rvn_sg_flip_y() ? 1.0f : -1.0f;
	transform[4] = data->pointer[0];
	transform[5] = data->pointer[1];
	transform[6] = transform[7] = 0.0f;
	range.ptr = transform;
	range.size = sizeof(transform);
	sg_apply_uniforms(0, &range);
}

static void rvn_sg_begin_pass(const rvn_sg_target_t* target, const sg_color* clear) {
	sg_pass pass;
	memset(&pass, 0, sizeof(pass));
	pass.action.colors[0].load_action = clear ? SG_LOADACTION_CLEAR : SG_LOADACTION_LOAD;
	pass.action.colors[0].store_action = SG_STOREACTION_STORE;
	if (clear) pass.action.colors[0].clear_value = *clear;
	pass.attachments.colors[0] = target->color;
	sg_begin_pass(&pass);
}

/* Binds the UI pipeline and what it needs to start (or restart, after a backdrop capture) drawing into the target. */
static void rvn_sg_bind_ui_state(const rvn_sg_surface_t* surface, const rvn_sg_target_t* target, const RDrawData* data) {
	sg_apply_viewport(0, 0, target->width, target->height, rvn_sg_origin());
	sg_apply_pipeline(g_sg.ui_pipeline);
	rvn_sg_apply_transform(surface, data);
}

static void rvn_sg_draw_instances(const RDrawData* data, const RDrawCommand* command, sg_view sharp, sg_view blurred) {
	const rvn_sg_texture_t* texture = rvn_sg_texture_find(command->texture);
	sg_bindings bindings;
	memset(&bindings, 0, sizeof(bindings));
	bindings.vertex_buffers[0] = g_sg.instances;
	/* (no base instance in GLES 3.0: the first instance is an offset into the buffer) */
	bindings.vertex_buffer_offsets[0] = (int)(command->first_instance * sizeof(RDrawInstance));
	bindings.views[0] = texture->view;
	bindings.views[1] = sharp;
	bindings.views[2] = blurred;
	bindings.samplers[0] = texture->filter == R_FILTER_NEAREST ? g_sg.nearest : (texture->filter == R_FILTER_MIPMAP && texture->mips > 1) ? g_sg.mipmap : g_sg.linear;
	bindings.samplers[1] = g_sg.linear;
	bindings.samplers[2] = g_sg.linear;
	sg_apply_bindings(&bindings);
	/* four vertices generated by the vertex shader, one instance per primitive */
	sg_draw(0, 4, (int)command->instance_count);
	RVN_UNUSED(data);
}

/* Records the commands of the frame inside the pass already begun. With `glass`, that pass is the scene. */
static void rvn_sg_record_commands(rvn_sg_surface_t* surface, const RDrawData* data, int glass) {
	const rvn_sg_target_t* target = glass ? &surface->scene : &surface->frame;
	sg_view sharp = glass ? surface->down[0].sample : g_sg.white.view;
	sg_view blurred = glass ? surface->up[0].sample : g_sg.white.view;
	static int warned_callback;
	uint32_t i;

	rvn_sg_bind_ui_state(surface, target, data);
	for (i = 0; i < data->command_count; i++) {
		const RDrawCommand* command = &data->commands[i];
		if (command->type == R_DRAW_CALLBACK) {
			if (!warned_callback) {
				warned_callback = R_TRUE;
				rvn_error(R_ERROR_BACKEND, R_SEVERITY_WARNING, "Sokol: viewport draw callbacks are not available in this backend yet");
			}
			continue;
		}
		if (command->type == R_DRAW_BACKDROP) {
			if (!glass) continue;
			sg_end_pass();
			rvn_sg_glass_capture(surface, data->scale, command->rect);
			rvn_sg_begin_pass(&surface->scene, NULL);
			rvn_sg_bind_ui_state(surface, target, data);
			continue;
		}
		if (command->type != R_DRAW_INSTANCES && command->type != R_DRAW_ERASE && command->type != R_DRAW_SHADER) continue;
		if (command->instance_count == 0 || command->first_instance + command->instance_count > data->instance_count) continue;
		if (command->type == R_DRAW_SHADER) {
			const rvn_sg_effect_t* effect = rvn_sg_effect_find(command->shader);
			if (!effect) continue;
			sg_apply_pipeline(effect->pipeline);
			rvn_sg_apply_transform(surface, data);
		} else if (command->type == R_DRAW_ERASE) {
			sg_apply_pipeline(g_sg.erase_pipeline);
			rvn_sg_apply_transform(surface, data);
		}
		rvn_sg_draw_instances(data, command, sharp, blurred);
		if (command->type != R_DRAW_INSTANCES) {
			sg_apply_pipeline(g_sg.ui_pipeline);
			rvn_sg_apply_transform(surface, data);
		}
	}
}

/* Draws the finished frame into the swapchain of the window (a pass of its own: the frame is an image). */
static int rvn_sg_present_frame(rvn_sg_surface_t* surface, int vsync) {
	sg_pass pass;
	sg_bindings bindings;
	float params[4];
	sg_range range;
	if (!surface->present) return R_FALSE;
	memset(&pass, 0, sizeof(pass));
	if (!rvn_sg_present_begin(surface, vsync, &pass.swapchain)) return R_FALSE;
	pass.action.colors[0].load_action = SG_LOADACTION_DONTCARE;
	pass.action.colors[0].store_action = SG_STOREACTION_STORE;
	sg_begin_pass(&pass);
	sg_apply_viewport(0, 0, surface->width, surface->height, rvn_sg_origin());
	sg_apply_pipeline(g_sg.present_pipeline);
	memset(&bindings, 0, sizeof(bindings));
	bindings.views[0] = surface->frame.sample;
	bindings.samplers[0] = g_sg.linear;
	sg_apply_bindings(&bindings);
	/* copy; the frame has the UI top in its first row, which a GL window (origin bottom left) shows upside down: flip it */
	params[0] = params[1] = 0.0f;
	params[2] = 2.0f;
	params[3] = rvn_sg_flip_y() ? 0.0f : 1.0f;
	range.ptr = params;
	range.size = sizeof(params);
	sg_apply_uniforms(0, &range);
	sg_draw(0, 3, 1);
	sg_end_pass();
	return R_TRUE;
}

/* Copies the instances of the frame into the transient vertex buffer. */
static int rvn_sg_upload_instances(const RDrawData* data) {
	size_t needed = (size_t)data->instance_count * sizeof(RDrawInstance);
	sg_write_buffer_desc write;
	if (needed == 0) return R_TRUE;
	if (needed > g_sg.instances_size) {
		sg_buffer_desc desc;
		size_t size = g_sg.instances_size ? g_sg.instances_size : (size_t)1024 * sizeof(RDrawInstance);
		while (size < needed) size *= 2;
		if (g_sg.instances.id) sg_destroy_buffer(g_sg.instances);
		memset(&desc, 0, sizeof(desc));
		desc.size = size;
		desc.usage.vertex_buffer = true;
		desc.usage.write_transient = true;
		desc.label = "raven-instances";
		g_sg.instances = sg_make_buffer(&desc);
		g_sg.instances_size = size;
		if (sg_query_buffer_state(g_sg.instances) != SG_RESOURCESTATE_VALID) return R_FALSE;
	}
	memset(&write, 0, sizeof(write));
	write.src.data.ptr = data->instances;
	write.src.data.size = needed;
	write.dst.buffer = g_sg.instances;
	sg_write_buffer_transient(&write);
	return R_TRUE;
}

static void rvn_sg_render_frame(const RDrawData* data, RCapture* capture) {
	rvn_sg_surface_t* surface = rvn_sg_surface_find(data->window);
	sg_color clear;
	uint32_t i;
	int glass = R_FALSE, presented;

	if (capture) memset(capture, 0, sizeof(*capture));
	if (!surface || data->framebuffer_width == 0 || data->framebuffer_height == 0) return;

	/* the surface follows the frame: new images when the size changed */
	if ((int)data->framebuffer_width != surface->width || (int)data->framebuffer_height != surface->height) {
		rvn_sg_surface_release(surface);
		surface->width = (int)data->framebuffer_width;
		surface->height = (int)data->framebuffer_height;
		if (!rvn_sg_target_create(&surface->frame, surface->width, surface->height, "raven-frame")) return;
	}
	for (i = 0; i < data->command_count && !glass; i++) glass = data->commands[i].type == R_DRAW_BACKDROP;
	if (glass && !rvn_sg_glass_prepare(surface)) glass = R_FALSE;

	rvn_sg_uploads_flush();
	rvn_sg_mips_generate();
	if (!rvn_sg_upload_instances(data)) return;

	clear.r = data->clear_color[0];
	clear.g = data->clear_color[1];
	clear.b = data->clear_color[2];
	clear.a = data->clear_color[3];
	rvn_sg_begin_pass(glass ? &surface->scene : &surface->frame, &clear);
	if (data->instance_count) rvn_sg_record_commands(surface, data, glass);
	sg_end_pass();
	if (glass) rvn_sg_glass_present(surface);
	presented = rvn_sg_present_frame(surface, data->vsync);
	sg_commit();
	if (presented) rvn_sg_present_end(surface);

	if (capture) {
		uint8_t* pixels = rvn_sg_read_target(&surface->frame);
		size_t count = (size_t)surface->width * (size_t)surface->height, p;
		/* a window is opaque whatever the alpha left in the frame, unless it is one that is seen through */
		if (!surface->transparent) for (p = 0; p < count; p++) pixels[p * 4 + 3] = 255;
		capture->pixels = pixels;
		capture->width = surface->width;
		capture->height = surface->height;
	}
}

/* ---------------------------------------------------------------------------
 *  The table: every entry runs in an autorelease pool (Metal creates Objective-C objects on every call;
 *  the render thread has no run loop that would drain them)
 * ------------------------------------------------------------------------- */

static int rvn_sg_entry_init(const RBackendInitInfo* info) {
	void* pool = rvn_sg_pool_push();
	int ok = rvn_sg_init(info);
	rvn_sg_pool_pop(pool);
	return ok;
}

static void rvn_sg_entry_shutdown(void) {
	void* pool = rvn_sg_pool_push();
	rvn_sg_shutdown();
	rvn_sg_pool_pop(pool);
}

static int rvn_sg_entry_surface_create(Rid window, const RNativeWindow* native, Ruint width, Ruint height) {
	void* pool = rvn_sg_pool_push();
	int ok = rvn_sg_surface_create(window, native, width, height);
	rvn_sg_pool_pop(pool);
	return ok;
}

static void rvn_sg_entry_surface_destroy(Rid window) {
	void* pool = rvn_sg_pool_push();
	rvn_sg_surface_destroy(window);
	rvn_sg_pool_pop(pool);
}

static int rvn_sg_entry_texture_create(Rid texture, const RTextureDesc* desc, const void* pixels) {
	void* pool = rvn_sg_pool_push();
	int ok = rvn_sg_texture_create(texture, desc, pixels);
	rvn_sg_pool_pop(pool);
	return ok;
}

static void rvn_sg_entry_texture_update(Rid texture, Ruint x, Ruint y, Ruint width, Ruint height, const void* pixels) {
	void* pool = rvn_sg_pool_push();
	rvn_sg_texture_update(texture, x, y, width, height, pixels);
	rvn_sg_pool_pop(pool);
}

static void rvn_sg_entry_texture_destroy(Rid texture) {
	void* pool = rvn_sg_pool_push();
	rvn_sg_texture_destroy(texture);
	rvn_sg_pool_pop(pool);
}

static void rvn_sg_entry_render_frame(const RDrawData* data, RCapture* capture) {
	void* pool = rvn_sg_pool_push();
	rvn_sg_render_frame(data, capture);
	rvn_sg_pool_pop(pool);
}

static void rvn_sg_entry_wait_idle(void) {
	void* pool = rvn_sg_pool_push();
	rvn_sg_wait_idle();
	rvn_sg_pool_pop(pool);
}

static int rvn_sg_entry_shader_create(Rid shader, const uint32_t* spirv, size_t size) {
	void* pool = rvn_sg_pool_push();
	int ok = rvn_sg_shader_create(shader, spirv, size);
	rvn_sg_pool_pop(pool);
	return ok;
}

static void rvn_sg_entry_shader_destroy(Rid shader) {
	void* pool = rvn_sg_pool_push();
	rvn_sg_shader_destroy(shader);
	rvn_sg_pool_pop(pool);
}

const RBackendVTable* rvn_backend_sokol(void) {
	static const RBackendVTable vtable = {
		"sokol",
		rvn_sg_entry_init,
		rvn_sg_entry_shutdown,
		rvn_sg_entry_surface_create,
		rvn_sg_entry_surface_destroy,
		rvn_sg_entry_texture_create,
		rvn_sg_entry_texture_update,
		rvn_sg_texture_set_filter,
		rvn_sg_entry_texture_destroy,
		rvn_sg_entry_render_frame,
		rvn_sg_entry_wait_idle,
		rvn_sg_entry_shader_create,
		rvn_sg_entry_shader_destroy,
		R_BACKEND_CAP_GLASS | R_BACKEND_CAP_ERASE | R_BACKEND_CAP_SHADERS | R_BACKEND_CAP_READBACK
	};
	return &vtable;
}
