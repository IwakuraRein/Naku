#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "io/window.hpp"
#include "utils/device.hpp"
#include "utils/swap_chain.hpp"
#include "utils/render_pass.hpp"
#include "resources/model.hpp"
#include "resources/object.hpp"
#include "resources/camera.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <cassert>
#include <memory>
#include <vector>

namespace naku {

extern class Engine;
extern class OpaqueRenderer;
extern class GbufferRenderer;
extern class TransparentRenderer;
extern class PresentRenderer;
extern class ShadowmapRenderer;

struct FrameInfo {
	const uint32_t& frameIndex;
	const uint32_t& imageIndex;
	const float& runningTime;
	const float& deltaTime;
	VkCommandBuffer commandBuffer;
	const std::vector<VkDescriptorSet>& globalSets;
	Camera& mainCam;
};

class Renderer {
public:
	template<typename T>
	struct RendererInfo {
		std::shared_ptr<RenderPass> pRenderPass;
		uint32_t subpass;
	};
	Renderer(
		Engine& engine,
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR);
	~Renderer();
	Renderer(const Renderer&) = delete;
	void operator=(const Renderer&) = delete;

	VkDevice device() const { return _device.device(); }

	VkExtent2D getSwapChainExtent() const { return _pSwapChain->getSwapChainExtent(); }

	VkCommandBuffer beginFrame();
	void endFrame();
	void beginRenderPass(
		VkCommandBuffer commandBuffer,
		RenderPass& renderpass,
		bool fullWindow = false,
		bool flipY = false);
	void nextSubpass(VkCommandBuffer commandBuffer) {
		vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
	}
	void endRenderPass(VkCommandBuffer commandBuffer);
	void beginShadowPass(VkCommandBuffer commandBuffer, bool flipY = false);
	void beginDeferredPass(VkCommandBuffer commandBuffer, bool fullWindow = false, bool flipViewPort = false) {
		beginRenderPass(commandBuffer, *gbufferPass, fullWindow, flipViewPort);
	}
	void beginPostPass(VkCommandBuffer commandBuffer, bool fullWindow = false, bool flipViewPort = false) {
		beginRenderPass(commandBuffer, *postPass, fullWindow, flipViewPort);
	}

	bool isFrameInProgress() const { return _isFrameStarted; }
	void changePresentMode(VkPresentModeKHR mode);
	VkCommandBuffer getCurrentCommandBuffer() const {
		assert(_isFrameStarted && "error: cannot get command buffer if the frame isn't in progress.");
		return _commandBuffers[_currentFrameIdx];
	}
	size_t imageCount() { return _pSwapChain->imageCount(); }
	uint32_t getFrameIndex() const {
		assert(_isFrameStarted && "Cannot get frame index when frame not in progress");
		return _currentFrameIdx;
	}
	uint32_t getImageIndex() const {
		assert(_isFrameStarted && "Cannot get image index when frame not in progress");
		return _currentImageIdx;
	}
	float getAspectRatio() const {
		return static_cast<float>(_window.viewPortWidth()) /
			static_cast<float>(_window.viewPortHeight());
	}
	size_t getImageCount() const { return _pSwapChain->imageCount(); }
	VkResult getFenceStatus(uint32_t imageIdx) const {
		return vkGetFenceStatus(device(), _pSwapChain->imagesInFlight[imageIdx]);
	}
	void createRenderers();
	//void recreateRenderers();
	//template<typename T>
	//void createRenderer(std::unique_ptr<T>& pRenderer, RenderPass& renderPass, uint32_t subpass) {
	//	pRenderer = std::make_unique<T>(_engine, renderPass, subpass);
	//	pRenderer->createPipeline();
	//}


	// gbuffer
	FrameBufferAttachment colorAttachment;
	FrameBufferAttachment presentAttachment;
	FrameBufferAttachment depthAttachment;
	FrameBufferAttachment normalOcclusionAttachment; // normal, occlusion
	FrameBufferAttachment albedoAttachment;
	FrameBufferAttachment metalRoughAttachment; //metalness, roughness
	FrameBufferAttachment emissionAttachment;
	FrameBufferAttachment mtlIdAttachment;
	FrameBufferAttachment objIdAttachment;

	FrameBufferAttachment shadowmapAttachment;

	std::unique_ptr<RenderPass> gbufferPass;
	std::unique_ptr<RenderPass> shadowPass;
	std::unique_ptr<RenderPass> opaquePass;
	std::unique_ptr<RenderPass> transparentPass;
	std::unique_ptr<RenderPass> postPass;

	std::unique_ptr<OpaqueRenderer> opaqueRenderer{nullptr};
	std::unique_ptr<GbufferRenderer> gbufferRenderer{ nullptr };
	std::unique_ptr<TransparentRenderer> transparentRenderer{ nullptr };
	std::unique_ptr<PresentRenderer> presentRenderer{ nullptr };
	std::unique_ptr<ShadowmapRenderer> shadowmapRenderer{ nullptr };
private:
	Engine& _engine;
	Window& _window;
	Device& _device;
	std::unique_ptr<SwapChain> _pSwapChain;
	std::vector<VkCommandBuffer> _commandBuffers;
	DescriptorPool& _descriptorPool;

	VkPresentModeKHR _presentMode;
	bool _presentModeWasChanged{ false };

	uint32_t _currentImageIdx{0};
	uint32_t _currentFrameIdx{0};
	bool _isFrameStarted{false};

	void recreate() {
		createSwapChain();
		createAttachments();
		createRenderPasses();
		createRenderers();
	}
	void createCommandBuffers();
	void createSwapChain();
	void createAttachments();
	void createRenderPasses();
	void createGbufferAttachment(FrameBufferAttachment& attachment, bool tripleBuffering = true);
	void freeCommandBuffers();
	void freeAttachments();
};

}

#endif