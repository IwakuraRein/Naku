#ifndef GBUFFER_RENDERER_HPP
#define GBUFFER_RENDERER_HPP

#include "utils/device.hpp"
#include "utils/descriptors.hpp"
#include "utils/pipeline.hpp"
#include "utils/render_pass.hpp"
#include "render_systems/renderer.hpp"
#include "resources/material.hpp"
#include "resources/camera.hpp"

#include <memory>
#include <array>

namespace naku {

extern class Engine;

class GbufferRenderer {
public:
	GbufferRenderer(
		Engine& engine,
		RenderPass& renderPass,
		uint32_t subpass);
	~GbufferRenderer();

	GbufferRenderer(const GbufferRenderer&&) = delete;
	GbufferRenderer& operator=(const GbufferRenderer&&) = delete;

	void createPipeline();
	VkDevice device() const { return _device.device(); }
	void render(FrameInfo& frameInfo);

	friend class Renderer;

private:
	Engine& _engine;
	Device& _device;
	ResourceManager& _resources;
	RenderPass& _renderPass;
	RenderPass::Subpass& _subpass;
	size_t _imageCount;
	std::shared_ptr<Shader> _vert;
	std::shared_ptr<Shader> _frag;
	//std::unique_ptr<DescriptorSetLayout> _setLayout;
	//std::vector<std::unique_ptr<DescriptorWriter>> _writers;
	//std::vector<VkDescriptorSet> _sets;
	//VkDescriptorSet _textureInputSet;
	VkPipelineLayout _pipelineLayout;
	PipelineConfig _config{};
	std::unique_ptr<GraphicsPipeline> _pipeline;

};

}

#endif