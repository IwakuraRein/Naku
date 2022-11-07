#include "render_systems/shadowmap_renderer.hpp"
#include "utils/engine.hpp"

namespace naku {

ShadowmapRenderer::ShadowmapRenderer(
	Engine& engine,
	RenderPass& renderPass,
	uint32_t subpass)
	: _engine{ engine },
	_device{ *engine.pDevice },
	_resources{ engine.resources },
	_renderPass{ renderPass },
	_subpass{ renderPass.subpasses[subpass] },
	_imageCount{ _renderPass.imageCount() } {

	if (_engine.resources.exist<Shader>("shadowmap.vert.spv"))
		_vert = _engine.resources.get<Shader>("shadowmap.vert.spv");
	else
		_vert = _engine.createShader("res/shader/shadowmap.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	if (_engine.resources.exist<Shader>("shadowmap.frag.spv"))
		_frag = _engine.resources.get<Shader>("shadowmap.frag.spv");
	else
		_frag = _engine.createShader("res/shader/shadowmap.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

}

ShadowmapRenderer::~ShadowmapRenderer()
{
	vkDestroyPipelineLayout(device(), _pipelineLayout, nullptr);
}

void ShadowmapRenderer::createPipeline()
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

	_config.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	_config.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	_config.dynamicStateInfo.pDynamicStates = _config.dynamicStateEnables.data();
	_config.dynamicStateInfo.dynamicStateCount =
		static_cast<uint32_t>(_config.dynamicStateEnables.size());
	_config.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
	_config.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

	//prepare pipeline layout
	std::vector<VkDescriptorSetLayout> layouts;
	layouts.reserve(1);
	layouts.push_back(_engine.pGlobalSetLayout->getDescriptorSetLayout());


	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
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

	// create pipeline
	_config.pipelineLayout = _pipelineLayout;
	_pipeline = std::make_unique<GraphicsPipeline>(_device, _config);
}

void ShadowmapRenderer::render(FrameInfo& frameInfo, Light& light, int cubeFace)
{
	_pipeline->cmdBind(frameInfo.commandBuffer);

	auto& Objects = _resources.getResource<Object>();
	if (!light.isActive())
		return;
	PushConstants push{};
	if (cubeFace >= 0 && cubeFace < 6) {
		//const glm::mat4 projMat{ glm::perspective(glm::half_pi<float>(), 1.0f, light.projector.near, light.lightInfo.radius)};
		const glm::mat4 projMat = Projector::getPerspProjMat(90.f, 1.0f, LIGHT_PROJECT_NEAR, light.lightInfo.radius);

		if (cubeFace == 0) {
			//push.depthPV = Projector::getViewMatrix(light.position(), { 180.f, -90.f, 0.f });
			push.depthPV = Projector::getViewMatrixFromDirection(light.position(), { 1.f, 0.f, 0.f });
			//push.depthPV = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		}
		else if (cubeFace == 1) {
			//push.depthPV = Projector::getViewMatrix(light.position(), { 180.f, 90.f, 0.f });
			push.depthPV = Projector::getViewMatrixFromDirection(light.position(), { -1.f, 0.f, 0.f });
			//push.depthPV = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		}
		else if (cubeFace == 2) {
			//push.depthPV = Projector::getViewMatrix(light.position(), { -90.f, 0.f, 0.f });
			push.depthPV = Projector::getViewMatrixFromDirection(light.position(), { 0.f, 1.f, 0.f }, {0.f, 0.f, 1.f});
			//push.depthPV = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		}
		else if (cubeFace == 3) {
			//push.depthPV = Projector::getViewMatrix(light.position(), { 90.f, 0.f, 0.f });
			push.depthPV = Projector::getViewMatrixFromDirection(light.position(), { 0.f, -1.f, 0.f }, { 0.f, 0.f, -1.f });
			//push.depthPV = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
		}
		else if (cubeFace == 4) {
			//push.depthPV = Projector::getViewMatrix(light.position(), { 0.f, 0.f, 180.f });
			push.depthPV = Projector::getViewMatrixFromDirection(light.position(), { 0.f, 0.f, 1.f });
			//push.depthPV = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		}
		else if (cubeFace == 5) {
			//push.depthPV = Projector::getViewMatrix(light.position(), { 180.f, 0.f, 0.f });
			push.depthPV = Projector::getViewMatrixFromDirection(light.position(), { 0.f, 0.f, -1.f });
			//push.depthPV = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		}
		push.depthPV = projMat * push.depthPV;
	}
	else {
		push.depthPV = light.lightInfo.projViewMat;
	}
	vkCmdPushConstants(
		frameInfo.commandBuffer,
		_pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(PushConstants),
		&push);
	for (ResId objId = 0; objId < Objects.size(); objId++) {
		if (!Objects.exist(objId)) continue;
		auto obj = Objects[objId];
		if (obj->type() != Object::Type::MESH) continue;
		if (obj->isActive() && obj->model && obj->castShadow()) {
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
			auto model = obj->model;
			model->cmdBind(frameInfo.commandBuffer);
			model->cmdDraw(frameInfo.commandBuffer);
		}
	}
}

}