#include "utils/pipeline.hpp"

#include <iostream>
#include <cassert>

namespace naku {
GraphicsPipeline::GraphicsPipeline(
	Device& device,
	const PipelineConfig& config)
	: _device{ device } {
	createGraphicsPipeline(config);
}
GraphicsPipeline::~GraphicsPipeline() {
	vkDestroyPipeline(device(), _pipeline, nullptr);
}
void GraphicsPipeline::createGraphicsPipeline(
	const PipelineConfig& config) {
	assert(config.pipelineLayout  && "Cannot create graphics pipeline: no pipelineLayout provided in config.");
	assert(config.renderPass  && "Cannot create graphics pipeline: no renderPass provided in config.");

	auto& bindingDescriptions = config.bindingDescriptions;
	auto& attributeDescriptions = config.attributeDescriptions;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	VkPipelineShaderStageCreateInfo stages[2] = { config.vertStageCreateInfo, config.fragStageCreateInfo };
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = stages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &config.inputAssemblyInfo;
	pipelineInfo.pViewportState = &config.viewportInfo;
	pipelineInfo.pRasterizationState = &config.rasterizationInfo;
	pipelineInfo.pMultisampleState = &config.multisampleInfo;

	pipelineInfo.pColorBlendState = &config.colorBlendInfo;
	pipelineInfo.pDepthStencilState = &config.depthStencilInfo;
	pipelineInfo.pDynamicState = &config.dynamicStateInfo;

	pipelineInfo.layout = config.pipelineLayout;
	pipelineInfo.renderPass = config.renderPass;
	pipelineInfo.subpass = config.subpass;

	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(
			_device.device(),
			VK_NULL_HANDLE,
			1,
			&pipelineInfo,
			nullptr,
			&_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline.");
	}
}

void GraphicsPipeline::defaultPipelineConfig(PipelineConfig& config) {
	// we don't have defualt configurations for pipelineLayout  && renderline pass;

	// input assembly
	config.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	config.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	config.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	config.inputAssemblyInfo.pNext = nullptr;
	config.inputAssemblyInfo.flags = 0;

	config.bindingDescriptions = Vertex::getBindingDescriptions();
	config.attributeDescriptions = Vertex::getAttributeDescriptions();

	// rasterization
	config.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	// if true, discard frags whose z is less than 0 or larger than 1
	config.rasterizationInfo.depthClampEnable = VK_FALSE;
	// if true, discard primitives immediately before the rasterization stage.
	config.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	// can be VK_POLYGON_MODE_FILL, VK_POLYGON_MODE_LINE, or VK_POLYGON_MODE_POINT
	config.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	config.rasterizationInfo.lineWidth = 1.0f; 

	config.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	config.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	config.rasterizationInfo.depthBiasEnable = VK_FALSE;
	config.rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
	config.rasterizationInfo.depthBiasClamp = 0.0f;           // Optional
	config.rasterizationInfo.depthBiasSlopeFactor = 0.0f;     // Optional

	// viewport
	//config.viewport.x = 0;
	//config.viewport.y = 0;
	//config.viewport.width = static_cast<float>(width);
	//config.viewport.height = static_cast<float>(height);
	//config.viewport.minDepth = 0.0f;
	//config.viewport.maxDepth = 1.0f;
	//config.scissors.offset = { 0, 0 };
	//config.scissors.extent = { width, height };
	config.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	config.viewportInfo.viewportCount = 1;
	config.viewportInfo.pViewports = nullptr;
	config.viewportInfo.scissorCount = 1;
	config.viewportInfo.pScissors = nullptr;

	// dynamic states
	config.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	config.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	config.dynamicStateInfo.pDynamicStates = config.dynamicStateEnables.data();
	config.dynamicStateInfo.dynamicStateCount =
		static_cast<uint32_t>(config.dynamicStateEnables.size());
	config.dynamicStateInfo.flags = 0;
	config.dynamicStateInfo.pNext = nullptr;

	// MSAA and more
	config.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	config.multisampleInfo.sampleShadingEnable = VK_FALSE;
	config.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	config.multisampleInfo.minSampleShading = 1.0f;           // Optional
	config.multisampleInfo.pSampleMask = nullptr;             // Optional
	config.multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
	config.multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional

	// global color bending
	config.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	config.colorBlendInfo.logicOpEnable = VK_FALSE;
	config.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
	config.colorBlendInfo.attachmentCount = 1;
	// color bending for a framebuffer, will be included in colorBlendInfo.pAttachments
	config.colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	config.colorBlendAttachment.blendEnable = VK_FALSE;
	config.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
	config.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
	config.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
	config.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
	config.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
	config.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

	config.colorBlendInfo.pAttachments = &config.colorBlendAttachment;
	config.colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
	config.colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
	config.colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
	config.colorBlendInfo.blendConstants[3] = 0.0f;  // Optional

	// depth detection
	config.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	config.depthStencilInfo.depthTestEnable = VK_TRUE;
	config.depthStencilInfo.depthWriteEnable = VK_TRUE;
	config.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	config.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	config.depthStencilInfo.minDepthBounds = 0.0f;  // Optional
	config.depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
	config.depthStencilInfo.stencilTestEnable = VK_FALSE;
	config.depthStencilInfo.front = {};  // Optional
	config.depthStencilInfo.back = {};   // Optional
}
void GraphicsPipeline::enableAlphaBlending(std::vector<VkPipelineColorBlendAttachmentState>& states)
{
	for (auto& state : states) {
		enableAlphaBlending(state);
	}
}
void GraphicsPipeline::defaultBlending(std::vector<VkPipelineColorBlendAttachmentState>& states) {
	for (auto& state : states) {
		defaultBlending(state);
	}
}
void GraphicsPipeline::enableAlphaBlending(VkPipelineColorBlendAttachmentState& state) {
	state.blendEnable = VK_TRUE;
	state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	state.colorBlendOp = VK_BLEND_OP_ADD;
	state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	state.alphaBlendOp = VK_BLEND_OP_ADD;
}
void GraphicsPipeline::defaultBlending(VkPipelineColorBlendAttachmentState& state) {
	state.blendEnable = VK_FALSE;
	state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	state.colorBlendOp = VK_BLEND_OP_ADD;
	state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	state.alphaBlendOp = VK_BLEND_OP_ADD;
}
void GraphicsPipeline::cmdBind(VkCommandBuffer commandBuffer) {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
}
}