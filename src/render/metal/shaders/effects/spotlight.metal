/* Spotlight: a soft light on a surface.
 * color: the light; color2: the surface. params: x, y (0..1), radius (0..1 of the larger side).
 * The pointer: the light goes where it is, and narrows and brightens when it presses.
 * (MSL twin of src/render/vulkan/shaders/effects/spotlight.frag) */
float4 raven_effect(RAVEN_EFFECT_ARGS) {
	float2 at = float2(fx.params.x, fx.params.y) * fx.size;
	float radius = max(fx.params.z, 0.01) * max(fx.size.x, fx.size.y);
	float boost = 1.0;
	if (raven_has_pointer(fx)) {
		at = mix(at, fx.pointer, fx.hover);
		radius *= mix(1.0, 0.55, fx.hover) * (1.0 - 0.2 * fx.press);
		boost = 1.0 + 0.5 * fx.hover + 0.5 * fx.press;
	}
	float d = length(fx.pixel - at) / radius;
	float light = exp(-d * d * 2.2);
	float3 color = fx.color2.rgb + fx.color.rgb * light * fx.color.a * boost;
	color += (raven_hash(fx.pixel) - 0.5) / 255.0;
	return float4(color, fx.color2.a);
}
