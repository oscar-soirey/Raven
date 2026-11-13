/**
 * Raven - render/sokol/sg_pipeline.c
 * Shaders, pipelines and effects.
 *
 * The UI shaders are the Vulkan ones, translated to the shading language of the graphics API at
 * development time (gen_shaders.py, committed in generated/). One vertex shader serves the UI and every
 * effect: an effect only replaces the fragment shader (same inputs, same bindings).
 *
 * An effect (raven_shader.h) arrives as SPIR-V, which cannot be translated at run time here: the built-in
 * effects are found by a hash of their SPIR-V and use their precompiled variant. Any other SPIR-V is
 * refused, and the effect is drawn as a rounded rectangle (what the core does for a backend that
 * refuses a shader). The planned .rfx bundle (docs/SOKOL_BACKEND_PLAN.md, 3.2) carries variants for
 * the effects of applications.
 */

#include "sg_backend.h"
#include "../rfx.h"

/* (the shader sources are strings of up to 16 KB: longer than the 4 KB ISO C promises, fine for every compiler Raven supports) */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Woverlength-strings"
#endif
#include "generated/sg_shaders.h"
#include "generated/sg_effects.h"

#if defined(RAVEN_SOKOL_GLES3)
#define RVN_SG_UI_VERT rvn_sg_ui_vert_es300
#define RVN_SG_UI_FRAG rvn_sg_ui_frag_es300
#define RVN_SG_BLUR_VERT rvn_sg_blur_vert_es300
#define RVN_SG_BLUR_FRAG rvn_sg_blur_frag_es300
#elif defined(RAVEN_SOKOL_METAL)
#define RVN_SG_UI_VERT rvn_sg_ui_vert_msl
#define RVN_SG_UI_FRAG rvn_sg_ui_frag_msl
#define RVN_SG_BLUR_VERT rvn_sg_blur_vert_msl
#define RVN_SG_BLUR_FRAG rvn_sg_blur_frag_msl
#else
#error "Raven: no shader sources for this sokol graphics API yet"
#endif
/* Entry point of a function of a generated shader: SPIRV-Cross renames main() for MSL, and GL ignores the name. */
#define RVN_SG_ENTRY "main0"

static const char* const g_ui_samplers[3] = { "u_texture", "u_backdrop", "u_backdrop_blur" };
static const char* const g_effect_samplers[3] = { "rvn_texture", "rvn_backdrop", "rvn_backdrop_blur" };

/* The UI vertex shader with a fragment shader: the UI itself, or an effect. */
static sg_shader rvn_sg_shader_make_ui(const char* fragment, const char* const sampler_names[3], const char* label) {
	static const char* const attributes[8] = { "in_rect", "in_uv", "in_clip", "in_radii", "in_color", "in_border_color", "in_border_width", "in_flags" };
	sg_shader_desc desc;
	int i;
	memset(&desc, 0, sizeof(desc));
	desc.vertex_func.source = RVN_SG_UI_VERT;
	desc.vertex_func.entry = RVN_SG_ENTRY;
	desc.fragment_func.source = fragment;
	desc.fragment_func.entry = RVN_SG_ENTRY;
	for (i = 0; i < 8; i++) {
		desc.attrs[i].glsl_name = attributes[i];
		desc.attrs[i].base_type = i == 7 ? SG_SHADERATTRBASETYPE_UINT : SG_SHADERATTRBASETYPE_FLOAT;
	}
	/* the push constants of the Vulkan shaders: pixels -> clip space (4 floats), the pointer, 2 spare */
	desc.uniform_blocks[0].stage = SG_SHADERSTAGE_VERTEX;
	desc.uniform_blocks[0].size = 32;
	desc.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
	desc.uniform_blocks[0].glsl_uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
	desc.uniform_blocks[0].glsl_uniforms[0].array_count = 2;
	desc.uniform_blocks[0].glsl_uniforms[0].glsl_name = "PushConstants";
	desc.uniform_blocks[0].msl_buffer_n = 0;
	/*
	 * Texture of the command, then the sharp and the blurred backdrop (set 0 and set 1 of the Vulkan pipeline),
	 * each with a sampler of its own: bindings.samplers[i] goes with bindings.views[i]. GL names the combined
	 * sampler (glsl_name), Metal numbers the slots (gen_shaders.py gives texture(i) and sampler(i)).
	 */
	for (i = 0; i < 3; i++) {
		desc.views[i].texture.stage = SG_SHADERSTAGE_FRAGMENT;
		desc.views[i].texture.image_type = SG_IMAGETYPE_2D;
		desc.views[i].texture.sample_type = SG_IMAGESAMPLETYPE_FLOAT;
		desc.views[i].texture.msl_texture_n = (uint8_t)i;
		desc.samplers[i].stage = SG_SHADERSTAGE_FRAGMENT;
		desc.samplers[i].sampler_type = SG_SAMPLERTYPE_FILTERING;
		desc.samplers[i].msl_sampler_n = (uint8_t)i;
		desc.texture_sampler_pairs[i].stage = SG_SHADERSTAGE_FRAGMENT;
		desc.texture_sampler_pairs[i].view_slot = (uint8_t)i;
		desc.texture_sampler_pairs[i].sampler_slot = (uint8_t)i;
		desc.texture_sampler_pairs[i].glsl_name = sampler_names[i];
	}
	desc.label = label;
	return sg_make_shader(&desc);
}

static sg_pipeline rvn_sg_pipeline_make_ui(sg_shader shader, int erase, const char* label) {
	sg_pipeline_desc desc;
	memset(&desc, 0, sizeof(desc));
	desc.shader = shader;
	desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP;
	/* no vertex data: one instance per primitive, the four corners come from the vertex index */
	desc.layout.buffers[0].stride = (int)sizeof(RDrawInstance);
	desc.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_INSTANCE;
	desc.layout.attrs[0] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, rect), SG_VERTEXFORMAT_FLOAT4 };
	desc.layout.attrs[1] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, uv), SG_VERTEXFORMAT_FLOAT4 };
	desc.layout.attrs[2] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, clip), SG_VERTEXFORMAT_FLOAT4 };
	desc.layout.attrs[3] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, radii), SG_VERTEXFORMAT_FLOAT4 };
	desc.layout.attrs[4] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, color), SG_VERTEXFORMAT_UBYTE4N };
	desc.layout.attrs[5] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, border_color), SG_VERTEXFORMAT_UBYTE4N };
	desc.layout.attrs[6] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, border_width), SG_VERTEXFORMAT_FLOAT };
	desc.layout.attrs[7] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, flags), SG_VERTEXFORMAT_UINT };
	desc.color_count = 1;
	desc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
	desc.colors[0].blend.enabled = true;
	desc.colors[0].blend.src_factor_rgb = erase ? SG_BLENDFACTOR_ZERO : SG_BLENDFACTOR_ONE;
	desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	desc.colors[0].blend.src_factor_alpha = erase ? SG_BLENDFACTOR_ZERO : SG_BLENDFACTOR_ONE;
	desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	desc.depth.pixel_format = SG_PIXELFORMAT_NONE;
	desc.label = label;
	return sg_make_pipeline(&desc);
}

/* The filter of the glass backdrop (blur.frag): one fragment uniform vec4 (xy = step, z = mode), one texture. */
static int rvn_sg_blur_init(void) {
	sg_shader_desc shader;
	sg_pipeline_desc pipeline;
	memset(&shader, 0, sizeof(shader));
	shader.vertex_func.source = RVN_SG_BLUR_VERT;
	shader.vertex_func.entry = RVN_SG_ENTRY;
	shader.fragment_func.source = RVN_SG_BLUR_FRAG;
	shader.fragment_func.entry = RVN_SG_ENTRY;
	shader.uniform_blocks[0].stage = SG_SHADERSTAGE_FRAGMENT;
	shader.uniform_blocks[0].size = 16;
	shader.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
	shader.uniform_blocks[0].glsl_uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
	shader.uniform_blocks[0].glsl_uniforms[0].array_count = 1;
	shader.uniform_blocks[0].glsl_uniforms[0].glsl_name = "PushConstants";
	shader.uniform_blocks[0].msl_buffer_n = 0;
	shader.views[0].texture.stage = SG_SHADERSTAGE_FRAGMENT;
	shader.views[0].texture.image_type = SG_IMAGETYPE_2D;
	shader.views[0].texture.sample_type = SG_IMAGESAMPLETYPE_FLOAT;
	shader.views[0].texture.msl_texture_n = 0;
	shader.samplers[0].stage = SG_SHADERSTAGE_FRAGMENT;
	shader.samplers[0].sampler_type = SG_SAMPLERTYPE_FILTERING;
	shader.samplers[0].msl_sampler_n = 0;
	shader.texture_sampler_pairs[0].stage = SG_SHADERSTAGE_FRAGMENT;
	shader.texture_sampler_pairs[0].view_slot = 0;
	shader.texture_sampler_pairs[0].sampler_slot = 0;
	shader.texture_sampler_pairs[0].glsl_name = "u_source";
	shader.label = "raven-blur";
	g_sg.blur_shader = sg_make_shader(&shader);
	if (sg_query_shader_state(g_sg.blur_shader) != SG_RESOURCESTATE_VALID) return R_FALSE;
	memset(&pipeline, 0, sizeof(pipeline));
	pipeline.shader = g_sg.blur_shader;
	pipeline.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
	pipeline.color_count = 1;
	pipeline.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
	pipeline.depth.pixel_format = SG_PIXELFORMAT_NONE;
	pipeline.label = "raven-blur";
	g_sg.blur_pipeline = sg_make_pipeline(&pipeline);
	if (sg_query_pipeline_state(g_sg.blur_pipeline) != SG_RESOURCESTATE_VALID) return R_FALSE;
	/* the same filter drawing into a window: the pixel format is the one of its swapchain (RGBA8 on GL, BGRA8 on Metal) */
	pipeline.colors[0].pixel_format = sg_query_desc().environment.defaults.color_format;
	pipeline.label = "raven-present";
	g_sg.present_pipeline = sg_make_pipeline(&pipeline);
	return sg_query_pipeline_state(g_sg.present_pipeline) == SG_RESOURCESTATE_VALID;
}

int rvn_sg_pipelines_init(void) {
	sg_sampler_desc sampler;
	memset(&sampler, 0, sizeof(sampler));
	sampler.min_filter = sampler.mag_filter = SG_FILTER_LINEAR;
	sampler.wrap_u = sampler.wrap_v = sampler.wrap_w = SG_WRAP_CLAMP_TO_EDGE;
	/* the first level only: a texture has room for mip levels, which only the mipmap sampler reads (0 would mean "no limit") */
	sampler.max_lod = 0.001f;
	g_sg.linear = sg_make_sampler(&sampler);
	sampler.min_filter = sampler.mag_filter = SG_FILTER_NEAREST;
	g_sg.nearest = sg_make_sampler(&sampler);
	sampler.min_filter = SG_FILTER_LINEAR;
	sampler.mag_filter = SG_FILTER_LINEAR;
	sampler.mipmap_filter = SG_FILTER_LINEAR;
	sampler.max_lod = 0.0f;
	sampler.label = "raven-mipmap";
	g_sg.mipmap = sg_make_sampler(&sampler);

	g_sg.ui_shader = rvn_sg_shader_make_ui(RVN_SG_UI_FRAG, g_ui_samplers, "raven-ui");
	if (sg_query_shader_state(g_sg.ui_shader) != SG_RESOURCESTATE_VALID) return R_FALSE;
	g_sg.ui_pipeline = rvn_sg_pipeline_make_ui(g_sg.ui_shader, R_FALSE, "raven-ui");
	g_sg.erase_pipeline = rvn_sg_pipeline_make_ui(g_sg.ui_shader, R_TRUE, "raven-erase");
	if (sg_query_pipeline_state(g_sg.ui_pipeline) != SG_RESOURCESTATE_VALID ||
		sg_query_pipeline_state(g_sg.erase_pipeline) != SG_RESOURCESTATE_VALID) return R_FALSE;
	return rvn_sg_blur_init();
}

void rvn_sg_pipelines_shutdown(void) {
	uint32_t i;
	for (i = 0; i < RVN_SG_MAX_EFFECTS; i++) {
		if (g_sg.effects[i].used) rvn_sg_shader_destroy((Rid)i);
	}
}

/* ---------------------------------------------------------------------------
 *  Effects
 * ------------------------------------------------------------------------- */

static uint64_t rvn_sg_hash(const void* data, size_t size) {
	const uint8_t* bytes = (const uint8_t*)data;
	uint64_t hash = 0xcbf29ce484222325ull;
	size_t i;
	for (i = 0; i < size; i++) hash = (hash ^ bytes[i]) * 0x100000001b3ull;
	return hash;
}

/* The shader of an effect from its source (a bundle's chunk for this graphics API, or a built-in's). */
static int rvn_sg_effect_make(rvn_sg_effect_t* effect, const char* source, const char* label) {
	memset(effect, 0, sizeof(*effect));
	effect->shader = rvn_sg_shader_make_ui(source, g_effect_samplers, label);
	if (sg_query_shader_state(effect->shader) != SG_RESOURCESTATE_VALID) return R_FALSE;
	effect->pipeline = rvn_sg_pipeline_make_ui(effect->shader, R_FALSE, label);
	effect->used = sg_query_pipeline_state(effect->pipeline) == SG_RESOURCESTATE_VALID;
	return effect->used;
}

/* The chunk of an effect bundle (rfx.h) that holds the source for the graphics API this library was built for. */
#if defined(RAVEN_SOKOL_GLES3)
#define RVN_SG_BUNDLE_TAG "ES30"
#elif defined(RAVEN_SOKOL_METAL)
#define RVN_SG_BUNDLE_TAG "MSL "
#endif

static int rvn_sg_shader_create_bundle(rvn_sg_effect_t* effect, const void* bundle, size_t size) {
	size_t chunk_size = 0;
	const char* source;
	char* text;
	int ok;
	if (!rvn_rfx_validate(bundle, size)) {
		rvn_error(R_ERROR_INVALID_FILE_FORMAT, R_SEVERITY_ERROR, "Sokol: this effect bundle (.rfx) is damaged");
		return R_FALSE;
	}
	source = (const char*)rvn_rfx_chunk(bundle, size, RVN_SG_BUNDLE_TAG, &chunk_size);
	if (!source || chunk_size == 0) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR,
			"Sokol: this effect bundle has no variant for the graphics API of this build (%s); make it again with tools/rfx/rfx.py: it is drawn as a rectangle", RVN_SG_BUNDLE_TAG);
		return R_FALSE;
	}
	/* (the source in the file is not required to end with a zero) */
	text = (char*)rvn_alloc(chunk_size + 1);
	memcpy(text, source, chunk_size);
	text[chunk_size] = 0;
	ok = rvn_sg_effect_make(effect, text, "raven-bundle");
	rvn_free(text);
	return ok;
}

int rvn_sg_shader_create(Rid shader, const uint32_t* spirv, size_t size) {
	uint64_t hash;
	uint32_t index = R_ID_INDEX(shader);
	size_t i;
	rvn_sg_effect_t* effect;
	if (index >= RVN_SG_MAX_EFFECTS) return R_FALSE;
	effect = &g_sg.effects[index];
	if (rvn_rfx_is_bundle(spirv, size)) return rvn_sg_shader_create_bundle(effect, spirv, size);
	hash = rvn_sg_hash(spirv, size);
	for (i = 0; i < sizeof(rvn_sg_effects) / sizeof(rvn_sg_effects[0]); i++) {
		if (rvn_sg_effects[i].hash == hash) return rvn_sg_effect_make(effect, rvn_sg_effects[i].source, rvn_sg_effects[i].name);
	}
	rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR,
		"Sokol: this SPIR-V effect has no variant for the graphics API (only the effects of Raven do): make an effect bundle with tools/rfx/rfx.py and use the .rfx file; it is drawn as a rectangle");
	return R_FALSE;
}

void rvn_sg_shader_destroy(Rid shader) {
	uint32_t index = R_ID_INDEX(shader);
	rvn_sg_effect_t* effect;
	if (index >= RVN_SG_MAX_EFFECTS) return;
	effect = &g_sg.effects[index];
	if (!effect->used) return;
	sg_destroy_pipeline(effect->pipeline);
	sg_destroy_shader(effect->shader);
	memset(effect, 0, sizeof(*effect));
}

const rvn_sg_effect_t* rvn_sg_effect_find(Rid shader) {
	uint32_t index = R_ID_INDEX(shader);
	if (shader == R_INVALID_ID || index >= RVN_SG_MAX_EFFECTS || !g_sg.effects[index].used) return NULL;
	return &g_sg.effects[index];
}
