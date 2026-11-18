/* Neon: an edge of light whose colors run around the shape.
 * color, color2: the colors that chase each other. params: speed, width (pixels), glow (pixels).
 * The pointer: the edge next to it burns brighter and wider; a press flashes the whole edge.
 * (MSL twin of src/render/vulkan/shaders/effects/neon.frag) */
float4 raven_effect(RAVEN_EFFECT_ARGS) {
	float speed = fx.params.x, width = max(fx.params.y, 0.5), glow = max(fx.params.z, 0.5);
	float2 c = fx.pixel - fx.size * 0.5;
	/* the angle around the middle, as a turn: 0..1 */
	float turn = atan2(c.y * fx.size.x / max(fx.size.y, 1.0), c.x) / RAVEN_TAU + 0.5;
	float phase = fract(turn - fx.time * 0.25 * speed);
	float chase = 0.5 + 0.5 * cos(phase * RAVEN_TAU * 2.0);
	float3 tone = mix(fx.color.rgb, fx.color2.rgb, chase);
	/* the pointer heats the edge near it */
	float near = 0.0;
	if (raven_has_pointer(fx)) {
		float reach = 0.3 * min(fx.size.x, fx.size.y) + 20.0;
		near = fx.hover * exp(-length(fx.pixel - fx.pointer) / reach);
		width *= 1.0 + near * 1.2;
		glow *= 1.0 + near * 1.5 + fx.press * 0.8;
	}
	/* the line itself, then its glow toward the inside */
	float inside = -fx.edge;
	float line = 1.0 - smoothstep(width * 0.5, width * 0.5 + 1.0, abs(inside - width * 0.5));
	float halo = exp(-max(inside - width, 0.0) / glow) * 0.55;
	/* the brightest point of the chase is hotter */
	float hot = pow(chase, 6.0) * 0.6 + near * 0.7 + fx.press * 0.35;
	float3 color = tone * (line + halo) + float3(hot) * line;
	float alpha = clamp(line + halo * 0.9, 0.0, 1.0) * max(fx.color.a, fx.color2.a);
	return float4(color / max(alpha, 1e-3), alpha);
}
