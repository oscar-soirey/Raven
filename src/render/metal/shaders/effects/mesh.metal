/* Mesh: four colors that blend across the surface and slowly move.
 * color, color2 and two colors made from them. params: speed.
 * The pointer: the colors are drawn toward it like a liquid, and a soft light follows it.
 * (MSL twin of src/render/vulkan/shaders/effects/mesh.frag) */
float4 raven_effect(RAVEN_EFFECT_ARGS) {
	float t = fx.time * 0.2 * fx.params.x;
	float3 a = fx.color.rgb;
	float3 b = fx.color2.rgb;
	float3 c = clamp(mix(a, b, 0.5).gbr * 1.1, 0.0, 1.0);
	float3 d = clamp(mix(a, b, 0.3) * 0.65 + 0.12, 0.0, 1.0);
	/* the surface is warped slowly so that the corners wander */
	float2 uv = fx.uv;
	uv += 0.12 * float2(sin(uv.y * 3.0 + t * 2.0), cos(uv.x * 3.0 - t * 1.7));
	uv += 0.06 * float2(raven_fbm(uv * 2.0 + t), raven_fbm(uv * 2.0 - t + 5.0)) - 0.03;
	/* around the pointer the surface is pulled in: the colors gather there */
	float glow = 0.0;
	if (raven_has_pointer(fx)) {
		float2 m = fx.pointer / max(fx.size, float2(1.0));
		float2 to = uv - m;
		float pull = exp(-dot(to, to) * 7.0) * fx.hover;
		uv -= to * pull * (0.45 + 0.25 * fx.press);
		glow = exp(-dot(fx.pixel - fx.pointer, fx.pixel - fx.pointer) / pow(0.35 * min(fx.size.x, fx.size.y), 2.0)) * fx.hover;
	}
	uv = clamp(uv, 0.0, 1.0);
	float2 s = uv * uv * (3.0 - 2.0 * uv);
	float3 color = mix(mix(a, c, s.x), mix(d, b, s.x), s.y);
	color = mix(color, min(color * 1.25 + 0.1, 1.0), glow * 0.6);
	color += (raven_hash(fx.pixel + t) - 0.5) / 255.0;
	return float4(color, mix(fx.color.a, fx.color2.a, uv.y));
}
