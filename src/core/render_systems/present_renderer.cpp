#include "render_systems/present_renderer.hpp"
#include "utils/engine.hpp"

namespace naku {

PresentRenderer::PresentRenderer(
	Engine& engine, 
	RenderPass& renderPass, 
	uint32_t subpass,
	std::vector<FrameBufferAttachment*> inputGbufferAttachments,
	std::vector<VkImageLayout> inputGbufferLayouts)
	: _engine{engine}, _device { *engine.pDevice }, _resources{engine.resources}, _renderPass{renderPass}, _subpass{_renderPass.subpasses[subpass]} {
	//pushValue.presentIndex = 0;

	//if (_subpass.inputAttachments.size() != static_cast<size_t>(GBuffers::SIZE))
	//	throw std::runtime_error("Error: Present renderer requires subpass input attachments' num equaling to the Gbuffer num.");
	//if (_subpass.colorAttachments.size() != 1)
	//	throw std::runtime_error("Error: Present renderer requires subpass color attachments' num equaling to 1.");

	_imageCount = _renderPass.imageCount();
	if (_engine.resources.exist<Shader>("present.vert.spv"))
		_vert = _engine.resources.get<Shader>("present.vert.spv");
	else
		_vert = _engine.createShader("res/shader/present.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	if (_engine.resources.exist<Shader>("present.frag.spv"))
		_frag = _engine.resources.get<Shader>("present.frag.spv");
	else
		_frag = _engine.createShader("res/shader/present.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	// create sampler
	VkSamplerCreateInfo samplerInfo = Sampler::getDefaultSamplerCreateInfo();
	_sampler = std::make_unique<Sampler>(_device, samplerInfo);

	// create descriptor set
	{
		DescriptorSetLayout::Builder layoutBuilder(_device);
		for (uint32_t i = 0; i < inputGbufferAttachments.size(); i++)
			layoutBuilder.addBinding(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		_setLayout = layoutBuilder.build();
		_writers.reserve(_imageCount);
		for (size_t i = 0; i < _imageCount; i++) {
			auto writer = std::make_unique<DescriptorWriter>(*_setLayout, *_engine.pDescriptorSetPool);
			for (uint32_t ii = 0; ii < inputGbufferAttachments.size(); ii++) {
				writer->writeImage(ii, { _sampler->sampler(), inputGbufferAttachments[ii]->views[i], inputGbufferLayouts[ii]});
			}
			_writers.push_back(std::move(writer));
		}
		_sets.resize(_imageCount);
		for (size_t i = 0; i < _imageCount; i++) {
			_writers[i]->build(_sets[i]);
		}
	}
}

PresentRenderer::~PresentRenderer()
{
	vkDestroyPipelineLayout(device(), _pipelineLayout, nullptr);
}

void PresentRenderer::createPipeline()
{
	//prepare pipeline configs
	{
		GraphicsPipeline::defaultPipelineConfig(_config);
		_config.renderPass = _renderPass.renderPass();
		_config.subpass = _subpass.subpass;
		_config.vertStageCreateInfo = _vert->getCreateInfo();
		_config.fragStageCreateInfo = _frag->getCreateInfo();
		static std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(_subpass.colorAttachments.size());
		GraphicsPipeline::defaultBlending(colorBlendAttachments);
		_config.colorBlendInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
		_config.colorBlendInfo.pAttachments = colorBlendAttachments.data();
		_config.bindingDescriptions.clear();
		_config.attributeDescriptions.clear();
	}
	//prepare pipeline layout

	std::vector<VkDescriptorSetLayout> layouts;
	layouts.reserve(2);
	layouts.push_back(_engine.pGlobalSetLayout->getDescriptorSetLayout());
	layouts.push_back(_setLayout->getDescriptorSetLayout());
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstants);
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
		pipelineLayoutInfo.pSetLayouts = layouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(device(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) !=
			VK_SUCCESS) {
			throw std::runtime_error("Error: Failed to create pipeline layout!");
		}
	}

	// create pipeline
	_config.pipelineLayout = _pipelineLayout;
	_pipeline = std::make_unique<GraphicsPipeline>(_device, _config);
}

void PresentRenderer::render(FrameInfo& frameInfo, float alpha, float gamma, int presentIndex)
{
	_pipeline->cmdBind(frameInfo.commandBuffer);
	auto sets = frameInfo.globalSets;
	sets.push_back(_sets[frameInfo.imageIndex]);

	// models doesn't matter.
	// we only needs camera information
	static const std::array<uint32_t, 1> offsets{ 0 };

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		_pipelineLayout,
		0,
		static_cast<uint32_t>(sets.size()),
		sets.data(),
		static_cast<uint32_t>(offsets.size()),
		offsets.data());
	const PushConstants pushValues{ {alpha, gamma}, presentIndex };
	vkCmdPushConstants(
		frameInfo.commandBuffer,
		_pipelineLayout,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PushConstants),
		&pushValues);
	vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
}

}