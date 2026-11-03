/**
 * Raven - render/vulkan/vk_pipeline.c
 * Shaders, samplers, descriptor sets, render passes and pipelines.
 *
 * The whole UI is drawn with a single pipeline: an instanced triangle strip
 * of four vertices whose only vertex buffer is the array of RDrawInstance.
 * A texture is one descriptor set (combined image sampler, binding 0).
 */

#include "vk_backend.h"

#include "generated/ui_vert_spv.h"
#include "generated/ui_frag_spv.h"
#include "generated/blur_vert_spv.h"
#include "generated/blur_frag_spv.h"

/* ---------------------------------------------------------------------------
 *  Device objects shared by every pipeline
 * ------------------------------------------------------------------------- */

static int rvn_vk_shader_create_module(const uint32_t* code, size_t size, VkShaderModule* module) {
	VkShaderModuleCreateInfo info;
	memset(&info, 0, sizeof(info));
	info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.codeSize = size;
	info.pCode = code;
	return !rvn_vk_failed(rvn_vk.CreateShaderModule(g_vk.device, &info, NULL, module), "vkCreateShaderModule");
}

static int rvn_vk_sampler_create(VkFilter filter, int mipmaps, VkSampler* sampler) {
	VkSamplerCreateInfo info;
	memset(&info, 0, sizeof(info));
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.magFilter = filter;
	info.minFilter = filter;
	/* without mipmaps maxLod stays 0: only the first level is read */
	info.mipmapMode = mipmaps ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
	if (mipmaps) info.maxLod = VK_LOD_CLAMP_NONE;
	info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.maxAnisotropy = 1.0f;
	return !rvn_vk_failed(rvn_vk.CreateSampler(g_vk.device, &info, NULL, sampler), "vkCreateSampler");
}

static void rvn_vk_pass_destroy(rvn_vk_pass_t* pass);

int rvn_vk_pipeline_init(void) {
	VkDescriptorSetLayoutBinding binding;
	VkDescriptorSetLayoutBinding backdrop_bindings[2];
	VkDescriptorSetLayoutCreateInfo set_info;
	VkDescriptorSetLayout set_layouts[2];
	VkDescriptorPoolSize pool_size;
	VkDescriptorPoolCreateInfo pool_info;
	VkPushConstantRange push;
	VkPipelineLayoutCreateInfo layout_info;

	if (!rvn_vk_shader_create_module(rvn_ui_vert_spv, sizeof(rvn_ui_vert_spv), &g_vk.vertex_shader)) return R_FALSE;
	if (!rvn_vk_shader_create_module(rvn_ui_frag_spv, sizeof(rvn_ui_frag_spv), &g_vk.fragment_shader)) return R_FALSE;
	if (!rvn_vk_shader_create_module(rvn_blur_vert_spv, sizeof(rvn_blur_vert_spv), &g_vk.blur_vertex_shader)) return R_FALSE;
	if (!rvn_vk_shader_create_module(rvn_blur_frag_spv, sizeof(rvn_blur_frag_spv), &g_vk.blur_fragment_shader)) return R_FALSE;
	if (!rvn_vk_sampler_create(VK_FILTER_LINEAR, R_FALSE, &g_vk.sampler_linear)) return R_FALSE;
	if (!rvn_vk_sampler_create(VK_FILTER_NEAREST, R_FALSE, &g_vk.sampler_nearest)) return R_FALSE;
	if (!rvn_vk_sampler_create(VK_FILTER_LINEAR, R_TRUE, &g_vk.sampler_mipmap)) return R_FALSE;

	memset(&binding, 0, sizeof(binding));
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	memset(&set_info, 0, sizeof(set_info));
	set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set_info.bindingCount = 1;
	set_info.pBindings = &binding;
	if (rvn_vk_failed(rvn_vk.CreateDescriptorSetLayout(g_vk.device, &set_info, NULL, &g_vk.set_layout),
		"vkCreateDescriptorSetLayout")) return R_FALSE;

	/* set 1 of the UI pipeline: the backdrop of glass, sharp and blurred */
	memset(backdrop_bindings, 0, sizeof(backdrop_bindings));
	backdrop_bindings[0] = binding;
	backdrop_bindings[1] = binding;
	backdrop_bindings[1].binding = 1;
	set_info.bindingCount = 2;
	set_info.pBindings = backdrop_bindings;
	if (rvn_vk_failed(rvn_vk.CreateDescriptorSetLayout(g_vk.device, &set_info, NULL, &g_vk.backdrop_layout),
		"vkCreateDescriptorSetLayout")) return R_FALSE;

	pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_size.descriptorCount = RVN_VK_MAX_BACKDROPS * 2;
	memset(&pool_info, 0, sizeof(pool_info));
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = RVN_VK_MAX_BACKDROPS;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes = &pool_size;
	if (rvn_vk_failed(rvn_vk.CreateDescriptorPool(g_vk.device, &pool_info, NULL, &g_vk.backdrop_pool),
		"vkCreateDescriptorPool")) return R_FALSE;

	/* UI: vec2 scale, vec2 translate, vec2 pointer (for the effects), vec2 unused */
	push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push.offset = 0;
	push.size = sizeof(float) * 8;
	set_layouts[0] = g_vk.set_layout;
	set_layouts[1] = g_vk.backdrop_layout;
	memset(&layout_info, 0, sizeof(layout_info));
	layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.setLayoutCount = 2;
	layout_info.pSetLayouts = set_layouts;
	layout_info.pushConstantRangeCount = 1;
	layout_info.pPushConstantRanges = &push;
	if (rvn_vk_failed(rvn_vk.CreatePipelineLayout(g_vk.device, &layout_info, NULL, &g_vk.pipeline_layout),
		"vkCreatePipelineLayout")) return R_FALSE;

	/* blur: one source texture; vec2 step, int mode */
	push.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	push.size = sizeof(float) * 2 + sizeof(int32_t);
	layout_info.setLayoutCount = 1;
	if (rvn_vk_failed(rvn_vk.CreatePipelineLayout(g_vk.device, &layout_info, NULL, &g_vk.blur_layout),
		"vkCreatePipelineLayout")) return R_FALSE;
	return R_TRUE;
}

void rvn_vk_pipeline_shutdown(void) {
	uint32_t i;
	for (i = 0; i < g_vk.shader_cap; i++) {
		if (g_vk.shaders[i].used) rvn_vk_shader_destroy(g_vk.shaders[i].id);
	}
	rvn_free(g_vk.shaders);
	g_vk.shaders = NULL;
	g_vk.shader_cap = 0;
	for (i = 0; i < g_vk.pass_count; i++) rvn_vk_pass_destroy(&g_vk.passes[i]);
	g_vk.pass_count = 0;
	if (g_vk.backdrop_pool) rvn_vk.DestroyDescriptorPool(g_vk.device, g_vk.backdrop_pool, NULL);
	if (g_vk.backdrop_layout) rvn_vk.DestroyDescriptorSetLayout(g_vk.device, g_vk.backdrop_layout, NULL);
	if (g_vk.blur_layout) rvn_vk.DestroyPipelineLayout(g_vk.device, g_vk.blur_layout, NULL);
	if (g_vk.blur_vertex_shader) rvn_vk.DestroyShaderModule(g_vk.device, g_vk.blur_vertex_shader, NULL);
	if (g_vk.blur_fragment_shader) rvn_vk.DestroyShaderModule(g_vk.device, g_vk.blur_fragment_shader, NULL);
	g_vk.backdrop_pool = VK_NULL_HANDLE;
	g_vk.backdrop_layout = VK_NULL_HANDLE;
	g_vk.backdrop_default = VK_NULL_HANDLE;
	g_vk.blur_layout = VK_NULL_HANDLE;
	g_vk.blur_vertex_shader = g_vk.blur_fragment_shader = VK_NULL_HANDLE;
	for (i = 0; i < g_vk.pool_count; i++) rvn_vk.DestroyDescriptorPool(g_vk.device, g_vk.pools[i], NULL);
	rvn_free(g_vk.pools);
	rvn_free(g_vk.pool_used);
	g_vk.pools = NULL;
	g_vk.pool_used = NULL;
	g_vk.pool_count = g_vk.pool_cap = 0;
	if (g_vk.pipeline_layout) rvn_vk.DestroyPipelineLayout(g_vk.device, g_vk.pipeline_layout, NULL);
	if (g_vk.set_layout) rvn_vk.DestroyDescriptorSetLayout(g_vk.device, g_vk.set_layout, NULL);
	if (g_vk.sampler_linear) rvn_vk.DestroySampler(g_vk.device, g_vk.sampler_linear, NULL);
	if (g_vk.sampler_nearest) rvn_vk.DestroySampler(g_vk.device, g_vk.sampler_nearest, NULL);
	if (g_vk.sampler_mipmap) rvn_vk.DestroySampler(g_vk.device, g_vk.sampler_mipmap, NULL);
	if (g_vk.vertex_shader) rvn_vk.DestroyShaderModule(g_vk.device, g_vk.vertex_shader, NULL);
	if (g_vk.fragment_shader) rvn_vk.DestroyShaderModule(g_vk.device, g_vk.fragment_shader, NULL);
	g_vk.pipeline_layout = VK_NULL_HANDLE;
	g_vk.set_layout = VK_NULL_HANDLE;
	g_vk.sampler_linear = g_vk.sampler_nearest = g_vk.sampler_mipmap = VK_NULL_HANDLE;
	g_vk.vertex_shader = g_vk.fragment_shader = VK_NULL_HANDLE;
}

/* ---------------------------------------------------------------------------
 *  Render pass + pipeline
 * ------------------------------------------------------------------------- */

/*
 * One color attachment, no depth. `present` = the swapchain: the image goes to
 * the screen afterwards. Otherwise the image is sampled afterwards (offscreen
 * frame of a window with glass, blur targets).
 */
static int rvn_vk_render_pass_create(VkFormat format, VkAttachmentLoadOp load, int present, VkRenderPass* render_pass) {
	VkAttachmentDescription attachment;
	VkAttachmentReference reference;
	VkSubpassDescription subpass;
	VkSubpassDependency dependencies[2];
	VkRenderPassCreateInfo info;

	memset(&attachment, 0, sizeof(attachment));
	attachment.format = format;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = load;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	/* continuing a frame keeps its content; anything else starts from scratch */
	attachment.initialLayout = load == VK_ATTACHMENT_LOAD_OP_LOAD ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = present ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	reference.attachment = 0;
	reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	memset(&subpass, 0, sizeof(subpass));
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &reference;

	memset(dependencies, 0, sizeof(dependencies));
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	if (present) {
		/* the image is only ours once the semaphore of vkAcquireNextImageKHR is signaled */
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = 0;
	} else {
		/* wait for whatever still writes or samples the image, and make the next reader wait for us */
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}

	memset(&info, 0, sizeof(info));
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = present ? 1 : 2;
	info.pDependencies = dependencies;
	return !rvn_vk_failed(rvn_vk.CreateRenderPass(g_vk.device, &info, NULL, render_pass), "vkCreateRenderPass");
}

static int rvn_vk_pipeline_create(VkRenderPass render_pass, int erase, VkShaderModule fragment, VkPipeline* pipeline) {
	VkPipelineShaderStageCreateInfo stages[2];
	VkVertexInputBindingDescription binding;
	VkVertexInputAttributeDescription attributes[8];
	VkPipelineVertexInputStateCreateInfo vertex_input;
	VkPipelineInputAssemblyStateCreateInfo assembly;
	VkPipelineViewportStateCreateInfo viewport;
	VkPipelineRasterizationStateCreateInfo raster;
	VkPipelineMultisampleStateCreateInfo multisample;
	VkPipelineColorBlendAttachmentState blend_attachment;
	VkPipelineColorBlendStateCreateInfo blend;
	VkPipelineDynamicStateCreateInfo dynamic;
	VkGraphicsPipelineCreateInfo info;
	static const VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	/* must match RDrawInstance and the inputs of ui.vert */
	static const struct { VkFormat format; uint32_t offset; } layout[8] = {
		{ VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(RDrawInstance, rect) },
		{ VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(RDrawInstance, uv) },
		{ VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(RDrawInstance, clip) },
		{ VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(RDrawInstance, radii) },
		{ VK_FORMAT_R8G8B8A8_UNORM,      offsetof(RDrawInstance, color) },
		{ VK_FORMAT_R8G8B8A8_UNORM,      offsetof(RDrawInstance, border_color) },
		{ VK_FORMAT_R32_SFLOAT,          offsetof(RDrawInstance, border_width) },
		{ VK_FORMAT_R32_UINT,            offsetof(RDrawInstance, flags) }
	};
	uint32_t i;

	memset(stages, 0, sizeof(stages));
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = g_vk.vertex_shader;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = fragment;
	stages[1].pName = "main";

	binding.binding = 0;
	binding.stride = sizeof(RDrawInstance);
	binding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
	for (i = 0; i < 8; i++) {
		attributes[i].location = i;
		attributes[i].binding = 0;
		attributes[i].format = layout[i].format;
		attributes[i].offset = layout[i].offset;
	}
	memset(&vertex_input, 0, sizeof(vertex_input));
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input.vertexBindingDescriptionCount = 1;
	vertex_input.pVertexBindingDescriptions = &binding;
	vertex_input.vertexAttributeDescriptionCount = 8;
	vertex_input.pVertexAttributeDescriptions = attributes;

	memset(&assembly, 0, sizeof(assembly));
	assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	memset(&viewport, 0, sizeof(viewport));
	viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport.viewportCount = 1;
	viewport.scissorCount = 1;

	memset(&raster, 0, sizeof(raster));
	raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = VK_CULL_MODE_NONE;
	raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
	raster.lineWidth = 1.0f;

	memset(&multisample, 0, sizeof(multisample));
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	/* the fragment shader outputs premultiplied colors */
	memset(&blend_attachment, 0, sizeof(blend_attachment));
	blend_attachment.blendEnable = VK_TRUE;
	blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	if (erase) {
		/* nothing is added: what is there is kept by as much as the instance does not cover it */
		blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	}
	memset(&blend, 0, sizeof(blend));
	blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend.attachmentCount = 1;
	blend.pAttachments = &blend_attachment;

	memset(&dynamic, 0, sizeof(dynamic));
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic.dynamicStateCount = (uint32_t)RVN_COUNT_OF(dynamic_states);
	dynamic.pDynamicStates = dynamic_states;

	memset(&info, 0, sizeof(info));
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.stageCount = 2;
	info.pStages = stages;
	info.pVertexInputState = &vertex_input;
	info.pInputAssemblyState = &assembly;
	info.pViewportState = &viewport;
	info.pRasterizationState = &raster;
	info.pMultisampleState = &multisample;
	info.pColorBlendState = &blend;
	info.pDynamicState = &dynamic;
	info.layout = g_vk.pipeline_layout;
	info.renderPass = render_pass;
	info.subpass = 0;
	return !rvn_vk_failed(rvn_vk.CreateGraphicsPipelines(g_vk.device, VK_NULL_HANDLE, 1, &info, NULL, pipeline),
		"vkCreateGraphicsPipelines");
}

/* Full-screen filter of the backdrop passes: three vertices made by blur.vert, no blending. */
static int rvn_vk_blur_pipeline_create(VkRenderPass render_pass, VkPipeline* pipeline) {
	VkPipelineShaderStageCreateInfo stages[2];
	VkPipelineVertexInputStateCreateInfo vertex_input;
	VkPipelineInputAssemblyStateCreateInfo assembly;
	VkPipelineViewportStateCreateInfo viewport;
	VkPipelineRasterizationStateCreateInfo raster;
	VkPipelineMultisampleStateCreateInfo multisample;
	VkPipelineColorBlendAttachmentState blend_attachment;
	VkPipelineColorBlendStateCreateInfo blend;
	VkPipelineDynamicStateCreateInfo dynamic;
	VkGraphicsPipelineCreateInfo info;
	static const VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	memset(stages, 0, sizeof(stages));
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = g_vk.blur_vertex_shader;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = g_vk.blur_fragment_shader;
	stages[1].pName = "main";

	memset(&vertex_input, 0, sizeof(vertex_input));
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	memset(&assembly, 0, sizeof(assembly));
	assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	memset(&viewport, 0, sizeof(viewport));
	viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport.viewportCount = 1;
	viewport.scissorCount = 1;
	memset(&raster, 0, sizeof(raster));
	raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = VK_CULL_MODE_NONE;
	raster.lineWidth = 1.0f;
	memset(&multisample, 0, sizeof(multisample));
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	memset(&blend_attachment, 0, sizeof(blend_attachment));
	blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	memset(&blend, 0, sizeof(blend));
	blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend.attachmentCount = 1;
	blend.pAttachments = &blend_attachment;
	memset(&dynamic, 0, sizeof(dynamic));
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic.dynamicStateCount = (uint32_t)RVN_COUNT_OF(dynamic_states);
	dynamic.pDynamicStates = dynamic_states;

	memset(&info, 0, sizeof(info));
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.stageCount = 2;
	info.pStages = stages;
	info.pVertexInputState = &vertex_input;
	info.pInputAssemblyState = &assembly;
	info.pViewportState = &viewport;
	info.pRasterizationState = &raster;
	info.pMultisampleState = &multisample;
	info.pColorBlendState = &blend;
	info.pDynamicState = &dynamic;
	info.layout = g_vk.blur_layout;
	info.renderPass = render_pass;
	return !rvn_vk_failed(rvn_vk.CreateGraphicsPipelines(g_vk.device, VK_NULL_HANDLE, 1, &info, NULL, pipeline),
		"vkCreateGraphicsPipelines");
}

static void rvn_vk_pass_destroy(rvn_vk_pass_t* pass) {
	if (pass->pipeline) rvn_vk.DestroyPipeline(g_vk.device, pass->pipeline, NULL);
	if (pass->erase_pipeline) rvn_vk.DestroyPipeline(g_vk.device, pass->erase_pipeline, NULL);
	if (pass->blur_pipeline) rvn_vk.DestroyPipeline(g_vk.device, pass->blur_pipeline, NULL);
	if (pass->render_pass) rvn_vk.DestroyRenderPass(g_vk.device, pass->render_pass, NULL);
	if (pass->scene_clear_pass) rvn_vk.DestroyRenderPass(g_vk.device, pass->scene_clear_pass, NULL);
	if (pass->scene_load_pass) rvn_vk.DestroyRenderPass(g_vk.device, pass->scene_load_pass, NULL);
	if (pass->blur_pass) rvn_vk.DestroyRenderPass(g_vk.device, pass->blur_pass, NULL);
	if (pass->blur_load_pass) rvn_vk.DestroyRenderPass(g_vk.device, pass->blur_load_pass, NULL);
	memset(pass, 0, sizeof(*pass));
}

rvn_vk_pass_t* rvn_vk_pass_get(VkFormat format) {
	rvn_vk_pass_t* pass;
	uint32_t i;
	for (i = 0; i < g_vk.pass_count; i++) {
		if (g_vk.passes[i].format == format) return &g_vk.passes[i];
	}
	if (g_vk.pass_count == RVN_VK_MAX_PASSES) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Vulkan: too many different swapchain formats");
		return NULL;
	}
	pass = &g_vk.passes[g_vk.pass_count];
	memset(pass, 0, sizeof(*pass));
	pass->format = format;
	if (!rvn_vk_render_pass_create(format, VK_ATTACHMENT_LOAD_OP_CLEAR, R_TRUE, &pass->render_pass) ||
		!rvn_vk_render_pass_create(format, VK_ATTACHMENT_LOAD_OP_CLEAR, R_FALSE, &pass->scene_clear_pass) ||
		!rvn_vk_render_pass_create(format, VK_ATTACHMENT_LOAD_OP_LOAD, R_FALSE, &pass->scene_load_pass) ||
		!rvn_vk_render_pass_create(format, VK_ATTACHMENT_LOAD_OP_DONT_CARE, R_FALSE, &pass->blur_pass) ||
		!rvn_vk_render_pass_create(format, VK_ATTACHMENT_LOAD_OP_LOAD, R_FALSE, &pass->blur_load_pass) ||
		!rvn_vk_pipeline_create(pass->render_pass, R_FALSE, g_vk.fragment_shader, &pass->pipeline) ||
		!rvn_vk_pipeline_create(pass->render_pass, R_TRUE, g_vk.fragment_shader, &pass->erase_pipeline) ||
		!rvn_vk_blur_pipeline_create(pass->blur_pass, &pass->blur_pipeline)) {
		rvn_vk_pass_destroy(pass);
		return NULL;
	}
	g_vk.pass_count++;
	return pass;
}

/* ---------------------------------------------------------------------------
 *  Effects: one fragment shader of the application each, the rest of the
 *  pipeline as the UI (same vertex shader, layout, blending)
 * ------------------------------------------------------------------------- */

static rvn_vk_shader_t* rvn_vk_shader_find(Rid id) {
	uint32_t index = R_ID_INDEX(id);
	if (index >= g_vk.shader_cap || !g_vk.shaders[index].used || g_vk.shaders[index].id != id) return NULL;
	return &g_vk.shaders[index];
}

int rvn_vk_shader_create(Rid id, const uint32_t* spirv, size_t size) {
	uint32_t index = R_ID_INDEX(id);
	rvn_vk_shader_t* shader;
	/* the device is lost: Raven gives every shader again once the backend is started again */
	if (!rvn_vk_usable()) return R_TRUE;
	if (index >= g_vk.shader_cap) {
		uint32_t cap = g_vk.shader_cap ? g_vk.shader_cap : 16;
		while (cap <= index) cap *= 2;
		g_vk.shaders = (rvn_vk_shader_t*)rvn_realloc(g_vk.shaders, sizeof(rvn_vk_shader_t) * cap);
		memset(g_vk.shaders + g_vk.shader_cap, 0, sizeof(rvn_vk_shader_t) * (cap - g_vk.shader_cap));
		g_vk.shader_cap = cap;
	}
	shader = &g_vk.shaders[index];
	if (shader->used) rvn_vk_shader_destroy(shader->id);
	memset(shader, 0, sizeof(*shader));
	if (!rvn_vk_shader_create_module(spirv, size, &shader->module)) return R_FALSE;
	shader->used = R_TRUE;
	shader->id = id;
	return R_TRUE;
}

void rvn_vk_shader_destroy(Rid id) {
	rvn_vk_shader_t* shader = g_vk.initialized ? rvn_vk_shader_find(id) : NULL;
	uint32_t i;
	if (!shader) return;
	/* (frames on the GPU may still draw with it) */
	rvn_vk.DeviceWaitIdle(g_vk.device);
	for (i = 0; i < RVN_VK_MAX_PASSES; i++) {
		if (shader->pipelines[i]) rvn_vk.DestroyPipeline(g_vk.device, shader->pipelines[i], NULL);
	}
	if (shader->module) rvn_vk.DestroyShaderModule(g_vk.device, shader->module, NULL);
	memset(shader, 0, sizeof(*shader));
}

VkPipeline rvn_vk_shader_pipeline(Rid id, rvn_vk_pass_t* pass) {
	rvn_vk_shader_t* shader = rvn_vk_shader_find(id);
	uint32_t slot;
	if (!shader || !pass) return VK_NULL_HANDLE;
	slot = (uint32_t)(pass - g_vk.passes);
	if (slot >= RVN_VK_MAX_PASSES) return VK_NULL_HANDLE;
	if (!shader->pipelines[slot] && !shader->failed[slot]) {
		if (!rvn_vk_pipeline_create(pass->render_pass, R_FALSE, shader->module, &shader->pipelines[slot])) {
			shader->failed[slot] = R_TRUE;
			rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Vulkan: the effect shader cannot be used (is it written with raven_effect.glsl?)");
		}
	}
	return shader->pipelines[slot];
}

/* ---------------------------------------------------------------------------
 *  Descriptor sets
 * ------------------------------------------------------------------------- */

int rvn_vk_set_allocate(VkDescriptorSet* set, VkDescriptorPool* pool) {
	VkDescriptorSetAllocateInfo allocation;
	uint32_t i;

	memset(&allocation, 0, sizeof(allocation));
	allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocation.descriptorSetCount = 1;
	allocation.pSetLayouts = &g_vk.set_layout;

	for (i = 0; i < g_vk.pool_count; i++) {
		if (g_vk.pool_used[i] < RVN_VK_SETS_PER_POOL) break;
	}
	if (i == g_vk.pool_count) {
		/* every pool is full: add one */
		VkDescriptorPoolSize size;
		VkDescriptorPoolCreateInfo info;
		VkDescriptorPool created;
		size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		size.descriptorCount = RVN_VK_SETS_PER_POOL;
		memset(&info, 0, sizeof(info));
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		info.maxSets = RVN_VK_SETS_PER_POOL;
		info.poolSizeCount = 1;
		info.pPoolSizes = &size;
		if (rvn_vk_failed(rvn_vk.CreateDescriptorPool(g_vk.device, &info, NULL, &created), "vkCreateDescriptorPool")) return R_FALSE;
		if (g_vk.pool_count == g_vk.pool_cap) {
			g_vk.pool_cap = g_vk.pool_cap ? g_vk.pool_cap * 2 : 4;
			g_vk.pools = (VkDescriptorPool*)rvn_realloc(g_vk.pools, sizeof(VkDescriptorPool) * g_vk.pool_cap);
			g_vk.pool_used = (uint32_t*)rvn_realloc(g_vk.pool_used, sizeof(uint32_t) * g_vk.pool_cap);
		}
		g_vk.pools[g_vk.pool_count] = created;
		g_vk.pool_used[g_vk.pool_count] = 0;
		g_vk.pool_count++;
	}

	allocation.descriptorPool = g_vk.pools[i];
	if (rvn_vk_failed(rvn_vk.AllocateDescriptorSets(g_vk.device, &allocation, set), "vkAllocateDescriptorSets")) return R_FALSE;
	g_vk.pool_used[i]++;
	*pool = g_vk.pools[i];
	return R_TRUE;
}

void rvn_vk_set_free(VkDescriptorSet set, VkDescriptorPool pool) {
	uint32_t i;
	if (!set) return;
	rvn_vk.FreeDescriptorSets(g_vk.device, pool, 1, &set);
	for (i = 0; i < g_vk.pool_count; i++) {
		if (g_vk.pools[i] == pool && g_vk.pool_used[i] > 0) g_vk.pool_used[i]--;
	}
}

void rvn_vk_set_write(VkDescriptorSet set, VkImageView view, VkSampler sampler) {
	VkDescriptorImageInfo image;
	VkWriteDescriptorSet write;
	image.sampler = sampler;
	image.imageView = view;
	image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	memset(&write, 0, sizeof(write));
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = set;
	write.dstBinding = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.pImageInfo = &image;
	rvn_vk.UpdateDescriptorSets(g_vk.device, 1, &write, 0, NULL);
}

int rvn_vk_backdrop_set_create(VkDescriptorSet* set, VkImageView sharp, VkImageView blurred) {
	VkDescriptorSetAllocateInfo allocation;
	VkDescriptorImageInfo images[2];
	VkWriteDescriptorSet write;

	memset(&allocation, 0, sizeof(allocation));
	allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocation.descriptorPool = g_vk.backdrop_pool;
	allocation.descriptorSetCount = 1;
	allocation.pSetLayouts = &g_vk.backdrop_layout;
	/* too many windows with glass: the caller draws without a backdrop, nothing to report */
	if (rvn_vk.AllocateDescriptorSets(g_vk.device, &allocation, set) < 0) {
		*set = VK_NULL_HANDLE;
		return R_FALSE;
	}

	images[0].sampler = g_vk.sampler_linear;
	images[0].imageView = sharp;
	images[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	images[1] = images[0];
	images[1].imageView = blurred;
	memset(&write, 0, sizeof(write));
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = *set;
	write.dstBinding = 0;
	write.descriptorCount = 2;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.pImageInfo = images;
	rvn_vk.UpdateDescriptorSets(g_vk.device, 1, &write, 0, NULL);
	return R_TRUE;
}

void rvn_vk_backdrop_set_destroy(VkDescriptorSet set) {
	if (set) rvn_vk.FreeDescriptorSets(g_vk.device, g_vk.backdrop_pool, 1, &set);
}
