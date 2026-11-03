#version 450
#extension GL_GOOGLE_include_directive : require
#include "raven_effect.glsl"

/* The smallest effect: its first color. tests/test_render.c and tests/test_rfx.c use the bundle made from it:
 *   python3 tools/rfx/rfx.py tests/data/solid.frag -o tests/data/solid.rfx --backdrop */
vec4 raven_effect(RavenEffect fx) {
	return fx.color;
}
