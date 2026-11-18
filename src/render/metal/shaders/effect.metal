/*
 * Raven - effect.metal
 * What Raven does around an effect on the Metal backend: the MSL twin of
 * include/raven/shaders/raven_effect.glsl, which stays the reference (the
 * comments and names are kept so that the two can be read side by side).
 *
 * The library of an effect is common.metal + this file + effects/<name>.metal.
 * The file of the effect defines one function:
 *
 *     float4 raven_effect(RAVEN_EFFECT_ARGS) {
 *         return mix(fx.color, fx.color2, fx.uv.x);   // straight alpha, 0..1
 *     }
 *
 * Raven calls it for every pixel of the rectangle. The rounded corners, the
 * anti-aliased edge, the clipping of the widget and its opacity are applied
 * to what it returns: the effect only says what color the pixel is.
 */

/* ---- what the effect receives ---- */

struct RavenEffect {
	/** Where the pixel is in the rectangle: (0, 0) top left .. (1, 1) bottom right. */
	float2 uv;
	/** The same in pixels, from the top left corner of the rectangle. */
	float2 pixel;
	/** Size of the rectangle, in pixels. */
	float2 size;
	/** Where the pixel is in the window, in pixels (for raven_backdrop). */
	float2 screen;
	/** Distance to the edge of the rounded rectangle, in pixels: negative inside, 0 on the edge. */
	float edge;
	/** Seconds: grows while the effect is animated. */
	float time;
	/**
	 * Where the pointer is, in pixels from the top left corner of the
	 * rectangle, like `pixel` (it may be outside the rectangle). Far away
	 * (below -1e5) when the pointer is not over the window, or when the
	 * effect is told not to follow it (RShaderLook::pointer, EffectPointer).
	 */
	float2 pointer;
	/** How much the pointer is over the widget, 0..1: eased, it rises and falls in a fraction of a second. */
	float hover;
	/** How much the widget is pressed, 0..1 (eased). */
	float press;
	/** The two colors of the look, straight alpha. */
	float4 color;
	float4 color2;
	/** The four parameters of the look. */
	float4 params;
};

/*
 * MSL functions cannot see the bindings of the entry point: they are passed
 * to raven_effect, and the macros below read them by these names.
 */
#define RAVEN_EFFECT_ARGS RavenEffect fx, texture2d<float> rvn_texture, sampler rvn_texture_sampler, \
	texture2d<float> rvn_backdrop, texture2d<float> rvn_backdrop_blur, float2 rvn_inv_size

/** The texture of the look (RShaderLook::texture), at uv 0..1. White when there is none. */
#define raven_texture(uv)           rvn_texture.sample(rvn_texture_sampler, (uv))
/**
 * What was drawn behind the rectangle, at a position of the window in
 * pixels: sharp, or blurred. Only for a shader created with
 * R_SHADER_BACKDROP (EffectBackdrop="true"); white otherwise.
 */
#define raven_backdrop(screen)      rvn_backdrop.sample(rvn_linear, (screen) * rvn_inv_size, level(0.0))
#define raven_backdrop_blur(screen) rvn_backdrop_blur.sample(rvn_linear, (screen) * rvn_inv_size, level(0.0))

/** The effect: the color of the pixel, straight alpha (not premultiplied). Defined by the file of the effect. */
float4 raven_effect(RAVEN_EFFECT_ARGS);

/* ---- helpers ---- */

constant float RAVEN_PI = 3.14159265358979;
constant float RAVEN_TAU = 6.28318530717959;

/** A number in 0..1 that looks random, the same for the same point. */
float raven_hash(float2 p) {
	p = fract(p * float2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

/** Smooth noise in 0..1. */
float raven_noise(float2 p) {
	float2 i = floor(p);
	float2 f = fract(p);
	float2 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(raven_hash(i), raven_hash(i + float2(1.0, 0.0)), u.x),
		mix(raven_hash(i + float2(0.0, 1.0)), raven_hash(i + float2(1.0, 1.0)), u.x), u.y);
}

/** Noise of noises (fractal Brownian motion), in 0..1. */
float raven_fbm(float2 p) {
	float sum = 0.0, amplitude = 0.5;
	for (int i = 0; i < 5; i++) {
		sum += amplitude * raven_noise(p);
		p = p * 2.03 + float2(17.1, 9.2);
		amplitude *= 0.5;
	}
	return sum;
}

/** Hue (0..1), saturation, value -> RGB. */
float3 raven_hsv(float3 c) {
	float3 rgb = clamp(abs(rvn_mod(c.x * 6.0 + float3(0.0, 4.0, 2.0), float3(6.0)) - 3.0) - 1.0, 0.0, 1.0);
	return c.z * mix(float3(1.0), rgb, c.y);
}

/** R_TRUE when the effect knows where the pointer is (it is over the window, and the effect follows it). */
bool raven_has_pointer(RavenEffect fx) {
	return fx.pointer.x > -1e5;
}

/** Signed distance to a rounded box centered on the origin (radii: top-left, top-right, bottom-right, bottom-left). */
float raven_sd_round_box(float2 p, float2 half_size, float4 radii) {
	return rvn_sd_round_box(p, half_size, radii);
}

/* ---- what Raven does around the effect ---- */

fragment float4 rvn_effect_fragment(RvnVaryings in [[stage_in]],
	texture2d<float> rvn_texture [[texture(0)]], sampler rvn_texture_sampler [[sampler(0)]],
	texture2d<float> rvn_backdrop [[texture(1)]], texture2d<float> rvn_backdrop_blur [[texture(2)]]) {
	float2 p = in.position.xy;
	if (p.x < in.clip.x || p.y < in.clip.y || p.x >= in.clip.z || p.y >= in.clip.w) {
		discard_fragment();
		return float4(0.0);
	}
	float2 size = max(in.rect.zw - in.rect.xy, float2(1.0));
	float2 center = (in.rect.xy + in.rect.zw) * 0.5;
	float d = rvn_sd_round_box(p - center, size * 0.5, in.radii);
	float coverage = clamp(0.5 - d, 0.0, 1.0);
	if (coverage <= 0.0) {
		discard_fragment();
		return float4(0.0);
	}

	RavenEffect fx;
	fx.pixel = p - in.rect.xy;
	fx.uv = fx.pixel / size;
	fx.size = size;
	fx.screen = p;
	fx.edge = d;
	fx.time = in.border_width;
	fx.pointer = in.pointer.xy - in.rect.xy;
	fx.hover = in.pointer.z;
	fx.press = in.pointer.w;
	/* (the vertex function premultiplies the colors: the effect gets them straight) */
	fx.color = in.color.a > 0.0 ? float4(in.color.rgb / in.color.a, in.color.a) : float4(0.0);
	fx.color2 = in.border_color.a > 0.0 ? float4(in.border_color.rgb / in.border_color.a, in.border_color.a) : float4(0.0);
	fx.params = in.params;

	float4 color = raven_effect(fx, rvn_texture, rvn_texture_sampler, rvn_backdrop, rvn_backdrop_blur, in.inv_size);
	color = clamp(color, float4(0.0), float4(1.0));
	float opacity = float((in.flags >> 8) & 255u) / 255.0;
	return float4(color.rgb * color.a, color.a) * (coverage * opacity);
}
