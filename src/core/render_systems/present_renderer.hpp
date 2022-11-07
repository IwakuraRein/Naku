#ifndef PRESENT_RENDERER_HPP
#define PRESENT_RENDERER_HPP

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

class PresentRenderer {
public:
	struct PushConstants {
		alignas(8) glm::vec2 alphaGamma{ 1.f, 1.f };
		int presentIndex{ 0 };
	} /*pushValue*/;
	PresentRenderer(
		Engine& engine,
		RenderPass& renderPass,
		uint32_t subpass,
		std::vector<FrameBufferAttachment*> inputGbufferAttachments,
		std::vector<VkImageLayout> inputGbufferLayouts);
	~PresentRenderer();

	PresentRenderer(const PresentRenderer&&) = delete;
	PresentRenderer& operator=(const PresentRenderer&&) = delete;

	void createPipeline();
	VkDevice device() const { return _device.device(); }
	void render(FrameInfo& frameInfo, float alpha, float gamma, int presentIndex);

	friend class Renderer;

private:
	Engine& _engine;
	Device& _device;
	ResourceManager& _resources;
	RenderPass& _renderPass;
	RenderPass::Subpass& _subpass;
	std::unique_ptr<Sampler> _sampler;
	size_t _imageCount;
	std::shared_ptr<Shader> _vert;
	std::shared_ptr<Shader> _frag;
	std::unique_ptr<DescriptorSetLayout> _setLayout;
	std::vector<std::unique_ptr<DescriptorWriter>> _writers;
	std::vector<VkDescriptorSet> _sets;
	VkDescriptorSet _textureInputSet;
	VkPipelineLayout _pipelineLayout;
	PipelineConfig _config{};
	std::unique_ptr<GraphicsPipeline> _pipeline;

};

}

#endif