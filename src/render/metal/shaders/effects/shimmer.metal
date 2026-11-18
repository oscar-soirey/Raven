/* Shimmer: a band of light sweeping across, as on content that is loading.
 * color: the surface; color2: the light (its alpha is its strength). params: speed, width (0..1), angle (degrees).
 * The pointer: the light also gathers under it.
 * (MSL twin of src/render/vulkan/shaders/effects/shimmer.frag) */
float4 raven_effect(RAVEN_EFFECT_ARGS) {
	float speed = fx.params.x, width = clamp(fx.params.y, 0.02, 1.0), angle = fx.params.z * (RAVEN_PI / 180.0);
	float2 direction = float2(cos(angle), sin(angle));
	/* where the pixel is along the sweep, 0..1 across the whole rectangle whatever the angle */
	float2 c = fx.pixel - fx.size * 0.5;
	float reach = abs(direction.x) * fx.size.x * 0.5 + abs(direction.y) * fx.size.y * 0.5;
	float along = dot(c, direction) / max(reach, 1.0) * 0.5 + 0.5;
	/* the band goes from just before the start to just after the end, then waits a little */
	float cycle = fract(fx.time * 0.45 * speed);
	float position = mix(-width, 1.0 + width, clamp(cycle * 1.25, 0.0, 1.0));
	float x = (along - position) / width;
	float band = exp(-x * x * 4.0);
	if (raven_has_pointer(fx)) {
		float2 d = (fx.pixel - fx.pointer) / max(width * max(fx.size.x, fx.size.y), 1.0);
		band = max(band, fx.hover * (0.7 + 0.3 * fx.press) * exp(-dot(d, d) * 2.0));
	}
	return float4(mix(fx.color.rgb, fx.color2.rgb, band * fx.color2.a), fx.color.a);
}
