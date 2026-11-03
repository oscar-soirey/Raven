#version 450

/*
 * Raven - reference fragment shader.
 *
 * Shapes are evaluated as signed distances in pixels, which gives exact
 * anti-aliasing at any size without tessellation. The output color is
 * premultiplied: blend with (ONE, ONE_MINUS_SRC_ALPHA).
 */

const uint PRIM_RECT     = 0u;
const uint PRIM_GLYPH    = 1u;
const uint PRIM_SHADOW   = 2u;
const uint PRIM_LINE     = 3u;
const uint PRIM_TRIANGLE = 4u;
const uint PRIM_GLASS    = 5u;
const uint PRIM_BLUR     = 6u;
const uint PRIM_CURVE    = 7u;
const uint PRIM_MASK     = 15u;
const uint FLAG_TEXTURED = 16u;

layout(set = 0, binding = 0) uniform sampler2D u_texture;
/* the frame behind the glass primitives, as captured by the last backdrop command */
layout(set = 1, binding = 0) uniform sampler2D u_backdrop;
layout(set = 1, binding = 1) uniform sampler2D u_backdrop_blur;

layout(location = 0) in vec2 v_uv;
layout(location = 1) flat in vec4 v_rect;
layout(location = 2) flat in vec4 v_clip;
layout(location = 3) flat in vec4 v_radii;
layout(location = 4) flat in vec4 v_color;
layout(location = 5) flat in vec4 v_border_color;
layout(location = 6) flat in float v_border_width;
layout(location = 7) flat in uint v_flags;
layout(location = 8) flat in vec4 v_params;
layout(location = 9) flat in vec2 v_inv_size;

layout(location = 0) out vec4 out_color;

/* Rounded box centered on the origin; radii = top-left, top-right, bottom-right, bottom-left. */
float sd_round_box(vec2 p, vec2 half_size, vec4 radii) {
	float radius = p.x < 0.0 ? (p.y < 0.0 ? radii.x : radii.w) : (p.y < 0.0 ? radii.y : radii.z);
	radius = min(radius, min(half_size.x, half_size.y));
	vec2 q = abs(p) - half_size + radius;
	return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;
}

/* Same distance, and the direction pointing out of the shape (for the rim of glass). */
float sd_round_box_normal(vec2 p, vec2 half_size, vec4 radii, out vec2 normal) {
	float radius = p.x < 0.0 ? (p.y < 0.0 ? radii.x : radii.w) : (p.y < 0.0 ? radii.y : radii.z);
	radius = min(radius, min(half_size.x, half_size.y));
	vec2 side = vec2(p.x < 0.0 ? -1.0 : 1.0, p.y < 0.0 ? -1.0 : 1.0);
	vec2 q = abs(p) - half_size + radius;
	float corner = length(max(q, 0.0));
	if (q.x > 0.0 && q.y > 0.0) normal = side * (q / max(corner, 1e-4));
	else normal = q.x > q.y ? vec2(side.x, 0.0) : vec2(0.0, side.y);
	return min(max(q.x, q.y), 0.0) + corner - radius;
}

/*
 * Glass: a thick, rounded slab. Its flat middle shows the backdrop, more or
 * less frosted; its curved rim bends it like a lens, splits the colors a
 * little, and catches the light.
 */
vec4 glass(vec2 p, float d, vec2 normal) {
	float frost = v_params.x;
	float refraction = v_params.y;
	float bevel = max(v_border_width, 1.0);

	/* 1 on the edge, 0 where the rim meets the flat part; the cube keeps the middle undistorted */
	float t = clamp(1.0 + d / bevel, 0.0, 1.0);
	float bend = t * t * t;

	/* the rim shows what lies further inside: content appears pulled toward the edge */
	vec2 shift = -normal * bend * refraction;
	vec2 uv = (p + shift) * v_inv_size;
	vec2 spread = shift * 0.14 * v_inv_size;
	vec4 middle = textureLod(u_backdrop, uv, 0.0);
	vec3 sharp = vec3(
		textureLod(u_backdrop, uv + spread, 0.0).r,
		middle.g,
		textureLod(u_backdrop, uv - spread, 0.0).b);
	vec4 wide = textureLod(u_backdrop_blur, uv, 0.0);
	vec3 blurred = wide.rgb;
	vec3 color = mix(sharp, blurred, frost);
	/* how much of the window there is behind: less than 1 where it lets the screen show (see-through windows) */
	float behind = mix(middle.a, wide.a, frost);

	/* vivid colors survive the frosting */
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	color = max(mix(vec3(luma), color, v_params.z), 0.0);

	/* tint (v_color is premultiplied) */
	color = color * (1.0 - v_color.a) + v_color.rgb;

	/* light comes from the top left: a thin bright line on the rim, strongest
	 * where it faces the light, with an echo on the opposite side */
	const vec2 light = vec2(-0.5547, -0.8321);
	float facing = dot(normal, light);
	float lit = 0.22 + (facing > 0.0 ? pow(facing, 1.5) : 0.5 * pow(-facing, 1.5));
	float line_width = clamp(bevel * 0.12, 1.0, 2.5);
	float line = clamp(1.0 + d / line_width, 0.0, 1.0);
	color += v_border_color.rgb * (line * line * lit + bend * 0.10);

	/* the rim is a curved mirror too: it reflects what lies around the slab,
	 * so colors that come close to the glass show on its edge before they
	 * pass under it */
	vec3 around = textureLod(u_backdrop_blur, (p + normal * (bevel * 1.5 + 6.0)) * v_inv_size, 0.0).rgb;
	color += around * (1.0 - color) * bend * 0.45 * v_border_color.a;

	/* a slab is a little brighter at the top than at the bottom */
	float height = max(v_rect.w - v_rect.y, 1.0);
	color += v_border_color.a * 0.07 * (0.5 - (p.y - v_rect.y) / height);

	color += v_params.w;
	float opacity = float((v_flags >> 8) & 255u) / 255.0;
	/* over nothing, glass is only its tint and its light: the screen shows through the rest */
	float alpha = clamp(1.0 - (1.0 - v_color.a) * (1.0 - behind) + v_border_color.a * line * line * lit, 0.0, 1.0);
	return vec4(clamp(color, 0.0, 1.0), alpha) * opacity;
}

/* Fine grain, the same in every frame: what tells frosted glass from a blurred picture. */
float grain(vec2 p) {
	return fract(sin(dot(floor(p), vec2(12.9898, 78.233))) * 43758.5453) - 0.5;
}

/*
 * Blur: what is behind, out of focus by a radius in pixels (v_params.x),
 * with its colors strengthened or weakened (z), a grain (y) and a tint.
 * Small radii are worked out here from the half-size copy of the frame, a
 * ring of samples around the pixel; large ones come from the copy that is
 * already blurred wide, pushed further when the radius asks for more.
 */
vec4 blur(vec2 p) {
	const float WIDE = 22.0;
	float radius = max(v_params.x, 0.0);
	vec2 uv = p * v_inv_size;
	vec4 color;
	if (radius < 0.75) {
		color = textureLod(u_backdrop, uv, 0.0);
	} else {
		/* sixteen samples on a spiral: the golden angle spreads them evenly over the disc */
		vec4 near = vec4(0.0);
		float reach = min(radius, WIDE);
		for (int i = 0; i < 16; i++) {
			float t = (float(i) + 0.5) / 16.0;
			float angle = float(i) * 2.39996323;
			vec2 offset = vec2(cos(angle), sin(angle)) * sqrt(t) * reach;
			near += textureLod(u_backdrop, uv + offset * v_inv_size, 0.0);
		}
		near /= 16.0;
		vec4 far = textureLod(u_backdrop_blur, uv, 0.0);
		if (radius > WIDE) {
			/* wider than the wide copy: a ring of it, as far as the radius goes past */
			float extra = radius - WIDE;
			vec4 ring = vec4(0.0);
			for (int i = 0; i < 8; i++) {
				float angle = float(i) * 0.78539816;
				ring += textureLod(u_backdrop_blur, uv + vec2(cos(angle), sin(angle)) * extra * v_inv_size, 0.0);
			}
			far = mix(far, ring / 8.0, 0.75);
		}
		color = mix(near, far, smoothstep(WIDE * 0.45, WIDE, radius));
	}
	float luma = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
	color.rgb = max(mix(vec3(luma), color.rgb, v_params.z), 0.0);
	color.rgb += grain(p) * v_params.y * 0.04 * color.a;
	/* tint (premultiplied), then a little light or shade over the whole */
	color = color * (1.0 - v_color.a) + v_color;
	color.rgb += v_params.w * color.a;
	float opacity = float((v_flags >> 8) & 255u) / 255.0;
	return clamp(color, 0.0, 1.0) * opacity;
}

float sd_segment(vec2 p, vec2 a, vec2 b) {
	vec2 pa = p - a;
	vec2 ba = b - a;
	float h = clamp(dot(pa, ba) / max(dot(ba, ba), 1e-6), 0.0, 1.0);
	return length(pa - ba * h);
}

/* Works for both windings. */
float sd_triangle(vec2 p, vec2 p0, vec2 p1, vec2 p2) {
	vec2 e0 = p1 - p0, e1 = p2 - p1, e2 = p0 - p2;
	vec2 v0 = p - p0, v1 = p - p1, v2 = p - p2;
	vec2 pq0 = v0 - e0 * clamp(dot(v0, e0) / max(dot(e0, e0), 1e-6), 0.0, 1.0);
	vec2 pq1 = v1 - e1 * clamp(dot(v1, e1) / max(dot(e1, e1), 1e-6), 0.0, 1.0);
	vec2 pq2 = v2 - e2 * clamp(dot(v2, e2) / max(dot(e2, e2), 1e-6), 0.0, 1.0);
	float s = sign(e0.x * e2.y - e0.y * e2.x);
	vec2 d = min(min(
		vec2(dot(pq0, pq0), s * (v0.x * e0.y - v0.y * e0.x)),
		vec2(dot(pq1, pq1), s * (v1.x * e1.y - v1.y * e1.x))),
		vec2(dot(pq2, pq2), s * (v2.x * e2.y - v2.y * e2.x)));
	return -sqrt(d.x) * sign(d.y);
}

void main() {
	/* the derivatives of the coordinates are taken while the whole quad runs
	 * (before the discard and the branches): they choose the mip level of the
	 * textures that have mipmaps; the samplers of the others read level 0 only */
	vec2 uv_dx = dFdx(v_uv), uv_dy = dFdy(v_uv);
	vec2 p = gl_FragCoord.xy;
	if (p.x < v_clip.x || p.y < v_clip.y || p.x >= v_clip.z || p.y >= v_clip.w) discard;

	uint kind = v_flags & PRIM_MASK;
	vec4 color = v_color;
	if ((v_flags & FLAG_TEXTURED) != 0u) {
		vec4 texel = textureGrad(u_texture, v_uv, uv_dx, uv_dy);
		/* textures are straight alpha, v_color is already premultiplied */
		color *= vec4(texel.rgb * texel.a, texel.a);
	}

	float coverage = 1.0;
	if (kind == PRIM_RECT) {
		vec2 half_size = (v_rect.zw - v_rect.xy) * 0.5;
		float d = sd_round_box(p - (v_rect.xy + half_size), half_size, v_radii);
		coverage = clamp(0.5 - d, 0.0, 1.0);
		if (v_border_width > 0.0) {
			float inside = clamp(0.5 - (d + v_border_width), 0.0, 1.0);
			color = mix(v_border_color, color, inside);
		}
	} else if (kind == PRIM_SHADOW) {
		vec2 half_size = (v_rect.zw - v_rect.xy) * 0.5;
		float d = sd_round_box(p - (v_rect.xy + half_size), half_size, v_radii);
		float blur = max(v_border_width, 0.5);
		coverage = 1.0 - smoothstep(-blur, blur, d);
	} else if (kind == PRIM_LINE) {
		float d = sd_segment(p, v_rect.xy, v_rect.zw) - v_radii.x * 0.5;
		coverage = clamp(0.5 - d, 0.0, 1.0);
	} else if (kind == PRIM_TRIANGLE) {
		float d = sd_triangle(p, v_rect.xy, v_rect.zw, v_radii.xy);
		coverage = clamp(0.5 - d, 0.0, 1.0);
	} else if (kind == PRIM_CURVE) {
		/* a cubic curve, followed as a chain of short segments: the nearest one gives the distance and how far along it is */
		vec2 a = v_rect.xy, b = v_radii.xy, c = v_radii.zw, e = v_rect.zw;
		vec2 from = a;
		float nearest = 1e20, along = 0.0;
		for (int i = 1; i <= 24; i++) {
			float t = float(i) / 24.0;
			float u = 1.0 - t;
			vec2 to = u * u * u * a + 3.0 * u * u * t * b + 3.0 * u * t * t * c + t * t * t * e;
			vec2 pa = p - from, ba = to - from;
			float h = clamp(dot(pa, ba) / max(dot(ba, ba), 1e-6), 0.0, 1.0);
			vec2 away = pa - ba * h;
			float d2 = dot(away, away);
			if (d2 < nearest) {
				nearest = d2;
				along = (float(i - 1) + h) / 24.0;
			}
			from = to;
		}
		float d = sqrt(nearest) - v_border_width * 0.5;
		float stroke = clamp(0.5 - d, 0.0, 1.0);
		float pulse = 0.0;
		color = mix(v_color, v_border_color, along);
		float bright = float((v_flags >> 8) & 255u) / 255.0;
		if (v_params.w > 0.0 && bright > 0.0) {
			/* pulses of light that run from one end to the other */
			float f = fract(along * v_params.w - v_params.z) - 0.5;
			pulse = exp(-f * f * 80.0) * bright;
			color.rgb = mix(color.rgb, vec3(color.a), pulse * 0.6);
		}
		float halo = 0.0;
		if (v_params.x > 0.0) {
			halo = 1.0 - smoothstep(0.0, v_params.x, max(d, 0.0));
			halo = halo * halo * v_params.y * (1.0 + pulse * 0.8);
		}
		coverage = stroke + (1.0 - stroke) * min(halo, 1.0);
	} else if (kind == PRIM_GLASS) {
		vec2 half_size = (v_rect.zw - v_rect.xy) * 0.5;
		vec2 normal;
		float d = sd_round_box_normal(p - (v_rect.xy + half_size), half_size, v_radii, normal);
		coverage = clamp(0.5 - d, 0.0, 1.0);
		color = glass(p, d, normal);
	} else if (kind == PRIM_BLUR) {
		vec2 half_size = (v_rect.zw - v_rect.xy) * 0.5;
		float d = sd_round_box(p - (v_rect.xy + half_size), half_size, v_radii);
		coverage = clamp(0.5 - d, 0.0, 1.0);
		color = blur(p);
		if (v_border_width > 0.0) {
			/* a hairline around it, over the blur */
			float inside = clamp(0.5 - (d + v_border_width), 0.0, 1.0);
			color = mix(v_border_color + color * (1.0 - v_border_color.a), color, inside);
		}
	}

	out_color = color * coverage;
}
