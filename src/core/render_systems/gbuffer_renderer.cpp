#include "render_systems/gbuffer_renderer.hpp"
#include "utils/engine.hpp"

namespace naku {

naku::GbufferRenderer::GbufferRenderer(
	Engine& engine,
	RenderPass& renderPass,
	uint32_t subpass)
	:_engine{ engine },
	_device { *_engine.pDevice },
	_resources{ _engine.resources }, 
	_renderPass{ renderPass }, 
	_subpass{ renderPass.subpasses[subpass] } {

	//if (_subpass.colorAttachments.size() != (GBUFFER_NUM-2)) // no color buffr and depth buffer
	//	throw std::runtime_error("Error: Gbuffer renderer requires subpass color attachments' num equaling to the G-buffer's num-1.");
	_imageCount = _renderPass.imageCount();

	if (_engine.resources.exist<Shader>("gbuffer.vert.spv"))
		_vert = _engine.resources.get<Shader>("gbuffer.vert.spv");
	else
		_vert = _engine.createShader("res/shader/gbuffer.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	if (_engine.resources.exist<Shader>("gbuffer.frag.spv"))
		_frag = _engine.resources.get<Shader>("gbuffer.frag.spv");
	else
		_frag = _engine.createShader("res/shader/gbuffer.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	//DescriptorWriter writer(*Material::textureInputSetLayout, *_engine.pDescriptorSetPool);
	//writer.build(_textureInputSet);
}

GbufferRenderer::~GbufferRenderer()
{
	vkDestroyPipelineLayout(device(), _pipelineLayout, nullptr);
}

void GbufferRenderer::createPipeline()
{
	//prepare pipeline configs
	GraphicsPipeline::defaultPipelineConfig(_config);
	_config.renderPass = _renderPass.renderPass();
	_config.subpass = _subpass.subpass;
	_config.vertStageCreateInfo = _vert->getCreateInfo();
	_config.fragStageCreateInfo = _frag->getCreateInfo();
	static std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(_subpass.colorAttachments.size());
	GraphicsPipeline::defaultBlending(colorBlendAttachments);
	_config.colorBlendInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
	_config.colorBlendInfo.pAttachments = colorBlendAttachments.data();
	
	std::vector<VkDescriptorSetLayout> layouts;
	layouts.reserve(2);
	layouts.push_back(_engine.pGlobalSetLayout->getDescriptorSetLayout());
	layouts.push_back(Material::textureInputSetLayout->getDescriptorSetLayout());

	//prepare pipeline layout
	{
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
	}

	// create pipeline
	_config.pipelineLayout = _pipelineLayout;
	_pipeline = std::make_unique<GraphicsPipeline>(_device, _config);
}

void GbufferRenderer::render(FrameInfo& frameInfo)
{
	_pipeline->cmdBind(frameInfo.commandBuffer);
	auto& Materials = _resources.getResource<Material>();
	auto& Objects = _resources.getResource<Object>();

	for (ResId mtlId = 0; mtlId < Materials.size(); mtlId++) {
		if (!Materials.exist(mtlId)) continue;
		auto material = Materials[mtlId];
		if (material->type() != Material::Type::OPAQUE) continue;
		material->cmdBindSet(frameInfo.commandBuffer, _pipelineLayout, frameInfo.globalSets.size());
		auto& objIds = Materials.getCollect<Object>(mtlId);
		for (ResId objId : objIds) {
			if (!Objects.exist(objId)) continue;
			auto obj = Objects[objId];
			if (obj->isActive()) {
				if (obj->model) {
					const uint32_t offsets = obj->getOffset();
					vkCmdBindDescriptorSets(
						frameInfo.commandBuffer,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						_pipelineLayout,
						0,
						static_cast<uint32_t>(frameInfo.globalSets.size()),
						frameInfo.globalSets.data(),
						1,
						&offsets);
					vkCmdPushConstants(
						frameInfo.commandBuffer,
						_pipelineLayout,
						VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
						0,
						sizeof(Material::PushConstants),
						&material->pushConstants);
					auto model = obj->model;
					model->cmdBind(frameInfo.commandBuffer);
					model->cmdDraw(frameInfo.commandBuffer);
				}
			}
		}
	}
}

}
