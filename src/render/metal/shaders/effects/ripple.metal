/* Ripple: rings run over what is behind, like water. Needs the backdrop.
 * color: tint (its alpha is its strength). params: speed, strength (pixels), density, center x (0..1).
 * The pointer: the rings start from it, and a press makes them stronger.
 * (MSL twin of src/render/vulkan/shaders/effects/ripple.frag) */
float4 raven_effect(RAVEN_EFFECT_ARGS) {
	float speed = fx.params.x, strength = fx.params.y, density = max(fx.params.z, 0.1);
	float2 center = float2(fx.params.w, 0.5) * fx.size;
	if (raven_has_pointer(fx)) {
		center = mix(center, fx.pointer, fx.hover);
		strength *= 1.0 + fx.press * 1.5;
	}
	float2 delta = fx.pixel - center;
	float dist = length(delta);
	float2 direction = dist > 0.0 ? delta / dist : float2(0.0);
	float scale = min(fx.size.x, fx.size.y);
	float phase = dist / scale * density * 6.0 - fx.time * speed * 4.0;
	/* the rings fade toward the edge, and come in trains */
	float fade = exp(-dist / scale * 1.2) * (0.6 + 0.4 * sin(phase * 0.25));
	float wave = sin(phase) * fade;
	float2 offset = direction * wave * strength;
	float3 seen = raven_backdrop(fx.screen + offset).rgb;
	/* the crests catch the light */
	float crest = max(cos(phase), 0.0) * fade;
	/* under the pointer the crests shine more, and the water glows a little */
	float lit = 1.0 + fx.hover * 2.0 + fx.press * 2.0;
	float3 color = mix(seen, fx.color.rgb, fx.color.a) + float3(crest * 0.08 * lit)
		+ fx.color.rgb * fx.hover * 0.14 * exp(-dist / scale * 3.0);
	return float4(color, 1.0);
}
