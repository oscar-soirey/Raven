/**
 * @file raven_shader.h
 * @brief Effects: fragment shaders that color a rounded rectangle.
 *
 * An effect is a fragment shader that Raven runs on a rectangle of a widget,
 * with rounded corners, anti-aliased edges, the clipping and the opacity of
 * the widget already taken care of. It receives where the pixel is (0..1 in
 * the rectangle, and in pixels), the size of the rectangle, two colors, four
 * parameters, the time, an optional texture and, when it asks for it, what
 * was drawn behind the rectangle (sharp and blurred).
 *
 * Raven comes with effects of its own (REffect: aurora, shimmer, neon edge,
 * ripple, plasma, grain, spotlight, mesh gradient), and an application can
 * write its own in GLSL, compiled to SPIR-V with glslangValidator or glslc:
 *
 *     #version 450
 *     #extension GL_GOOGLE_include_directive : require
 *     #include "raven_effect.glsl"          // include/raven/shaders in a package
 *
 *     vec4 raven_effect(RavenEffect fx) {
 *         float wave = 0.5 + 0.5 * sin(fx.uv.x * 12.0 + fx.time * 3.0);
 *         return mix(fx.color, fx.color2, wave);   // straight alpha
 *     }
 *
 *     glslangValidator -V -I<raven package>/include/raven/shaders wave.frag -o wave.spv
 *
 * An effect written this way works on the Vulkan backend as a .spv. To run on every backend (Vulkan, and the Sokol backend:
 * OpenGL ES 3 / WebGL 2 and Metal, so also iOS, macOS and the web), make an effect bundle (.rfx) from it: the effect compiled
 * for each of them in one file (tools/rfx/rfx.py, which needs glslangValidator and spirv-cross when the bundle is made, and
 * nothing at run time):
 *
 *     python3 tools/rfx/rfx.py wave.frag -o wave.rfx --animated [--backdrop]
 *
 * and use wave.rfx wherever wave.spv was used (Rshader_create_from_file, Effect="wave.rfx"); the flags of the bundle
 * (--backdrop, --animated) are added to the ones given.
 *
 * Then, in XML:   <Border Effect="wave.spv" EffectColor="#0A84FF" EffectColor2="#BF5AF2" CornerRadius="16"/>
 * or in C:        Rid wave = Rshader_create_from_file("wave.spv", 0);
 *                 Rborder_set_effect(border, wave, &look);   // or Rdraw_shader in a paint callback
 *
 * Effects follow the pointer: fx.pointer is where it is (pixels, from the
 * corner of the rectangle), fx.hover and fx.press how much it is over the
 * widget and pressing it. The effects of Raven all react to it.
 *
 * The guide docs/shaders.html describes everything an effect can use.
 * The Vulkan backend runs effects from SPIR-V or from a bundle, the Sokol backend from a bundle (and the effects of Raven);
 * a backend without them (R_BACKEND_NULL, a custom backend whose CreateShader is NULL) draws the rectangle in `color`.
 */

#ifndef RAVEN_SHADER_H
#define RAVEN_SHADER_H

#include "raven_core.h"
#include "raven_draw.h"

/** The effects that come with Raven (Reffect_get). */
typedef enum REffect{
	/** Soft lights that drift slowly: color, color2; params: speed, scale, intensity, -. Animated. */
	R_EFFECT_AURORA = 0x0B40,
	/** A band of light that sweeps across (loading placeholders): color = base, color2 = light; params: speed, width, angle, -. Animated. */
	R_EFFECT_SHIMMER,
	/** A glowing edge whose colors run around the shape: color, color2; params: speed, width, glow, -. Animated. */
	R_EFFECT_NEON,
	/** Rings that ripple what is behind, like water (reads the backdrop): color = tint; params: speed, strength, density, center x (0..1). Animated. */
	R_EFFECT_RIPPLE,
	/** Bands of color that flow into each other: color, color2; params: speed, scale, contrast, -. Animated. */
	R_EFFECT_PLASMA,
	/** A gradient with the fine grain of film: color (top left) to color2 (bottom right); params: grain, angle, -, -. Animated grain. */
	R_EFFECT_GRAIN,
	/** A soft light that follows a point: color = light, color2 = background; params: x, y (0..1), radius (0..1), -. */
	R_EFFECT_SPOTLIGHT,
	/** Four colors blended across the surface, slowly moving: color, color2 and their mix; params: speed, -, -, -. Animated. */
	R_EFFECT_MESH,
	R_EFFECT_LAST_
}REffect;

/** Flags of Rshader_create. */
typedef enum RShaderFlags{
	/** The effect reads what is drawn behind it (raven_backdrop, raven_backdrop_blur). */
	R_SHADER_BACKDROP = 1u << 0,
	/** The effect changes with time: the window keeps drawing frames while it is shown. */
	R_SHADER_ANIMATED = 1u << 1
}RShaderFlags;

/** How an effect is drawn. Start from Rshader_look_init. */
typedef struct RShaderLook {
	/** First and second color, 0..1, straight alpha. */
	float color[4];
	float color2[4];
	/** Four numbers the effect reads as fx.params (their meaning is the effect's). */
	float params[4];
	/** Radius of the corners, in logical units. */
	float radius;
	/** Texture the effect reads with raven_texture(), or R_INVALID_ID. */
	Rid texture;
	/** -1: animated when the shader is (R_SHADER_ANIMATED); R_TRUE / R_FALSE forces it. */
	int animate;
	/** R_TRUE (the default): the effect knows where the pointer is (fx.pointer, fx.hover, fx.press). */
	int pointer;
	/**
	 * How much the pointer is over it and how much it is pressed, 0..1, as
	 * the effect reads them (fx.hover, fx.press). -1 (the default): 1 while
	 * the pointer is in the rectangle, and while the left button is down
	 * there; a Border eases them itself.
	 */
	float hover;
	float press;
} RShaderLook;

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @brief A shader from SPIR-V code (a fragment shader written with
	 * raven_effect.glsl, as in a .spv file) or from an effect bundle (a .rfx
	 * file, see above): `size` bytes, copied. R_INVALID_ID when the data is
	 * neither, or a bundle is damaged; a backend that cannot run it (the
	 * Sokol backend with SPIR-V, a bundle without the variant of its graphics
	 * API) reports an error and the effect is drawn as a rectangle.
	 */
	RAVEN_API Rid Rshader_create(const void* spirv, size_t size, Ruint flags);
	/** @brief The same, from a .spv or .rfx file (UTF-8 path, relative to the current folder). */
	RAVEN_API Rid Rshader_create_from_file(const char* path, Ruint flags);
	RAVEN_API void Rshader_destroy(Rid shader);
	RAVEN_API int Rshader_is_valid(Rid shader);
	RAVEN_API Ruint Rshader_get_flags(Rid shader);

	/** @brief A built-in effect, created the first time it is asked for. */
	RAVEN_API Rid Reffect_get(REffect effect);
	/** @brief A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain", "spotlight", "mesh"), or R_INVALID_ID. */
	RAVEN_API Rid Reffect_find(const char* name);
	RAVEN_API const char* Reffect_get_name(REffect effect);
	/** @brief Its colors and parameters when nothing else is given (Rshader_look_init uses them). */
	RAVEN_API void Reffect_get_defaults(REffect effect, RShaderLook* look);

	/** @brief White to white, no parameter, no texture, square corners, animated when the shader is. */
	RAVEN_API void Rshader_look_init(RShaderLook* look);
	/** @brief Fills a look with the defaults of the shader when it is a built-in effect (else Rshader_look_init). */
	RAVEN_API void Rshader_look_defaults(Rid shader, RShaderLook* look);

	/**
	 * @brief From a paint callback: draws the effect over a rectangle of the
	 * widget (logical units, from its top left corner).
	 */
	RAVEN_API void Rdraw_shader(RPainter* painter, Rid shader, float x, float y, float width, float height, const RShaderLook* look);

	/**
	 * @brief The background of a border becomes an effect (its Background,
	 * if any, is drawn under it; its child over it). R_INVALID_ID removes it.
	 * XML: Effect="aurora" or Effect="file.spv", EffectColor, EffectColor2,
	 * EffectParams="a,b,c,d", EffectAnimate, EffectBackdrop (for a file).
	 */
	RAVEN_API void Rborder_set_effect(Rid border, Rid shader, const RShaderLook* look);
	RAVEN_API Rid Rborder_get_effect(Rid border);

#ifdef __cplusplus
}
#endif

#endif /* RAVEN_SHADER_H */
