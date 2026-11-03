/**
 * Raven - render/shader.c
 * Effects: fragment shaders of the application and the built-in ones, and
 * how a painter draws them (raven_shader.h).
 *
 * A shader is a handle on the UI thread; its SPIR-V goes to the backend
 * through the render thread (CreateShader). Drawing one appends a SHADER
 * instance to an R_DRAW_SHADER command that names it; the backend runs it
 * with the inputs and bindings of the UI shader (raven_effect.glsl).
 */

#include "renderer.h"
#include "../core/context.h"

#include "vulkan/generated/effect_aurora_spv.h"
#include "vulkan/generated/effect_shimmer_spv.h"
#include "vulkan/generated/effect_neon_spv.h"
#include "vulkan/generated/effect_ripple_spv.h"
#include "vulkan/generated/effect_plasma_spv.h"
#include "vulkan/generated/effect_grain_spv.h"
#include "vulkan/generated/effect_spotlight_spv.h"
#include "vulkan/generated/effect_mesh_spv.h"

#define RVN_EFFECT_COUNT ((int)(R_EFFECT_LAST_ - R_EFFECT_AURORA))

typedef struct rvn_effect_info_t {
	const char* name;
	const uint32_t* code;
	size_t size;
	Ruint flags;
	uint32_t color, color2;   /* 0xRRGGBB, opaque */
	float params[4];
} rvn_effect_info_t;

/* in the order of REffect */
static const rvn_effect_info_t g_effects[] = {
	{ "aurora", rvn_effect_aurora_spv, sizeof(rvn_effect_aurora_spv), R_SHADER_ANIMATED, 0x30D158, 0x5E5CE6, { 1.0f, 1.0f, 1.5f, 0.0f } },
	{ "shimmer", rvn_effect_shimmer_spv, sizeof(rvn_effect_shimmer_spv), R_SHADER_ANIMATED, 0x2C2C30, 0x48484E, { 1.0f, 0.35f, 20.0f, 0.0f } },
	{ "neon", rvn_effect_neon_spv, sizeof(rvn_effect_neon_spv), R_SHADER_ANIMATED, 0x0A84FF, 0xFF375F, { 1.0f, 2.0f, 10.0f, 0.0f } },
	{ "ripple", rvn_effect_ripple_spv, sizeof(rvn_effect_ripple_spv), R_SHADER_ANIMATED | R_SHADER_BACKDROP, 0x64D2FF, 0xFFFFFF, { 1.0f, 6.0f, 1.0f, 0.5f } },
	{ "plasma", rvn_effect_plasma_spv, sizeof(rvn_effect_plasma_spv), R_SHADER_ANIMATED, 0x5E5CE6, 0xFF9F0A, { 1.0f, 1.0f, 1.0f, 0.0f } },
	{ "grain", rvn_effect_grain_spv, sizeof(rvn_effect_grain_spv), R_SHADER_ANIMATED, 0x3A3A3F, 0x1C1C1E, { 0.6f, 60.0f, 0.0f, 0.0f } },
	{ "spotlight", rvn_effect_spotlight_spv, sizeof(rvn_effect_spotlight_spv), 0, 0x2B2B60, 0x141416, { 0.3f, 0.2f, 0.7f, 0.0f } },
	{ "mesh", rvn_effect_mesh_spv, sizeof(rvn_effect_mesh_spv), R_SHADER_ANIMATED, 0xFF6482, 0x0A84FF, { 1.0f, 0.0f, 0.0f, 0.0f } }
};

/* the built-in effects, made the first time they are asked for */
static Rid g_effect_ids[RVN_EFFECT_COUNT];

rvn_shader_t* rvn_shader_get(Rid id) {
	return (rvn_shader_t*)rvn_pool_get(&g_rvn.shaders, id);
}

static Rid rvn_shader_make(const void* spirv, size_t size, Ruint flags, int effect) {
	rvn_shader_t* shader;
	uint32_t* copy;
	uint32_t magic = 0;
	/* SPIR-V: whole 32-bit words, starting with its magic number (the bytes may have any alignment) */
	if (spirv && size >= 4) memcpy(&magic, spirv, sizeof(magic));
	if (!spirv || size < 20 || size % 4 != 0 || magic != 0x07230203u) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rshader_create: this is not SPIR-V (compile the shader with glslangValidator -V or glslc)");
		return R_INVALID_ID;
	}
	shader = (rvn_shader_t*)rvn_calloc(sizeof(rvn_shader_t));
	shader->flags = flags;
	shader->effect = effect;
	shader->id = rvn_pool_add(&g_rvn.shaders, shader);
	if (shader->id == R_INVALID_ID) {
		rvn_free(shader);
		return R_INVALID_ID;
	}
	copy = (uint32_t*)rvn_alloc(size);
	memcpy(copy, spirv, size);
	shader->spirv = (uint32_t*)rvn_alloc(size);
	memcpy(shader->spirv, spirv, size);
	shader->spirv_size = size;
	rvn_renderer_create_shader(shader->id, copy, size);
	shader->created = R_TRUE;
	return shader->id;
}

void rvn_shader_restore(rvn_shader_t* shader) {
	uint32_t* copy;
	shader->created = R_FALSE;
	if (!shader->spirv) return;
	copy = (uint32_t*)rvn_alloc(shader->spirv_size);
	memcpy(copy, shader->spirv, shader->spirv_size);
	rvn_renderer_create_shader(shader->id, copy, shader->spirv_size);
	shader->created = R_TRUE;
}

Rid Rshader_create(const void* spirv, size_t size, Ruint flags) {
	RVN_API_GUARD(R_INVALID_ID);
	return rvn_shader_make(spirv, size, flags, -1);
}

Rid Rshader_create_from_file(const char* path, Ruint flags) {
	size_t size = 0;
	char* data;
	Rid id;
	RVN_API_GUARD(R_INVALID_ID);
	if (!path) return R_INVALID_ID;
	data = Rfile_read(path, &size);
	if (!data) {
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rshader_create_from_file: cannot read %s", path);
		return R_INVALID_ID;
	}
	id = rvn_shader_make(data, size, flags, -1);
	Rmemory_free(data);
	return id;
}

/* the .spv files named by Effect attributes: one shader each */
typedef struct rvn_shader_file_t {
	char* path;
	Ruint flags;
	Rid id;
} rvn_shader_file_t;
static rvn_shader_file_t* g_shader_files;
static uint32_t g_shader_file_count;
static uint32_t g_shader_file_cap;

Rid rvn_shader_from_file_cached(const char* path, Ruint flags) {
	uint32_t i;
	Rid id;
	for (i = 0; i < g_shader_file_count; i++) {
		if (g_shader_files[i].flags == flags && strcmp(g_shader_files[i].path, path) == 0 && rvn_shader_get(g_shader_files[i].id)) return g_shader_files[i].id;
	}
	id = Rshader_create_from_file(path, flags);
	if (id == R_INVALID_ID) return R_INVALID_ID;
	RVN_ARRAY_RESERVE(g_shader_files, g_shader_file_cap, g_shader_file_count + 1);
	g_shader_files[g_shader_file_count].path = rvn_strdup(path);
	g_shader_files[g_shader_file_count].flags = flags;
	g_shader_files[g_shader_file_count].id = id;
	g_shader_file_count++;
	return id;
}

void Rshader_destroy(Rid id) {
	rvn_shader_t* shader;
	int i;
	RVN_API_GUARD_VOID();
	shader = rvn_shader_get(id);
	if (!shader) return;
	for (i = 0; i < RVN_EFFECT_COUNT; i++) if (g_effect_ids[i] == id) g_effect_ids[i] = R_INVALID_ID;
	/* (the frames already queued were drawn with it before the command arrives: the render thread keeps the order) */
	if (shader->created) rvn_renderer_destroy_shader(id);
	rvn_pool_remove(&g_rvn.shaders, id);
	rvn_free(shader->spirv);
	rvn_free(shader);
}

int Rshader_is_valid(Rid id) {
	return rvn_shader_get(id) != NULL;
}

Ruint Rshader_get_flags(Rid id) {
	rvn_shader_t* shader = rvn_shader_get(id);
	return shader ? shader->flags : 0;
}

void rvn_shaders_shutdown(void) {
	uint32_t i;
	for (i = 0; i < g_rvn.shaders.used; i++) {
		rvn_shader_t* shader = (rvn_shader_t*)rvn_pool_at(&g_rvn.shaders, i);
		if (!shader) continue;
		if (shader->created) rvn_renderer_destroy_shader(shader->id);
		rvn_pool_remove(&g_rvn.shaders, shader->id);
		rvn_free(shader->spirv);
		rvn_free(shader);
	}
	memset(g_effect_ids, 0, sizeof(g_effect_ids));
	for (i = 0; i < g_shader_file_count; i++) rvn_free(g_shader_files[i].path);
	rvn_free(g_shader_files);
	g_shader_files = NULL;
	g_shader_file_count = g_shader_file_cap = 0;
}

/* ---------------------------------------------------------------------------
 *  Built-in effects
 * ------------------------------------------------------------------------- */

Rid Reffect_get(REffect effect) {
	int index = (int)effect - (int)R_EFFECT_AURORA;
	RVN_API_GUARD(R_INVALID_ID);
	if (index < 0 || index >= RVN_EFFECT_COUNT) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Reffect_get: unknown effect %d", (int)effect);
		return R_INVALID_ID;
	}
	if (!rvn_shader_get(g_effect_ids[index])) {
		g_effect_ids[index] = rvn_shader_make(g_effects[index].code, g_effects[index].size, g_effects[index].flags, index);
	}
	return g_effect_ids[index];
}

Rid Reffect_find(const char* name) {
	int i;
	if (!name) return R_INVALID_ID;
	for (i = 0; i < RVN_EFFECT_COUNT; i++) {
		if (rvn_stricmp(g_effects[i].name, name) == 0) return Reffect_get((REffect)(R_EFFECT_AURORA + i));
	}
	return R_INVALID_ID;
}

const char* Reffect_get_name(REffect effect) {
	int index = (int)effect - (int)R_EFFECT_AURORA;
	return index >= 0 && index < RVN_EFFECT_COUNT ? g_effects[index].name : "";
}

static void rvn_set_rgb(float* out, uint32_t rgb) {
	out[0] = (float)((rgb >> 16) & 0xFFu) / 255.0f;
	out[1] = (float)((rgb >> 8) & 0xFFu) / 255.0f;
	out[2] = (float)(rgb & 0xFFu) / 255.0f;
	out[3] = 1.0f;
}

void Rshader_look_init(RShaderLook* look) {
	int i;
	if (!look) return;
	memset(look, 0, sizeof(*look));
	for (i = 0; i < 4; i++) look->color[i] = look->color2[i] = 1.0f;
	look->texture = R_INVALID_ID;
	look->animate = -1;
	look->pointer = R_TRUE;
	look->hover = -1.0f;
	look->press = -1.0f;
}

void Reffect_get_defaults(REffect effect, RShaderLook* look) {
	int index = (int)effect - (int)R_EFFECT_AURORA;
	Rshader_look_init(look);
	if (!look || index < 0 || index >= RVN_EFFECT_COUNT) return;
	rvn_set_rgb(look->color, g_effects[index].color);
	rvn_set_rgb(look->color2, g_effects[index].color2);
	memcpy(look->params, g_effects[index].params, sizeof(look->params));
}

void Rshader_look_defaults(Rid id, RShaderLook* look) {
	rvn_shader_t* shader = rvn_shader_get(id);
	if (shader && shader->effect >= 0) Reffect_get_defaults((REffect)(R_EFFECT_AURORA + shader->effect), look);
	else Rshader_look_init(look);
}

/* ---------------------------------------------------------------------------
 *  Drawing
 * ------------------------------------------------------------------------- */

void rvn_paint_shader(RPainter* painter, rvn_rect_t rect, const float radii[4], Rid id, const RShaderLook* look) {
	rvn_shader_t* shader = rvn_shader_get(id);
	RDrawInstance* instance;
	rvn_box_t box = rvn_paint_snap(painter, rect);
	float half, opacity = rvn_clampf(painter->opacity, 0.0f, 1.0f);
	int animate, i;

	if (!shader || !look || box.x1 <= box.x0 || box.y1 <= box.y0 || opacity <= 0.0f) return;
	if (!rvn_box_overlaps(box, painter->clip)) return;
	half = rvn_minf(box.x1 - box.x0, box.y1 - box.y0) * 0.5f;
	/* what is behind is captured first, a little around (an effect may look a few pixels away) */
	if (shader->flags & R_SHADER_BACKDROP) {
		float margin = 24.0f * painter->scale;
		rvn_drawlist_add_backdrop(painter->list, rvn_box_intersect(painter->clip,
			rvn_box(box.x0 - margin, box.y0 - margin, box.x1 + margin, box.y1 + margin)));
	}
	instance = rvn_drawlist_add_shader(painter->list, id, look->texture);
	memset(instance, 0, sizeof(*instance));
	instance->clip[0] = painter->clip.x0;
	instance->clip[1] = painter->clip.y0;
	instance->clip[2] = painter->clip.x1;
	instance->clip[3] = painter->clip.y1;
	instance->rect[0] = box.x0;
	instance->rect[1] = box.y0;
	instance->rect[2] = box.x1;
	instance->rect[3] = box.y1;
	for (i = 0; i < 4; i++) {
		float radius = radii ? radii[i] : look->radius;
		instance->radii[i] = rvn_clampf(radius * painter->scale, 0.0f, half);
		instance->uv[i] = look->params[i];
	}
	instance->color = rvn_rgba(look->color[0], look->color[1], look->color[2], look->color[3]);
	instance->border_color = rvn_rgba(look->color2[0], look->color2[1], look->color2[2], look->color2[3]);
	/* (seconds that keep their precision in a float: the effects only care how they change) */
	/* reduced motion: the effects stand still (they still follow the pointer) */
	instance->border_width = Rget_animations_enabled() ? (float)fmod(Rget_time(), 3600.0) : 0.0f;
	instance->flags = R_PRIM_SHADER | (look->texture != R_INVALID_ID ? R_INSTANCE_TEXTURED : 0u) | ((uint32_t)(opacity * 255.0f + 0.5f) << 8);
	animate = look->animate < 0 ? (shader->flags & R_SHADER_ANIMATED) != 0 : look->animate != 0;
	if (!Rget_animations_enabled()) animate = R_FALSE;
	if (look->pointer) {
		/* the pointer: how much it is over the rectangle and presses it (eased by a Border, else at once) */
		float px = 0.0f, py = 0.0f;
		uint32_t buttons = 0;
		int inside = painter->window && rvn_window_pointer(painter->window, &px, &py, &buttons);
		float hover, press;
		px *= painter->scale;
		py *= painter->scale;
		inside = inside && px >= box.x0 && py >= box.y0 && px < box.x1 && py < box.y1
			&& px >= painter->clip.x0 && py >= painter->clip.y0 && px < painter->clip.x1 && py < painter->clip.y1;
		hover = look->hover >= 0.0f ? look->hover : (inside ? 1.0f : 0.0f);
		press = look->press >= 0.0f ? look->press : (inside && (buttons & (1u << R_MOUSE_LEFT)) ? 1.0f : 0.0f);
		instance->flags |= ((uint32_t)(rvn_clampf(hover, 0.0f, 1.0f) * 255.0f + 0.5f) << 16)
			| ((uint32_t)(rvn_clampf(press, 0.0f, 1.0f) * 255.0f + 0.5f) << 24);
		/* while the pointer is over it, every frame shows where it is */
		if (inside || hover > 0.0f) animate = R_TRUE;
	} else {
		instance->flags |= R_INSTANCE_NO_POINTER;
	}
	if (animate) rvn_paint_keep_animating(painter);
}

void Rdraw_shader(RPainter* painter, Rid shader, float x, float y, float width, float height, const RShaderLook* look) {
	RShaderLook defaults;
	if (!painter) return;
	if (!look) {
		Rshader_look_defaults(shader, &defaults);
		look = &defaults;
	}
	rvn_paint_shader(painter, rvn_rect(x, y, width, height), NULL, shader, look);
}
