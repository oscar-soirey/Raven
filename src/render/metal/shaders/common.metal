/*
 * Raven - common.metal
 * What every Metal library of Raven starts with: the backend puts this file
 * in front of ui.metal, blur.metal and the effects before compiling them
 * (newLibraryWithSource). The layouts and indices are those of
 * docs/METAL_BACKEND.md and metal_backend.h: change all three together.
 */

#include <metal_stdlib>
using namespace metal;

/* primitive kinds (raven_backend.h) */
constant uint RVN_PRIM_RECT     = 0u;
constant uint RVN_PRIM_GLYPH    = 1u;
constant uint RVN_PRIM_SHADOW   = 2u;
constant uint RVN_PRIM_LINE     = 3u;
constant uint RVN_PRIM_TRIANGLE = 4u;
constant uint RVN_PRIM_GLASS    = 5u;
constant uint RVN_PRIM_BLUR     = 6u;
constant uint RVN_PRIM_CURVE    = 7u;
constant uint RVN_PRIM_SHADER   = 8u;
constant uint RVN_PRIM_MASK     = 15u;
constant uint RVN_FLAG_TEXTURED   = 16u;   /* R_INSTANCE_TEXTURED */
constant uint RVN_FLAG_NO_POINTER = 32u;   /* R_INSTANCE_NO_POINTER */

/*
 * One RDrawInstance, byte for byte (80 bytes, see raven_backend.h):
 *   offset  0  rect          float[4]
 *   offset 16  uv            float[4]
 *   offset 32  clip          float[4]
 *   offset 48  radii         float[4]
 *   offset 64  color         uint32_t, bytes R, G, B, A in memory order, straight alpha
 *   offset 68  border_color  uint32_t, the same
 *   offset 72  border_width  float
 *   offset 76  flags         uint32_t
 * The instances are read from a device buffer (not through a vertex
 * descriptor): the four float4 are 16-byte aligned because 80 is a multiple
 * of 16 and the buffer is bound at a multiple of 80.
 */
struct RvnInstance {
	float4 rect;
	float4 uv;
	float4 clip;
	float4 radii;
	uint color;
	uint border_color;
	float border_width;
	uint flags;
};
static_assert(sizeof(RvnInstance) == 80, "RvnInstance must match RDrawInstance");

/*
 * The push constants of the Vulkan UI pipeline (vec2 scale, translate,
 * pointer, unused), set with setVertexBytes. Metal's clip space points up,
 * so scale.y is negative here and the size of the frame, which ui.vert took
 * from scale, is given in the last member instead of `unused`:
 *   scale     = (2 / width, -2 / height)
 *   translate = (-1, 1)
 *   pointer   = RDrawData::pointer, in framebuffer pixels
 *   inv_size  = (1 / width, 1 / height)
 * width and height are those of the render target (the drawable). There is
 * no time here: the time of an effect travels in RDrawInstance::border_width,
 * as with Vulkan.
 */
struct RvnUniforms {
	float2 scale;
	float2 translate;
	float2 pointer;
	float2 inv_size;
};

/*
 * What rvn_ui_vertex hands to the fragment functions: the outputs of
 * ui.vert, location by location. The same structure is the stage_in of
 * rvn_ui_fragment, rvn_shader_fallback_fragment and rvn_effect_fragment.
 */
struct RvnVaryings {
	float4 position     [[position]];
	float2 uv           [[user(rvn_uv)]];                  /* location 0, interpolated */
	float4 rect         [[user(rvn_rect), flat]];          /* location 1 */
	float4 clip         [[user(rvn_clip), flat]];          /* location 2 */
	float4 radii        [[user(rvn_radii), flat]];         /* location 3 */
	float4 color        [[user(rvn_color), flat]];         /* location 4, premultiplied */
	float4 border_color [[user(rvn_border_color), flat]];  /* location 5, premultiplied */
	float  border_width [[user(rvn_border_width), flat]];  /* location 6 (the time of an effect) */
	uint   flags        [[user(rvn_flags), flat]];         /* location 7 */
	float4 params       [[user(rvn_params), flat]];        /* location 8: the uv of the instance as they are */
	float2 inv_size     [[user(rvn_inv_size), flat]];      /* location 9: 1 / size of the target */
	float4 pointer      [[user(rvn_pointer), flat]];       /* location 10: pointer (pixels), hover, press */
};

/* blur.frag's push constants, set with setFragmentBytes (16 bytes) */
struct RvnBlurUniforms {
	float2 step;   /* size of a texel of the source, times the spread of the filter */
	int mode;      /* 0 reduce, 1 enlarge, 2 copy */
	int pad;
};

struct RvnBlurVaryings {
	float4 position [[position]];
	float2 uv       [[user(rvn_blur_uv)]];
};

/* the sampler of every backdrop read and of the blur passes: the sampler_linear of the Vulkan backend */
constexpr sampler rvn_linear(coord::normalized, address::clamp_to_edge, filter::linear, mip_filter::none);

/* GLSL's mod (x - y * floor(x / y)); MSL's fmod truncates and differs for negative values */
inline float  rvn_mod(float x, float y)   { return x - y * floor(x / y); }
inline float2 rvn_mod(float2 x, float2 y) { return x - y * floor(x / y); }
inline float3 rvn_mod(float3 x, float3 y) { return x - y * floor(x / y); }

/* Rounded box centered on the origin; radii = top-left, top-right, bottom-right, bottom-left (ui.frag's sd_round_box). */
float rvn_sd_round_box(float2 p, float2 half_size, float4 radii) {
	float radius = p.x < 0.0 ? (p.y < 0.0 ? radii.x : radii.w) : (p.y < 0.0 ? radii.y : radii.z);
	radius = min(radius, min(half_size.x, half_size.y));
	float2 q = abs(p) - half_size + radius;
	return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;
}
