#ifndef OPAQUE_RENDERER_HPP
#define OPAQUE_RENDERER_HPP

#include "utils/pipeline.hpp"
#include "utils/device.hpp"
#include "utils/render_pass.hpp"
#include "render_systems/renderer.hpp"
#include "resources/resource.hpp"
#include "utils/descriptors.hpp"
#include "resources/material.hpp"
#include "resources/camera.hpp"

#include <map>
#include <vector>

namespace naku {

extern class Engine;

class OpaqueRenderer {
public:

	OpaqueRenderer(
		Engine& engine,
		RenderPass& renderPass,
		uint32_t subpass,
		std::vector<FrameBufferAttachment*> inputGbufferAttachments,
		std::vector<VkImageLayout> inputGbufferLayouts);
	~OpaqueRenderer();

	OpaqueRenderer(const OpaqueRenderer&) = delete;
	OpaqueRenderer& operator=(const OpaqueRenderer&) = delete;

	VkDevice device() const { return _device.device(); }


	void createPipeline();
	void render(FrameInfo frameInfo);
	//void renderObjects(
	//	FrameInfo& frameInfo,
	//	Material& material,
	//	VkPipelineLayout& pipelineLayout,
	//	GraphicsPipeline& pipeline);
	void cmdPushConstants(VkCommandBuffer cmd, Material::PushConstants push, VkPipelineLayout pipelineLayout);
	//VkPipelineLayout pipelineLayout(const std::string& name) { return _pipelineLayouts[name]; }
	VkPipelineLayout pipelineLayout() { return _pipelineLayout; }

private:
	Engine& _engine;
	Device& _device;
	RenderPass& _renderPass;
	RenderPass::Subpass& _subpass;
	ResourceManager& _resources;
	std::shared_ptr<Shader> _vert;
	std::shared_ptr<Shader> _frag;

	size_t _imageCount;

	std::unique_ptr<Sampler> _sampler;
	std::unique_ptr<DescriptorSetLayout> _setLayout;
	std::vector<std::unique_ptr<DescriptorWriter>> _writers;
	std::vector<VkDescriptorSet> _sets;

	//std::unordered_map<std::string, std::unique_ptr<GraphicsPipeline>> _pipelines;
	//std::map<std::string, VkPipelineLayout> _pipelineLayouts;
	PipelineConfig _config;
	std::unique_ptr<GraphicsPipeline> _pipeline;
	VkPipelineLayout _pipelineLayout;
	//void _createPipeline(
	//	const std::string& name,
	//	PipelineConfig& pipelineConfig,
	//	const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
};

}

#endif