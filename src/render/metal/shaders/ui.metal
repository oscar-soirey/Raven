/*
 * Raven - ui.metal
 * The UI of the Metal backend: src/render/vulkan/shaders/ui.vert and ui.frag
 * in the Metal Shading Language, line by line (the GLSL stays the reference:
 * a change to one is carried to the other by reading them side by side).
 * Compiled after common.metal.
 *
 * Every primitive is one instance (RDrawInstance, 80 bytes). No vertex
 * buffer is bound per vertex: the four corners of a quad are generated from
 * the vertex index (triangle strip) and the quad is sized to cover the
 * primitive. The fragment function computes the exact shape analytically.
 *
 * Shapes are evaluated as signed distances in pixels, which gives exact
 * anti-aliasing at any size without tessellation. The output color is
 * premultiplied: blend with (ONE, ONE_MINUS_SOURCE_ALPHA).
 */

vertex RvnVaryings rvn_ui_vertex(uint vid [[vertex_id]], uint iid [[instance_id]],
	const device RvnInstance* instances [[buffer(0)]], constant RvnUniforms& u [[buffer(1)]]) {
	RvnInstance inst = instances[iid];
	/* the colors are bytes R, G, B, A in memory order: the R8G8B8A8_UNORM attributes of Vulkan */
	float4 in_color = unpack_unorm4x8_to_float(inst.color);
	float4 in_border_color = unpack_unorm4x8_to_float(inst.border_color);
	RvnVaryings out;

	uint kind = inst.flags & RVN_PRIM_MASK;
	float2 corner = float2(float(vid & 1u), float(vid >> 1u));
	float2 lo = inst.rect.xy;
	float2 hi = inst.rect.zw;

	if (kind == RVN_PRIM_RECT || kind == RVN_PRIM_GLASS || kind == RVN_PRIM_BLUR || kind == RVN_PRIM_SHADER) {
		/* half a pixel more for the anti-aliased edge of unsnapped rectangles */
		lo -= 0.5;
		hi += 0.5;
	} else if (kind == RVN_PRIM_SHADOW) {
		float grow = inst.border_width + 1.0;
		lo -= grow;
		hi += grow;
	} else if (kind == RVN_PRIM_LINE) {
		float grow = inst.radii.x * 0.5 + 1.0;
		lo = min(inst.rect.xy, inst.rect.zw) - grow;
		hi = max(inst.rect.xy, inst.rect.zw) + grow;
	} else if (kind == RVN_PRIM_CURVE) {
		/* a curve stays inside its four points */
		float grow = inst.border_width * 0.5 + inst.uv.x + 1.0;
		lo = min(min(inst.rect.xy, inst.rect.zw), min(inst.radii.xy, inst.radii.zw)) - grow;
		hi = max(max(inst.rect.xy, inst.rect.zw), max(inst.radii.xy, inst.radii.zw)) + grow;
	} else if (kind == RVN_PRIM_TRIANGLE) {
		lo = min(min(inst.rect.xy, inst.rect.zw), inst.radii.xy) - 1.0;
		hi = max(max(inst.rect.xy, inst.rect.zw), inst.radii.xy) + 1.0;
	}

	float2 position = mix(lo, hi, corner);

	/* texture coordinates follow the rectangle, also outside of it */
	float2 size = max(inst.rect.zw - inst.rect.xy, float2(1e-6));
	out.uv = mix(inst.uv.xy, inst.uv.zw, (position - inst.rect.xy) / size);

	out.rect = inst.rect;
	out.clip = inst.clip;
	out.radii = inst.radii;
	out.color = float4(in_color.rgb * in_color.a, in_color.a);
	out.border_color = float4(in_border_color.rgb * in_border_color.a, in_border_color.a);
	out.border_width = inst.border_width;
	out.flags = inst.flags;
	out.params = inst.uv;
	out.inv_size = u.inv_size;
	out.pointer = (inst.flags & RVN_FLAG_NO_POINTER) != 0u ? float4(-1e6, -1e6, 0.0, 0.0)
		: float4(u.pointer, float((inst.flags >> 16) & 255u) / 255.0, float(inst.flags >> 24) / 255.0);

	out.position = float4(position * u.scale + u.translate, 0.0, 1.0);
	return out;
}

/* Same distance as rvn_sd_round_box, and the direction pointing out of the shape (for the rim of glass). */
float sd_round_box_normal(float2 p, float2 half_size, float4 radii, thread float2& normal) {
	float radius = p.x < 0.0 ? (p.y < 0.0 ? radii.x : radii.w) : (p.y < 0.0 ? radii.y : radii.z);
	radius = min(radius, min(half_size.x, half_size.y));
	float2 side = float2(p.x < 0.0 ? -1.0 : 1.0, p.y < 0.0 ? -1.0 : 1.0);
	float2 q = abs(p) - half_size + radius;
	float corner = length(max(q, 0.0));
	if (q.x > 0.0 && q.y > 0.0) normal = side * (q / max(corner, 1e-4));
	else normal = q.x > q.y ? float2(side.x, 0.0) : float2(0.0, side.y);
	return min(max(q.x, q.y), 0.0) + corner - radius;
}

/*
 * Dispersion: glass bends violet more than red, so what its rim shows is
 * split into a thin rainbow. Seven wavelengths, from red (bent least) to
 * violet (bent most), each read where it lands; their weights add up to
 * white, so a plain backdrop stays plain and the colors only show where
 * the backdrop changes: on edges, text, shapes passing under the rim.
 */
constant float3 SPECTRUM[7] = {
	float3(0.2788, 0.0146, 0.0000),
	float3(0.3163, 0.0831, 0.0011),
	float3(0.2022, 0.2356, 0.0133),
	float3(0.0728, 0.3333, 0.0812),
	float3(0.0165, 0.2356, 0.2467),
	float3(0.0325, 0.0831, 0.3742),
	float3(0.0810, 0.0146, 0.2834)
};

/* What lies behind p seen through a rim that bends it by `shift` pixels, each wavelength by `spread` more or less. */
float3 dispersed(texture2d<float> source, float2 p, float2 shift, float spread, float2 inv_size) {
	float3 sum = float3(0.0);
	for (int i = 0; i < 7; i++) {
		float k = float(i) / 3.0 - 1.0;
		sum += source.sample(rvn_linear, (p + shift * (1.0 + spread * k)) * inv_size, level(0.0)).rgb * SPECTRUM[i];
	}
	return sum;
}

/*
 * Glass: a thick, rounded slab. Its flat middle shows the backdrop, more or
 * less frosted; its curved rim bends it like a lens, splits the colors a
 * little, and catches the light.
 */
float4 glass(thread const RvnVaryings& in, float2 p, float d, float2 normal,
	texture2d<float> u_backdrop, texture2d<float> u_backdrop_blur) {
	float frost = in.params.x;
	float refraction = in.params.y;
	float bevel = max(in.border_width, 1.0);

	/* 1 on the edge, 0 where the rim meets the flat part; the cube keeps the middle undistorted */
	float t = clamp(1.0 + d / bevel, 0.0, 1.0);
	float bend = t * t * t;

	/* the rim shows what lies further inside: content appears pulled toward the edge */
	float2 shift = -normal * bend * refraction;
	float2 uv = (p + shift) * in.inv_size;
	float4 middle = u_backdrop.sample(rvn_linear, uv, level(0.0));
	float4 wide = u_backdrop_blur.sample(rvn_linear, uv, level(0.0));
	float3 sharp = middle.rgb;
	float3 blurred = wide.rgb;
	/* the rim splits the colors (the flat middle reads one sample); a frosted
	 * backdrop is softer, so it is spread a little further to keep its rainbow */
	if (bend > 0.002 && refraction > 0.0) {
		if (frost < 0.999) sharp = dispersed(u_backdrop, p, shift, 0.50, in.inv_size);
		if (frost > 0.001) blurred = dispersed(u_backdrop_blur, p, shift, 0.70, in.inv_size);
	}
	float3 color = mix(sharp, blurred, frost);
	/* how much of the window there is behind: less than 1 where it lets the screen show (see-through windows) */
	float behind = mix(middle.a, wide.a, frost);

	/* vivid colors survive the frosting */
	float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
	color = max(mix(float3(luma), color, in.params.z), 0.0);

	/* tint (in.color is premultiplied) */
	color = color * (1.0 - in.color.a) + in.color.rgb;

	/* light comes from the top left: a thin bright line on the rim, strongest
	 * where it faces the light, with an echo on the opposite side */
	const float2 light = float2(-0.5547, -0.8321);
	float facing = dot(normal, light);
	float lit = 0.22 + (facing > 0.0 ? pow(facing, 1.5) : 0.5 * pow(-facing, 1.5));
	float line_width = clamp(bevel * 0.12, 1.0, 2.5);
	float line = clamp(1.0 + d / line_width, 0.0, 1.0);
	color += in.border_color.rgb * (line * line * lit + bend * 0.10);

	/* the rim is a curved mirror too: it reflects what lies around the slab,
	 * so colors that come close to the glass show on its edge before they
	 * pass under it */
	float3 around = u_backdrop_blur.sample(rvn_linear, (p + normal * (bevel * 1.5 + 6.0)) * in.inv_size, level(0.0)).rgb;
	color += around * (1.0 - color) * bend * 0.45 * in.border_color.a;

	/* a slab is a little brighter at the top than at the bottom */
	float height = max(in.rect.w - in.rect.y, 1.0);
	color += in.border_color.a * 0.07 * (0.5 - (p.y - in.rect.y) / height);

	color += in.params.w;
	float opacity = float((in.flags >> 8) & 255u) / 255.0;
	/* over nothing, glass is only its tint and its light: the screen shows through the rest */
	float alpha = clamp(1.0 - (1.0 - in.color.a) * (1.0 - behind) + in.border_color.a * line * line * lit, 0.0, 1.0);
	return float4(clamp(color, 0.0, 1.0), alpha) * opacity;
}

/* Fine grain, the same in every frame: what tells frosted glass from a blurred picture. */
float grain(float2 p) {
	return fract(sin(dot(floor(p), float2(12.9898, 78.233))) * 43758.5453) - 0.5;
}

/*
 * Blur: what is behind, out of focus by a radius in pixels (in.params.x),
 * with its colors strengthened or weakened (z), a grain (y) and a tint.
 * Small radii are worked out here from the half-size copy of the frame, a
 * ring of samples around the pixel; large ones come from the copy that is
 * already blurred wide, pushed further when the radius asks for more.
 */
float4 blur(thread const RvnVaryings& in, float2 p, float d, float2 normal, float band,
	texture2d<float> u_backdrop, texture2d<float> u_backdrop_blur) {
	const float WIDE = 22.0;
	float radius = max(in.params.x, 0.0);
	float2 uv = p * in.inv_size;
	float4 color;
	if (radius < 0.75) {
		color = u_backdrop.sample(rvn_linear, uv, level(0.0));
	} else {
		/* sixteen samples on a spiral: the golden angle spreads them evenly over the disc */
		float4 near = float4(0.0);
		float reach = min(radius, WIDE);
		for (int i = 0; i < 16; i++) {
			float t = (float(i) + 0.5) / 16.0;
			float angle = float(i) * 2.39996323;
			float2 offset = float2(cos(angle), sin(angle)) * sqrt(t) * reach;
			near += u_backdrop.sample(rvn_linear, uv + offset * in.inv_size, level(0.0));
		}
		near /= 16.0;
		float4 far = u_backdrop_blur.sample(rvn_linear, uv, level(0.0));
		if (radius > WIDE) {
			/* wider than the wide copy: a ring of it, as far as the radius goes past */
			float extra = radius - WIDE;
			float4 ring = float4(0.0);
			for (int i = 0; i < 8; i++) {
				float angle = float(i) * 0.78539816;
				ring += u_backdrop_blur.sample(rvn_linear, uv + float2(cos(angle), sin(angle)) * extra * in.inv_size, level(0.0));
			}
			far = mix(far, ring / 8.0, 0.75);
		}
		color = mix(near, far, smoothstep(WIDE * 0.45, WIDE, radius));
	}
	/* its edge is the edge of a pane: a narrow band that bends what is behind
	 * toward the inside and splits it into a faint rainbow, as glass does */
	float t = clamp(1.0 + d / band, 0.0, 1.0);
	float bend = t * t * t;
	if (bend > 0.002) {
		/* (read from the sharp frame: in the blurred one, the colors would have nothing left to split) */
		float3 rim = dispersed(u_backdrop, p, -normal * bend * band * 1.2, 0.80, in.inv_size);
		color.rgb = mix(color.rgb, rim * color.a, bend * 0.55);
	}
	float luma = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
	color.rgb = max(mix(float3(luma), color.rgb, in.params.z), 0.0);
	color.rgb += grain(p) * in.params.y * 0.04 * color.a;
	/* tint (premultiplied), then a little light or shade over the whole */
	color = color * (1.0 - in.color.a) + in.color;
	color.rgb += in.params.w * color.a;
	float opacity = float((in.flags >> 8) & 255u) / 255.0;
	return clamp(color, 0.0, 1.0) * opacity;
}

float sd_segment(float2 p, float2 a, float2 b) {
	float2 pa = p - a;
	float2 ba = b - a;
	float h = clamp(dot(pa, ba) / max(dot(ba, ba), 1e-6), 0.0, 1.0);
	return length(pa - ba * h);
}

/* Works for both windings. */
float sd_triangle(float2 p, float2 p0, float2 p1, float2 p2) {
	float2 e0 = p1 - p0, e1 = p2 - p1, e2 = p0 - p2;
	float2 v0 = p - p0, v1 = p - p1, v2 = p - p2;
	float2 pq0 = v0 - e0 * clamp(dot(v0, e0) / max(dot(e0, e0), 1e-6), 0.0, 1.0);
	float2 pq1 = v1 - e1 * clamp(dot(v1, e1) / max(dot(e1, e1), 1e-6), 0.0, 1.0);
	float2 pq2 = v2 - e2 * clamp(dot(v2, e2) / max(dot(e2, e2), 1e-6), 0.0, 1.0);
	float s = sign(e0.x * e2.y - e0.y * e2.x);
	float2 d = min(min(
		float2(dot(pq0, pq0), s * (v0.x * e0.y - v0.y * e0.x)),
		float2(dot(pq1, pq1), s * (v1.x * e1.y - v1.y * e1.x))),
		float2(dot(pq2, pq2), s * (v2.x * e2.y - v2.y * e2.x)));
	return -sqrt(d.x) * sign(d.y);
}

fragment float4 rvn_ui_fragment(RvnVaryings in [[stage_in]],
	texture2d<float> u_texture [[texture(0)]], sampler u_sampler [[sampler(0)]],
	texture2d<float> u_backdrop [[texture(1)]], texture2d<float> u_backdrop_blur [[texture(2)]]) {
	/* the derivatives of the coordinates are taken while the whole quad runs
	 * (before the discard and the branches): they choose the mip level of the
	 * textures that have mipmaps; the samplers of the others read level 0 only */
	float2 uv_dx = dfdx(in.uv), uv_dy = dfdy(in.uv);
	float2 p = in.position.xy;
	if (p.x < in.clip.x || p.y < in.clip.y || p.x >= in.clip.z || p.y >= in.clip.w) {
		discard_fragment();
		return float4(0.0);
	}

	uint kind = in.flags & RVN_PRIM_MASK;
	float4 color = in.color;
	if ((in.flags & RVN_FLAG_TEXTURED) != 0u) {
		float4 texel = u_texture.sample(u_sampler, in.uv, gradient2d(uv_dx, uv_dy));
		/* textures are straight alpha, in.color is already premultiplied */
		color *= float4(texel.rgb * texel.a, texel.a);
	}

	float coverage = 1.0;
	if (kind == RVN_PRIM_RECT) {
		float2 half_size = (in.rect.zw - in.rect.xy) * 0.5;
		float d = rvn_sd_round_box(p - (in.rect.xy + half_size), half_size, in.radii);
		coverage = clamp(0.5 - d, 0.0, 1.0);
		if (in.border_width > 0.0) {
			float inside = clamp(0.5 - (d + in.border_width), 0.0, 1.0);
			color = mix(in.border_color, color, inside);
		}
	} else if (kind == RVN_PRIM_SHADOW) {
		float2 half_size = (in.rect.zw - in.rect.xy) * 0.5;
		float d = rvn_sd_round_box(p - (in.rect.xy + half_size), half_size, in.radii);
		float blur = max(in.border_width, 0.5);
		coverage = 1.0 - smoothstep(-blur, blur, d);
	} else if (kind == RVN_PRIM_LINE) {
		float d = sd_segment(p, in.rect.xy, in.rect.zw) - in.radii.x * 0.5;
		coverage = clamp(0.5 - d, 0.0, 1.0);
	} else if (kind == RVN_PRIM_TRIANGLE) {
		float d = sd_triangle(p, in.rect.xy, in.rect.zw, in.radii.xy);
		coverage = clamp(0.5 - d, 0.0, 1.0);
	} else if (kind == RVN_PRIM_CURVE) {
		/* a cubic curve, followed as a chain of short segments: the nearest one gives the distance and how far along it is */
		float2 a = in.rect.xy, b = in.radii.xy, c = in.radii.zw, e = in.rect.zw;
		float2 from = a;
		float nearest = 1e20, along = 0.0;
		for (int i = 1; i <= 24; i++) {
			float t = float(i) / 24.0;
			float u = 1.0 - t;
			float2 to = u * u * u * a + 3.0 * u * u * t * b + 3.0 * u * t * t * c + t * t * t * e;
			float2 pa = p - from, ba = to - from;
			float h = clamp(dot(pa, ba) / max(dot(ba, ba), 1e-6), 0.0, 1.0);
			float2 away = pa - ba * h;
			float d2 = dot(away, away);
			if (d2 < nearest) {
				nearest = d2;
				along = (float(i - 1) + h) / 24.0;
			}
			from = to;
		}
		float d = sqrt(nearest) - in.border_width * 0.5;
		float stroke = clamp(0.5 - d, 0.0, 1.0);
		float pulse = 0.0;
		color = mix(in.color, in.border_color, along);
		float bright = float((in.flags >> 8) & 255u) / 255.0;
		if (in.params.w > 0.0 && bright > 0.0) {
			/* pulses of light that run from one end to the other */
			float f = fract(along * in.params.w - in.params.z) - 0.5;
			pulse = exp(-f * f * 80.0) * bright;
			color.rgb = mix(color.rgb, float3(color.a), pulse * 0.6);
		}
		float halo = 0.0;
		if (in.params.x > 0.0) {
			halo = 1.0 - smoothstep(0.0, in.params.x, max(d, 0.0));
			halo = halo * halo * in.params.y * (1.0 + pulse * 0.8);
		}
		coverage = stroke + (1.0 - stroke) * min(halo, 1.0);
	} else if (kind == RVN_PRIM_GLASS) {
		float2 half_size = (in.rect.zw - in.rect.xy) * 0.5;
		float2 normal;
		float d = sd_round_box_normal(p - (in.rect.xy + half_size), half_size, in.radii, normal);
		coverage = clamp(0.5 - d, 0.0, 1.0);
		color = glass(in, p, d, normal, u_backdrop, u_backdrop_blur);
	} else if (kind == RVN_PRIM_BLUR) {
		float2 half_size = (in.rect.zw - in.rect.xy) * 0.5;
		float2 normal;
		float d = sd_round_box_normal(p - (in.rect.xy + half_size), half_size, in.radii, normal);
		coverage = clamp(0.5 - d, 0.0, 1.0);
		color = blur(in, p, d, normal, clamp(min(half_size.x, half_size.y) * 0.12, 3.0, 12.0), u_backdrop, u_backdrop_blur);
		if (in.border_width > 0.0) {
			/* a hairline around it, over the blur */
			float inside = clamp(0.5 - (d + in.border_width), 0.0, 1.0);
			color = mix(in.border_color + color * (1.0 - in.border_color.a), color, inside);
		}
	}

	return color * coverage;
}

/*
 * What a backend without the shader of an R_DRAW_SHADER command may draw
 * (raven_backend.h): the rounded rectangle of the instance filled with its
 * color. Used for the effects of the application (SPIR-V, which Metal does
 * not read) and for a built-in effect whose library did not compile. Same
 * clip test and coverage as the RECT branch above, no outline, times the
 * opacity of bits 8..15 of the flags.
 */
fragment float4 rvn_shader_fallback_fragment(RvnVaryings in [[stage_in]]) {
	float2 p = in.position.xy;
	if (p.x < in.clip.x || p.y < in.clip.y || p.x >= in.clip.z || p.y >= in.clip.w) {
		discard_fragment();
		return float4(0.0);
	}
	float2 half_size = (in.rect.zw - in.rect.xy) * 0.5;
	float d = rvn_sd_round_box(p - (in.rect.xy + half_size), half_size, in.radii);
	float coverage = clamp(0.5 - d, 0.0, 1.0);
	float opacity = float((in.flags >> 8) & 255u) / 255.0;
	return in.color * (coverage * opacity);
}
