#include "render_systems/renderer.hpp"
#include "utils/engine.hpp"
#include "render_systems/present_renderer.hpp"
#include "render_systems/opaque_renderer.hpp"
#include "render_systems/gbuffer_renderer.hpp"
#include "render_systems/transparent_renderer.hpp"
#include "render_systems/shadowmap_renderer.hpp"

#include <iostream>

namespace naku {

Renderer::Renderer(
	Engine& engine,
	VkPresentModeKHR presentMode)
	: _engine{ engine },
	_device { *engine.pDevice },
	_window{ *engine.pWindow },
	_descriptorPool{ *engine.pDescriptorSetPool },
	_presentMode{presentMode} {
	createSwapChain();
	createCommandBuffers();
	createAttachments();
	createRenderPasses();
	createRenderers();
}

Renderer::~Renderer() {
	freeCommandBuffers();
	freeAttachments();
}

void Renderer::freeAttachments() {
	for (int i = 0; i < presentAttachment.images.size(); i++) {
		vkDestroyImageView(_device.device(), presentAttachment.views[i], nullptr);
	}
	presentAttachment.views.clear();

	colorAttachment.clear(_device);
	depthAttachment.clear(_device);
	normalOcclusionAttachment.clear(_device);
	albedoAttachment.clear(_device);
	metalRoughAttachment.clear(_device);
	emissionAttachment.clear(_device);
	objIdAttachment.clear(_device);
	mtlIdAttachment.clear(_device);
	shadowmapAttachment.clear(_device);
}

void Renderer::createAttachments() {
	freeAttachments();
	{
		_pSwapChain->createSwapChainColorAttachment(presentAttachment.images, presentAttachment.views);
		presentAttachment.format = _pSwapChain->_format;
		presentAttachment.clearValue.color = VkClearColorValue{ _engine.globalUbo.clearColor.x, _engine.globalUbo.clearColor.y, _engine.globalUbo.clearColor.z };
		presentAttachment.extent = getSwapChainExtent();
		presentAttachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	{
		colorAttachment.format = presentAttachment.format;
		colorAttachment.clearValue.color = VkClearColorValue{ _engine.globalUbo.clearColor.x, _engine.globalUbo.clearColor.y, _engine.globalUbo.clearColor.z };
		colorAttachment.extent = getSwapChainExtent();
		colorAttachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		createGbufferAttachment(colorAttachment);
	}
	{
		depthAttachment.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		depthAttachment.format = _device.findDepthFormat();
		depthAttachment.extent = getSwapChainExtent();
		depthAttachment.clearValue.depthStencil = { 1.0f, 0 };
		createGbufferAttachment(depthAttachment);
	}
	{
		normalOcclusionAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		normalOcclusionAttachment.extent = getSwapChainExtent();
		normalOcclusionAttachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		normalOcclusionAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
		createGbufferAttachment(normalOcclusionAttachment);
	}
	{
		albedoAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
		albedoAttachment.extent = getSwapChainExtent();
		albedoAttachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		albedoAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
		createGbufferAttachment(albedoAttachment);
	}
	{
		metalRoughAttachment.format = VK_FORMAT_R16G16_SFLOAT;
		metalRoughAttachment.extent = getSwapChainExtent();
		metalRoughAttachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		metalRoughAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
		createGbufferAttachment(metalRoughAttachment);
	}
	{
		emissionAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
		emissionAttachment.extent = getSwapChainExtent();
		emissionAttachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		emissionAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
		createGbufferAttachment(emissionAttachment);
	}
	{
		mtlIdAttachment.format = VK_FORMAT_R32_UINT;
		mtlIdAttachment.extent = getSwapChainExtent();
		mtlIdAttachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		mtlIdAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
		createGbufferAttachment(mtlIdAttachment);
	}
	{
		objIdAttachment.format = VK_FORMAT_R32_UINT;
		objIdAttachment.extent = getSwapChainExtent();
		objIdAttachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		objIdAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
		createGbufferAttachment(objIdAttachment);
	}
	{
		shadowmapAttachment.format = _device.findDepthFormat();
		shadowmapAttachment.extent = VkExtent2D{ SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT };
		shadowmapAttachment.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		shadowmapAttachment.clearValue.depthStencil = { 1.0f, 0 };
		createGbufferAttachment(shadowmapAttachment, false);
	}
}

void Renderer::createGbufferAttachment(FrameBufferAttachment & attachment, bool tripleBuffering) {
	VkImageAspectFlags aspectMask = 0;

	if (attachment.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	if (attachment.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		auto bit = getFormatBit(attachment.format);
		if (bit | FormatBit::D)
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		else
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	assert(aspectMask > 0);
	VkImageCreateInfo imageInfo = Image2D::getDefaultImageCreateInfo(attachment.extent);
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = attachment.format;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	// VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT flag is required for input attachments
	imageInfo.usage = attachment.usage;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	int counts{ 1 };
	if (tripleBuffering) {
		counts = imageCount();
		attachment.images.resize(imageCount());
		attachment.allocs.resize(imageCount());
		attachment.views.resize(imageCount());
	}
	else {
		attachment.images.resize(1);
		attachment.allocs.resize(1);
		attachment.views.resize(1);
	}
	for (int i = 0; i < counts; i++) {
		attachment.images[i] = Image2D::createImage(
			_device,
			imageInfo,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&attachment.allocs[i]);

		VkImageViewCreateInfo imageView{};
		imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageView.format = attachment.format;
		imageView.subresourceRange = {};
		imageView.subresourceRange.aspectMask = aspectMask;
		imageView.subresourceRange.baseMipLevel = 0;
		imageView.subresourceRange.levelCount = 1;
		imageView.subresourceRange.baseArrayLayer = 0;
		imageView.subresourceRange.layerCount = 1;
		imageView.image = attachment.images[i];

		if (vkCreateImageView(_device.device(), &imageView, nullptr, &attachment.views[i])) {
			throw std::runtime_error("Error: Failed to create image view!");
		};
	}
}
void Renderer::createSwapChain() {
	//_currentFrameIdx = 0;
	_currentImageIdx = 0;
	auto extent = _window.extent();
	// pause when the window is minimized
	while (extent.height == 0 || extent.width == 0) {
		extent = _window.extent();
		glfwWaitEvents();
	}
	// wait until the current swapchain is not used
	vkDeviceWaitIdle(device());

	//if (_pSwapChain == nullptr) _pSwapChain = std::make_unique<SwapChain>(_device, extent, _presentMode);
	//else {
	//	std::shared_ptr<SwapChain> oldSwapChain = std::move(_pSwapChain);
	//	_pSwapChain = std::make_unique<SwapChain>(_device, extent, oldSwapChain, _presentMode);

	//	//if (!oldSwapChain->compareSwapFormats(*_pSwapChain.get())) {
	//	//	throw std::runtime_error("Swap chain image(or depth) format has changed!");
	//	//}
	//}

	//using oldSwapchain causes bug. still don't know why.
	_pSwapChain.reset();
	_pSwapChain = std::make_unique<SwapChain>(_device, extent, _presentMode);
}
void Renderer::createRenderPasses() {
	RenderPassAttachment PresentAttachment{ presentAttachment };
	RenderPassAttachment ColorAttachment{ colorAttachment };
	RenderPassAttachment DepthAttachment{ depthAttachment };
	RenderPassAttachment NormalOcclusionAttachment{ normalOcclusionAttachment };
	RenderPassAttachment AlbedoAttachment{ albedoAttachment };
	RenderPassAttachment MetalRoughAttachment{ metalRoughAttachment };
	RenderPassAttachment EmisionAttachment{ emissionAttachment };
	RenderPassAttachment MtlIdAttachment{ mtlIdAttachment };
	RenderPassAttachment ObjIdAttachment{ objIdAttachment };
	RenderPassAttachment ShadowmapAttachment{ shadowmapAttachment };

	{ // prepare attachment descriptions
		ShadowmapAttachment.description.format = shadowmapAttachment.format;
		ShadowmapAttachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}
	shadowPass = RenderPass::Builder(
		_device,
		*_pSwapChain,
		{ ShadowmapAttachment })
		.addSubPass({}, {}, 0)
		.build();
	{
		ShadowmapAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		ShadowmapAttachment.description.initialLayout = ShadowmapAttachment.description.finalLayout;

		ColorAttachment.description.format = colorAttachment.format;
		DepthAttachment.description.format = depthAttachment.format;
		NormalOcclusionAttachment.description.format = normalOcclusionAttachment.format;
		AlbedoAttachment.description.format = albedoAttachment.format;
		MetalRoughAttachment.description.format = metalRoughAttachment.format;
		EmisionAttachment.description.format = emissionAttachment.format;
		MtlIdAttachment.description.format = mtlIdAttachment.format;
		ObjIdAttachment.description.format = objIdAttachment.format;

		ColorAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		DepthAttachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		NormalOcclusionAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		AlbedoAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		MetalRoughAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		EmisionAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		MtlIdAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		ObjIdAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	gbufferPass = RenderPass::Builder(
		_device,
		*_pSwapChain,
		{ ColorAttachment,
		  NormalOcclusionAttachment,
		  AlbedoAttachment,
		  MetalRoughAttachment,
		  EmisionAttachment,
		  MtlIdAttachment,
		  ObjIdAttachment,
		  DepthAttachment })
		.addSubPass({ 1, 2, 3, 4, 5, 6 }, {}, 7) // generate gbuffer
		.build();
	{
		ColorAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		ColorAttachment.description.initialLayout = ColorAttachment.description.finalLayout;
		ColorAttachment.description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		DepthAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		DepthAttachment.description.initialLayout = DepthAttachment.description.finalLayout;
		//DepthAttachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthAttachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		NormalOcclusionAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		NormalOcclusionAttachment.description.initialLayout = NormalOcclusionAttachment.description.finalLayout;
		NormalOcclusionAttachment.description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		AlbedoAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		AlbedoAttachment.description.initialLayout = AlbedoAttachment.description.finalLayout;
		AlbedoAttachment.description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		MetalRoughAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		MetalRoughAttachment.description.initialLayout = MetalRoughAttachment.description.finalLayout;
		MetalRoughAttachment.description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		EmisionAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		EmisionAttachment.description.initialLayout = EmisionAttachment.description.finalLayout;
		EmisionAttachment.description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		MtlIdAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		MtlIdAttachment.description.initialLayout = MtlIdAttachment.description.finalLayout;
		EmisionAttachment.description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		ObjIdAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		ObjIdAttachment.description.initialLayout = ObjIdAttachment.description.finalLayout;
		ObjIdAttachment.description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	opaquePass = RenderPass::Builder(
		_device,
		*_pSwapChain,
		{ ColorAttachment,
		  NormalOcclusionAttachment,
		  AlbedoAttachment,
		  MetalRoughAttachment,
		  EmisionAttachment,
		  MtlIdAttachment,
		  ObjIdAttachment,
		  DepthAttachment })
		.addSubPass({ 0 }, { /*1, 2, 3, 4, 5, 6*/ }, -1) // render opaque objects
		.build();
	{
		ColorAttachment.description.initialLayout = ColorAttachment.description.finalLayout;
		ColorAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		DepthAttachment.description.initialLayout = DepthAttachment.description.finalLayout;
		DepthAttachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		NormalOcclusionAttachment.description.initialLayout = NormalOcclusionAttachment.description.finalLayout;
		NormalOcclusionAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		AlbedoAttachment.description.initialLayout = AlbedoAttachment.description.finalLayout;
		AlbedoAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		MetalRoughAttachment.description.initialLayout = MetalRoughAttachment.description.finalLayout;
		MetalRoughAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		EmisionAttachment.description.initialLayout = EmisionAttachment.description.finalLayout;
		EmisionAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		MtlIdAttachment.description.initialLayout = MtlIdAttachment.description.finalLayout;
		MtlIdAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		ObjIdAttachment.description.initialLayout = ObjIdAttachment.description.finalLayout;
		ObjIdAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	transparentPass = RenderPass::Builder(
		_device,
		*_pSwapChain,
		{ ColorAttachment,
		  NormalOcclusionAttachment,
		  AlbedoAttachment,
		  MetalRoughAttachment,
		  EmisionAttachment,
		  MtlIdAttachment,
		  ObjIdAttachment,
		  DepthAttachment })
		.addSubPass({ 0, 1, 2, 3, 4, 5, 6 }, {}, 7) // render transparent objects
		.build();
	{
		PresentAttachment.description.format = presentAttachment.format;
		PresentAttachment.description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
	postPass = RenderPass::Builder(
		_device,
		*_pSwapChain,
		{ PresentAttachment/*,
		  NormalOcclusionAttachment,
		  AlbedoAttachment,
		  MetalRoughAttachment,
		  EmisionAttachment,
		  ObjIdAttachment,
		  DepthAttachment*/ })
		.addSubPass({ 0 }, {/*0, 1, 2, 3, 4, 5, 6*/ }, -1) // choose present attachment
		.build();
}
void Renderer::createRenderers() {
	gbufferRenderer = std::make_unique<GbufferRenderer>(_engine, *gbufferPass, 0);
	gbufferRenderer->createPipeline();
	opaqueRenderer = std::make_unique<OpaqueRenderer>(_engine, *opaquePass, 0,
		std::vector<FrameBufferAttachment*>{ 
			&normalOcclusionAttachment,
			&albedoAttachment,
			&metalRoughAttachment,
			&emissionAttachment,
			&depthAttachment },
		std::vector<VkImageLayout>{ 
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		});
	opaqueRenderer->createPipeline();
	transparentRenderer = std::make_unique<TransparentRenderer>(_engine, *transparentPass, 0);
	transparentRenderer->createPipeline();
	presentRenderer = std::make_unique<PresentRenderer>(_engine, *postPass, 0,
		std::vector<FrameBufferAttachment*>{ 
			&colorAttachment,
			&normalOcclusionAttachment,
			&albedoAttachment,
			&metalRoughAttachment,
			&emissionAttachment,
			//&mtlIdAttachment,
			//&objIdAttachment,
			&depthAttachment },
		std::vector<VkImageLayout>{ 
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		//VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		//VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		});
	presentRenderer->createPipeline();
	shadowmapRenderer = std::make_unique<ShadowmapRenderer>(_engine, *shadowPass, 0);
	shadowmapRenderer->createPipeline();
}
void Renderer::changePresentMode(VkPresentModeKHR mode) {
	_presentMode = mode;
	_presentModeWasChanged = true;
	//createSwapChain();
	//createAttachments();
	//createRenderPasses();
	//createRenderers();
}

void Renderer::createCommandBuffers() {
	// how many frame buffers does this device support
	_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = _device.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());

	if (vkAllocateCommandBuffers(device(), &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Error: Failed to allocate command buffers.");
	}
}

VkCommandBuffer Renderer::beginFrame() {
	assert(!_isFrameStarted && "Error: Frame has already started.");
	auto result = _pSwapChain->acquireNextImage(&_currentImageIdx);

	// VK_ERROR_OUT_OF_DATE_KHR:
	// A surface has changed so that it is no longer compatible with the swapchain
	// and further presentation requests using the swapchain will fail. Engine must
	// query the new surface properties and recreate their swapchain.
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate();
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Error: Failed to acquire swap chain image!");
	}

	_isFrameStarted = true;
	auto commandBuffer = getCurrentCommandBuffer();
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Error: Failed to begin recording command buffer!");
	}
	return commandBuffer;
}

void Renderer::endFrame() {
	assert(_isFrameStarted && "Error: Frame isn't in progress.");
	auto commandBuffer = getCurrentCommandBuffer();
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	auto result = _pSwapChain->submitCommandBuffers(&commandBuffer, &_currentImageIdx);

	// VK_SUBOPTIMAL_KHR:
	// A swapchain no longer matches the surface properties exactly, 
	// but can still be used to present to the surface successfully.
	 if (result == VK_ERROR_OUT_OF_DATE_KHR ||
		 result == VK_SUBOPTIMAL_KHR ||
		 _window.wasResized() ||
		 _presentModeWasChanged)
		recreate();
	if (_window.wasResized()) _window.resetResizeFlag();
	if (_presentModeWasChanged) _presentModeWasChanged = false;

	_isFrameStarted = false;
	_currentFrameIdx = (_currentFrameIdx + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::beginRenderPass(VkCommandBuffer commandBuffer, RenderPass& renderpass, bool fullWindow, bool flipY) {
	assert(_isFrameStarted && "Error: Can't call begin render pass if frame is not in progress");
	assert(
		commandBuffer == getCurrentCommandBuffer() &&
		"Error: Can't begin render pass on command buffer from a different frame");

	auto startCoord = _window.viewPortCoord();
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderpass.renderPass();
	renderPassInfo.framebuffer = renderpass.frameBuffers[_currentImageIdx];

	//renderPassInfo.renderArea.offset = { static_cast<int>(startCoord.x), static_cast<int>(startCoord.y) };
	renderPassInfo.renderArea.offset = { 0, 0 };
	//renderPassInfo.renderArea.extent = _window.viewPortExtent();
	renderPassInfo.renderArea.extent = _pSwapChain->getSwapChainExtent();

	renderPassInfo.clearValueCount = static_cast<uint32_t>(renderpass.clearValues.size());
	renderPassInfo.pClearValues = renderpass.clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	static float viewportX{ static_cast<float>(_window.viewPortWidth()) };
	static float viewportY{ static_cast<float>(_window.viewPortHeight()) };
	static VkExtent2D swapChainExtent{ _pSwapChain->getSwapChainExtent() };
	if (!fullWindow) {
		_pSwapChain->getSwapChainExtent();
		viewport.x = startCoord.x;
		if (flipY)
			viewport.y = _window.viewPortHeight() - startCoord.y;
		else
			viewport.y = startCoord.y;
		if (_window.viewPortWidth() > 0) {
			viewportX = static_cast<float>(_window.viewPortWidth());
		}
		if (_window.viewPortHeight() > 0) {
			if (flipY)
				viewportY = -static_cast<float>(_window.viewPortHeight());
			else
				viewportY = static_cast<float>(_window.viewPortHeight());
		}
	}
	else {
		swapChainExtent = _pSwapChain->getSwapChainExtent();
		viewport.x = 0.f;
		if (flipY)
			viewport.y = static_cast<float>(swapChainExtent.height);
		else
			viewport.y = 0.f;
		if (swapChainExtent.width > 0) {
			viewportX = static_cast<float>(swapChainExtent.width);
		}
		if (swapChainExtent.height > 0) {
			if (flipY)
				viewportY = -static_cast<float>(swapChainExtent.height);
			else
				viewportY = static_cast<float>(swapChainExtent.height);
		}
	}
	viewport.width = viewportX;
	viewport.height = viewportY;

	viewport.minDepth = MIN_DEPTH;
	viewport.maxDepth = MAX_DEPTH;
	VkRect2D scissor{ {0,0}, _pSwapChain->getSwapChainExtent() };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::endRenderPass(VkCommandBuffer commandBuffer) {
	assert(_isFrameStarted && "Error: Can't call endRenderPass if frame is not in progress");
	assert(
		commandBuffer == getCurrentCommandBuffer() &&
		"Error: Can't end render pass on command buffer from a different frame");
	vkCmdEndRenderPass(commandBuffer);
}

void Renderer::beginShadowPass(VkCommandBuffer commandBuffer, bool flipY)
{
	assert(_isFrameStarted && "Error: Can't call beginDefferedRenderPass if frame is not in progress");
	assert(
		commandBuffer == getCurrentCommandBuffer() &&
		"Error: Can't begin render pass on command buffer from a different frame");
	assert(shadowPass && "Error: Can't begin shadow pass if it's not created yet.");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = shadowPass->renderPass();
	renderPassInfo.framebuffer = shadowPass->frameBuffers[_currentImageIdx];

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = {SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT};

	renderPassInfo.clearValueCount = static_cast<uint32_t>(shadowPass->clearValues.size());
	renderPassInfo.pClearValues = shadowPass->clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};

	viewport.width = static_cast<float>(SHADOWMAP_WIDTH);
	if (!flipY) {
		viewport.height = static_cast<float>(SHADOWMAP_HEIGHT);
	}
	else {
		viewport.height = -static_cast<float>(SHADOWMAP_HEIGHT);
	}

	viewport.x = 0.f;
	if (!flipY) {
		viewport.y = 0.f;
	}
	else {
		viewport.y = static_cast<float>(SHADOWMAP_HEIGHT);
	}

	viewport.minDepth = MIN_DEPTH;
	viewport.maxDepth = MAX_DEPTH;
	VkRect2D scissor{ {0,0}, {SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT} };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::freeCommandBuffers() {
	vkFreeCommandBuffers(
		device(),
		_device.getCommandPool(),
		static_cast<uint32_t>(_commandBuffers.size()),
		_commandBuffers.data());
	_commandBuffers.clear();
}

}