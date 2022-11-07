#include "render_systems/transparent_renderer.hpp"
#include "utils/engine.hpp"

#include <map>

namespace naku {

TransparentRenderer::TransparentRenderer(
	Engine& engine,
	RenderPass& renderPass,
	uint32_t subpass)
	: _engine{engine},
	_device{ *engine.pDevice },
	_resources{ engine.resources },
	_renderPass{ renderPass },
	_subpass{ renderPass.subpasses[subpass] },
	_imageCount{ _renderPass.imageCount() } {

	if (_engine.resources.exist<Shader>("transparent.vert.spv"))
		_vert = _engine.resources.get<Shader>("transparent.vert.spv");
	else
		_vert = _engine.createShader("res/shader/transparent.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	if (_engine.resources.exist<Shader>("transparent.frag.spv"))
		_frag = _engine.resources.get<Shader>("transparent.frag.spv");
	else
		_frag = _engine.createShader("res/shader/transparent.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	{
		auto samplerCreateInfo = Sampler::getDefaultSamplerCreateInfo();
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		_sampler = std::make_unique<Sampler>(_device, samplerCreateInfo);
	}
	{
		DescriptorSetLayout::Builder layoutBuilder(_device);
		layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); //shadowmap
		layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); //shadowmap
		_setLayout = layoutBuilder.build();
		_writers.reserve(_imageCount);
		for (size_t i = 0; i < _imageCount; i++) {
			auto writer = std::make_unique<DescriptorWriter>(*_setLayout, *_engine.pDescriptorSetPool);

			// shadowmap doesn't have triple buffering
			VkDescriptorImageInfo imgInfo{};
			imgInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;;
			imgInfo.imageView = Light::normalShadowmaps->defaultImageView();
			imgInfo.sampler = _sampler->sampler();
			writer->writeImage(0, imgInfo);

			imgInfo.imageView = Light::omniShadowmaps->defaultImageView();
			writer->writeImage(1, imgInfo);
			_writers.push_back(std::move(writer));
		}
		_sets.resize(_imageCount);
		for (size_t i = 0; i < _imageCount; i++) {
			_writers[i]->build(_sets[i]);
		}
	}

}

TransparentRenderer::~TransparentRenderer()
{
	vkDestroyPipelineLayout(device(), _pipelineLayout, nullptr);
}

void TransparentRenderer::createPipeline()
{
	//prepare pipeline configs
	GraphicsPipeline::defaultPipelineConfig(_config);
	_config.renderPass = _renderPass.renderPass();
	_config.subpass = _subpass.subpass;
	_config.vertStageCreateInfo = _vert->getCreateInfo();
	_config.fragStageCreateInfo = _frag->getCreateInfo();
	static std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(_subpass.colorAttachments.size());
	GraphicsPipeline::defaultBlending(colorBlendAttachments);
	GraphicsPipeline::enableAlphaBlending(colorBlendAttachments[0]); //shaded
	GraphicsPipeline::enableAlphaBlending(colorBlendAttachments[2]); //albedo
	GraphicsPipeline::enableAlphaBlending(colorBlendAttachments[4]); //emission
	_config.colorBlendInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
	_config.colorBlendInfo.pAttachments = colorBlendAttachments.data();

	_config.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_CULL_MODE};
	_config.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	_config.dynamicStateInfo.pDynamicStates = _config.dynamicStateEnables.data();
	_config.dynamicStateInfo.dynamicStateCount =
		static_cast<uint32_t>(_config.dynamicStateEnables.size());
	//_config.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;

	//prepare pipeline layout
	std::vector<VkDescriptorSetLayout> layouts;
	layouts.reserve(3);
	layouts.push_back(_engine.pGlobalSetLayout->getDescriptorSetLayout());
	layouts.push_back(_setLayout->getDescriptorSetLayout());
	layouts.push_back(Material::textureInputSetLayout->getDescriptorSetLayout());


	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(Material::PushConstants);
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

	// create pipeline
	_config.pipelineLayout = _pipelineLayout;
	_pipeline = std::make_unique<GraphicsPipeline>(_device, _config);
}

void TransparentRenderer::render(
	FrameInfo& frameInfo,
	const Object& obj)
{
	_pipeline->cmdBind(frameInfo.commandBuffer);
	auto& Materials = _resources.getResource<Material>();
	auto& Objects = _resources.getResource<Object>();


	auto sets = frameInfo.globalSets;
	sets.push_back(_sets[frameInfo.imageIndex]);

	if (obj.isActive()) {
		if (obj.model) {
			obj.material->cmdBindSet(frameInfo.commandBuffer, _pipelineLayout, sets.size());
			const uint32_t offsets = obj.getOffset();
			auto pushConstants = obj.material->pushConstants;
			vkCmdBindDescriptorSets(
				frameInfo.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				_pipelineLayout,
				0,
				static_cast<uint32_t>(sets.size()),
				sets.data(),
				1,
				&offsets);
			auto model = obj.model;
			model->cmdBind(frameInfo.commandBuffer);
			vkCmdSetCullMode(frameInfo.commandBuffer, VK_CULL_MODE_FRONT_BIT);
			pushConstants.side = 1;
			vkCmdPushConstants(
				frameInfo.commandBuffer,
				_pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(Material::PushConstants),
				&pushConstants);
			model->cmdDraw(frameInfo.commandBuffer);
			vkCmdSetCullMode(frameInfo.commandBuffer, VK_CULL_MODE_BACK_BIT);
			pushConstants.side = 0;
			vkCmdPushConstants(
				frameInfo.commandBuffer,
				_pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(Material::PushConstants),
				&pushConstants);
			model->cmdDraw(frameInfo.commandBuffer);
		}
	}
}

}