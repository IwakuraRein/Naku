#ifndef TRANSPARENT_RENDERER_HPP
#define TRANSPARENT_RENDERER_HPP

#include "utils/device.hpp"
#include "utils/descriptors.hpp"
#include "utils/pipeline.hpp"
#include "utils/render_pass.hpp"
#include "render_systems/renderer.hpp"
#include "resources/material.hpp"
#include "resources/camera.hpp"

namespace naku {

extern class Engine;

class TransparentRenderer {

public:
	TransparentRenderer(
		Engine& engine,
		RenderPass& renderPass,
		uint32_t subpass);
	~TransparentRenderer();

	TransparentRenderer(const TransparentRenderer&&) = delete;
	TransparentRenderer& operator=(const TransparentRenderer&&) = delete;

	void createPipeline();
	VkDevice device() const { return _device.device(); }
	void render(
		FrameInfo& frameInfo,
		const Object& obj);

	friend class Renderer;

private:
	Engine& _engine;
	Device& _device;
	ResourceManager& _resources;
	RenderPass& _renderPass;
	RenderPass::Subpass& _subpass;
	std::unique_ptr<Sampler> _sampler;

	size_t _imageCount;

	std::unique_ptr<DescriptorSetLayout> _setLayout;
	std::vector<std::unique_ptr<DescriptorWriter>> _writers;
	std::vector<VkDescriptorSet> _sets;

	std::shared_ptr<Shader> _vert;
	std::shared_ptr<Shader> _frag;

	VkPipelineLayout _pipelineLayout;
	PipelineConfig _config{};
	std::unique_ptr<GraphicsPipeline> _pipeline;

};

}

#endif