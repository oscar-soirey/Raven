#version 450
#extension GL_GOOGLE_include_directive : require
#include "raven_effect.glsl"

/* An effect of the application: waves of the two colors, rising; they swell under the pointer.
 * Compiled with: glslangValidator -V -I<raven>/include/raven/shaders wave.frag -o wave.spv */
vec4 raven_effect(RavenEffect fx) {
	float swell = 0.0;
	if (raven_has_pointer(fx)) {
		float d = (fx.pixel.x - fx.pointer.x) / max(fx.size.y, 1.0);
		swell = fx.hover * exp(-d * d * 4.0);
	}
	float wave = sin(fx.uv.x * 9.0 + fx.time * 2.0) * (0.04 + 0.08 * swell);
	float band = fract((fx.uv.y + wave) * 6.0 - fx.time * 0.3);
	float stripe = smoothstep(0.0, 0.5, band) * (1.0 - smoothstep(0.5, 1.0, band));
	return mix(fx.color, fx.color2, stripe);
}
