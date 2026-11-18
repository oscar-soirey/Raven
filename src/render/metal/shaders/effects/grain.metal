/* Grain: a gradient with the fine grain of film.
 * color -> color2 along the angle. params: grain (0..1), angle (degrees).
 * The pointer: a soft sheen follows it, as light on paper.
 * (MSL twin of src/render/vulkan/shaders/effects/grain.frag) */
float4 raven_effect(RAVEN_EFFECT_ARGS) {
	float grain = fx.params.x, angle = fx.params.y * (RAVEN_PI / 180.0);
	float2 direction = float2(cos(angle), sin(angle));
	float2 c = fx.pixel - fx.size * 0.5;
	float reach = abs(direction.x) * fx.size.x * 0.5 + abs(direction.y) * fx.size.y * 0.5;
	float t = clamp(dot(c, direction) / max(reach, 1.0) * 0.5 + 0.5, 0.0, 1.0);
	t = t * t * (3.0 - 2.0 * t);
	float4 color = mix(fx.color, fx.color2, t);
	/* a new grain 24 times a second, like film */
	float n = raven_hash(floor(fx.pixel) + floor(fx.time * 24.0) * float2(7.13, 3.71)) - 0.5;
	color.rgb += n * grain * 0.18;
	if (raven_has_pointer(fx)) {
		float2 d = (fx.pixel - fx.pointer) / max(0.45 * max(fx.size.x, fx.size.y), 1.0);
		color.rgb += (0.10 + 0.06 * fx.press) * fx.hover * exp(-dot(d, d) * 2.5);
	}
	return color;
}
