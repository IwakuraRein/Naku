#ifndef SHADOWMAP_RENDERER_HPP
#define SHADOWMAP_RENDERER_HPP

#include "utils/device.hpp"
#include "utils/descriptors.hpp"
#include "utils/pipeline.hpp"
#include "utils/render_pass.hpp"
#include "render_systems/renderer.hpp"
#include "resources/light.hpp"
#include "resources/camera.hpp"

namespace naku {

extern class Engine;

class ShadowmapRenderer {
public:
	struct PushConstants {
		glm::mat4 depthPV;
	};
	ShadowmapRenderer(
		Engine& engine,
		RenderPass& renderPass,
		uint32_t subpass/*,
		const std::vector<VkImageView>& colorBuffers*/);
	~ShadowmapRenderer();

	ShadowmapRenderer(const ShadowmapRenderer&&) = delete;
	ShadowmapRenderer& operator=(const ShadowmapRenderer&&) = delete;

	void createPipeline();
	VkDevice device() const { return _device.device(); }
	void render(FrameInfo& frameInfo, Light& light, int cubeFace = -1);

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

	VkPipelineLayout _pipelineLayout;
	PipelineConfig _config{};
	std::unique_ptr<GraphicsPipeline> _pipeline;
};

}

#endif