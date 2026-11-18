/*
 * Raven - blur.metal
 * The passes that build the backdrop of the glass primitives:
 * src/render/vulkan/shaders/blur.vert and blur.frag in the Metal Shading
 * Language. Compiled after common.metal.
 *
 * A wide blur is built cheaply with the "dual filtering" scheme: the frame is
 * reduced by half a few times (mode 0), then enlarged back (mode 1). Every
 * pass reads a handful of bilinear samples, so the cost stays small whatever
 * the blur radius. Mode 2 is a plain copy.
 */

/* Full-screen triangle, no vertex buffer. Metal's clip space points up: the
 * first row of the source (uv.y = 0) is drawn at the top of the target. */
vertex RvnBlurVaryings rvn_blur_vertex(uint vid [[vertex_id]]) {
	RvnBlurVaryings out;
	float2 corner = float2(float((vid << 1) & 2u), float(vid & 2u));
	out.uv = corner;                                                       /* (0, 0) = top left of the source */
	out.position = float4(corner.x * 2.0 - 1.0, 1.0 - corner.y * 2.0, 0.0, 1.0);
	return out;
}

fragment float4 rvn_blur_fragment(RvnBlurVaryings in [[stage_in]],
	texture2d<float> source [[texture(0)]], constant RvnBlurUniforms& u [[buffer(0)]]) {
	float2 uv = in.uv;
	if (u.mode == 0) {
		/* reduce: the center and the four diagonals */
		float4 sum = source.sample(rvn_linear, uv, level(0.0)) * 4.0;
		sum += source.sample(rvn_linear, uv + float2(-u.step.x, -u.step.y), level(0.0));
		sum += source.sample(rvn_linear, uv + float2( u.step.x, -u.step.y), level(0.0));
		sum += source.sample(rvn_linear, uv + float2(-u.step.x,  u.step.y), level(0.0));
		sum += source.sample(rvn_linear, uv + float2( u.step.x,  u.step.y), level(0.0));
		return sum / 8.0;
	} else if (u.mode == 1) {
		/* enlarge: a ring of eight samples */
		float4 sum = source.sample(rvn_linear, uv + float2(-u.step.x * 2.0, 0.0), level(0.0));
		sum += source.sample(rvn_linear, uv + float2( u.step.x * 2.0, 0.0), level(0.0));
		sum += source.sample(rvn_linear, uv + float2(0.0, -u.step.y * 2.0), level(0.0));
		sum += source.sample(rvn_linear, uv + float2(0.0,  u.step.y * 2.0), level(0.0));
		sum += source.sample(rvn_linear, uv + float2(-u.step.x,  u.step.y), level(0.0)) * 2.0;
		sum += source.sample(rvn_linear, uv + float2( u.step.x,  u.step.y), level(0.0)) * 2.0;
		sum += source.sample(rvn_linear, uv + float2(-u.step.x, -u.step.y), level(0.0)) * 2.0;
		sum += source.sample(rvn_linear, uv + float2( u.step.x, -u.step.y), level(0.0)) * 2.0;
		return sum / 12.0;
	} else {
		return source.sample(rvn_linear, uv, level(0.0));
	}
}
