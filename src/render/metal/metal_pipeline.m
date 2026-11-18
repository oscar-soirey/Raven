/**
 * Raven - render/metal/metal_pipeline.m
 * Shader libraries, pipeline states, samplers and effects.
 *
 * The shaders are compiled from their MSL sources when the backend starts
 * (generated/shaders.h): the UI library (common + ui) and the blur library
 * (common + blur) at Init, the library of a built-in effect (common + effect
 * + effects/<name>) the first time CreateShader names it.
 *
 * Effects given as SPIR-V cannot run here: Metal has no SPIR-V front end.
 * The built-in effects are recognised by their words (rvn_effect_index_of)
 * and run from their MSL ports; the others are drawn as rounded rectangles in
 * their color, which the backend contract allows.
 */

#include "metal_backend.h"
#include "generated/shaders.h"

_Static_assert(RVN_MTL_EFFECT_COUNT == RVN_MTL_EFFECTS, "one MSL source per built-in effect");
_Static_assert(sizeof(RDrawInstance) == RVN_MTL_INSTANCE_SIZE, "RvnInstance of common.metal is 80 bytes");
_Static_assert(sizeof(rvn_mtl_uniforms_t) == 32, "RvnUniforms of common.metal is 32 bytes");
_Static_assert(sizeof(rvn_mtl_blur_uniforms_t) == 16, "RvnBlurUniforms of common.metal is 16 bytes");

/* ---------------------------------------------------------------------------
 *  Libraries
 * ------------------------------------------------------------------------- */

static MTLCompileOptions* rvn_mtl_compile_options(void) {
	MTLCompileOptions* options = [MTLCompileOptions new];
	/*
	 * Precise math: fast math would change the hashes built on
	 * fract(sin(x) * 43758.5453) (grain, stars, dithering) and the effects
	 * compared with Vulkan.
	 */
#if defined(__MAC_15_0) || defined(__IPHONE_18_0)
	if (@available(macOS 15.0, iOS 18.0, *)) {
		options.mathMode = MTLMathModeSafe;
	} else
#endif
	{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		options.fastMathEnabled = NO;
#pragma clang diagnostic pop
	}
	return options;
}

/* Compiles the concatenation of the sources. nil, with the message of the compiler in `message`, when it fails. */
static id<MTLLibrary> rvn_mtl_library(const char* const* sources, int count, NSString** message) {
	NSMutableString* text = [NSMutableString string];
	NSError* error = nil;
	id<MTLLibrary> library;
	int i;
	for (i = 0; i < count; i++) {
		NSString* part = [NSString stringWithUTF8String:sources[i]];
		if (part) [text appendString:part];
	}
	library = [g_mtl.device newLibraryWithSource:text options:rvn_mtl_compile_options() error:&error];
	if (!library && message) *message = error ? error.localizedDescription : @"unknown error";
	return library;
}

static id<MTLFunction> rvn_mtl_function(id<MTLLibrary> library, NSString* name) {
	id<MTLFunction> function = [library newFunctionWithName:name];
	if (!function) rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: the shaders have no function %s", name.UTF8String);
	return function;
}

/*
 * One pipeline state: one BGRA8Unorm color attachment, no depth, one sample.
 * blend: 0 = none, 1 = premultiplied "over" (ONE, ONE_MINUS_SOURCE_ALPHA),
 * 2 = erase (ZERO, ONE_MINUS_SOURCE_ALPHA).
 */
static id<MTLRenderPipelineState> rvn_mtl_pipeline(id<MTLFunction> vertex, id<MTLFunction> fragment, int blend,
	NSString* label, NSString** message) {
	MTLRenderPipelineDescriptor* descriptor;
	MTLRenderPipelineColorAttachmentDescriptor* color;
	NSError* error = nil;
	id<MTLRenderPipelineState> pipeline;

	if (!vertex || !fragment) return nil;
	descriptor = [MTLRenderPipelineDescriptor new];
	descriptor.label = label;
	descriptor.vertexFunction = vertex;
	descriptor.fragmentFunction = fragment;
	descriptor.rasterSampleCount = 1;
	color = descriptor.colorAttachments[0];
	color.pixelFormat = RVN_MTL_COLOR_FORMAT;
	color.writeMask = MTLColorWriteMaskAll;
	if (blend) {
		color.blendingEnabled = YES;
		color.rgbBlendOperation = MTLBlendOperationAdd;
		color.alphaBlendOperation = MTLBlendOperationAdd;
		color.sourceRGBBlendFactor = blend == 2 ? MTLBlendFactorZero : MTLBlendFactorOne;
		color.sourceAlphaBlendFactor = blend == 2 ? MTLBlendFactorZero : MTLBlendFactorOne;
		color.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
		color.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	}
	pipeline = [g_mtl.device newRenderPipelineStateWithDescriptor:descriptor error:&error];
	if (!pipeline) {
		NSString* text = error ? error.localizedDescription : @"unknown error";
		if (message) *message = text;
		else rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: cannot create the %s pipeline: %s", label.UTF8String, text.UTF8String);
	}
	return pipeline;
}

static id<MTLSamplerState> rvn_mtl_sampler(MTLSamplerMinMagFilter filter, MTLSamplerMipFilter mip) {
	MTLSamplerDescriptor* descriptor = [MTLSamplerDescriptor new];
	descriptor.minFilter = filter;
	descriptor.magFilter = filter;
	descriptor.mipFilter = mip;
	descriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
	descriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
	descriptor.normalizedCoordinates = YES;
	descriptor.maxAnisotropy = 1;
	return [g_mtl.device newSamplerStateWithDescriptor:descriptor];
}

/* ---------------------------------------------------------------------------
 *  Module
 * ------------------------------------------------------------------------- */

int rvn_mtl_pipeline_init(void) {
	const char* ui_sources[2] = { rvn_mtl_common_source, rvn_mtl_ui_source };
	const char* blur_sources[2] = { rvn_mtl_common_source, rvn_mtl_blur_source };
	NSString* message = nil;
	id<MTLFunction> ui_vertex, ui_fragment, fallback_fragment, blur_vertex, blur_fragment;

	g_mtl.shaders = [NSMutableDictionary dictionary];

	g_mtl.ui_library = rvn_mtl_library(ui_sources, 2, &message);
	if (!g_mtl.ui_library) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: the UI shaders cannot be compiled: %s", message.UTF8String);
		return R_FALSE;
	}
	g_mtl.blur_library = rvn_mtl_library(blur_sources, 2, &message);
	if (!g_mtl.blur_library) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: the blur shaders cannot be compiled: %s", message.UTF8String);
		return R_FALSE;
	}

	ui_vertex = rvn_mtl_function(g_mtl.ui_library, @"rvn_ui_vertex");
	ui_fragment = rvn_mtl_function(g_mtl.ui_library, @"rvn_ui_fragment");
	fallback_fragment = rvn_mtl_function(g_mtl.ui_library, @"rvn_shader_fallback_fragment");
	blur_vertex = rvn_mtl_function(g_mtl.blur_library, @"rvn_blur_vertex");
	blur_fragment = rvn_mtl_function(g_mtl.blur_library, @"rvn_blur_fragment");

	g_mtl.ui_pipeline = rvn_mtl_pipeline(ui_vertex, ui_fragment, 1, @"UI", NULL);
	/* R_DRAW_ERASE: the same shaders, only the blending differs */
	g_mtl.erase_pipeline = rvn_mtl_pipeline(ui_vertex, ui_fragment, 2, @"erase", NULL);
	g_mtl.fallback_pipeline = rvn_mtl_pipeline(ui_vertex, fallback_fragment, 1, @"effect fallback", NULL);
	g_mtl.blur_pipeline = rvn_mtl_pipeline(blur_vertex, blur_fragment, 0, @"blur", NULL);
	if (!g_mtl.ui_pipeline || !g_mtl.erase_pipeline || !g_mtl.fallback_pipeline || !g_mtl.blur_pipeline) return R_FALSE;

	/* the twins of the samplers of the Vulkan backend */
	g_mtl.sampler_nearest = rvn_mtl_sampler(MTLSamplerMinMagFilterNearest, MTLSamplerMipFilterNotMipmapped);
	g_mtl.sampler_linear = rvn_mtl_sampler(MTLSamplerMinMagFilterLinear, MTLSamplerMipFilterNotMipmapped);
	g_mtl.sampler_trilinear = rvn_mtl_sampler(MTLSamplerMinMagFilterLinear, MTLSamplerMipFilterLinear);
	if (!g_mtl.sampler_nearest || !g_mtl.sampler_linear || !g_mtl.sampler_trilinear) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: cannot create the samplers");
		return R_FALSE;
	}
	return R_TRUE;
}

void rvn_mtl_pipeline_shutdown(void) {
	int i;
	g_mtl.shaders = nil;
	for (i = 0; i < RVN_MTL_EFFECTS; i++) {
		g_mtl.effect_libraries[i] = nil;
		g_mtl.effect_pipelines[i] = nil;
		g_mtl.effect_failed[i] = R_FALSE;
	}
	g_mtl.ui_pipeline = nil;
	g_mtl.erase_pipeline = nil;
	g_mtl.fallback_pipeline = nil;
	g_mtl.blur_pipeline = nil;
	g_mtl.sampler_nearest = nil;
	g_mtl.sampler_linear = nil;
	g_mtl.sampler_trilinear = nil;
	g_mtl.ui_library = nil;
	g_mtl.blur_library = nil;
}

/* ---------------------------------------------------------------------------
 *  Effects
 * ------------------------------------------------------------------------- */

/* The pipeline of built-in effect `index`, compiled the first time (then kept until Shutdown). nil: drawn with the fallback. */
static id<MTLRenderPipelineState> rvn_mtl_effect_pipeline(int index) {
	const char* sources[3];
	NSString* message = nil;
	id<MTLFunction> vertex, fragment = nil;

	if (g_mtl.effect_pipelines[index] || g_mtl.effect_failed[index]) return g_mtl.effect_pipelines[index];
	sources[0] = rvn_mtl_common_source;
	sources[1] = rvn_mtl_effect_source;
	sources[2] = rvn_mtl_effect_sources[index];
	g_mtl.effect_libraries[index] = rvn_mtl_library(sources, 3, &message);
	if (g_mtl.effect_libraries[index]) {
		fragment = [g_mtl.effect_libraries[index] newFunctionWithName:@"rvn_effect_fragment"];
		if (!fragment) message = @"no function rvn_effect_fragment";
	}
	/* the vertex function of the UI: both libraries declare the same RvnVaryings */
	vertex = [g_mtl.ui_library newFunctionWithName:@"rvn_ui_vertex"];
	if (fragment) {
		g_mtl.effect_pipelines[index] = rvn_mtl_pipeline(vertex, fragment, 1,
			[NSString stringWithFormat:@"effect %s", Reffect_get_name((REffect)(R_EFFECT_AURORA + index))], &message);
	}
	if (!g_mtl.effect_pipelines[index]) {
		/* said once per effect: it is then drawn as a rounded rectangle */
		g_mtl.effect_failed[index] = R_TRUE;
		g_mtl.effect_libraries[index] = nil;
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: the effect %s cannot be compiled: %s",
			Reffect_get_name((REffect)(R_EFFECT_AURORA + index)), message ? message.UTF8String : "unknown error");
	}
	return g_mtl.effect_pipelines[index];
}

int rvn_mtl_shader_create(Rid id, const uint32_t* spirv, size_t size) {
	RvnMtlShader* shader;
	/* the device is lost: Raven gives every shader again once the backend is started again */
	if (!rvn_mtl_usable()) return R_TRUE;
	shader = [RvnMtlShader new];
	shader->id = id;
	shader->effect = rvn_effect_index_of(spirv, size);
	if (shader->effect >= 0 && shader->effect < RVN_MTL_EFFECTS) {
		shader->pipeline = rvn_mtl_effect_pipeline(shader->effect);
	} else if (!g_mtl.custom_shader_reported) {
		g_mtl.custom_shader_reported = R_TRUE;
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_WARNING,
			"Metal: effects given as SPIR-V cannot run on the Metal backend: they are drawn as rounded rectangles");
	}
	/* (usable either way: only its look is reduced) */
	g_mtl.shaders[@(R_ID_INDEX(id))] = shader;
	return R_TRUE;
}

void rvn_mtl_shader_destroy(Rid id) {
	RvnMtlShader* shader = g_mtl.shaders[@(R_ID_INDEX(id))];
	/* (command buffers in flight keep the pipeline alive themselves) */
	if (shader && shader->id == id) [g_mtl.shaders removeObjectForKey:@(R_ID_INDEX(id))];
}

id<MTLRenderPipelineState> rvn_mtl_shader_pipeline(Rid id) {
	RvnMtlShader* shader = g_mtl.shaders[@(R_ID_INDEX(id))];
	if (shader && shader->id == id && shader->pipeline) return shader->pipeline;
	return g_mtl.fallback_pipeline;
}

@implementation RvnMtlShader
@end
