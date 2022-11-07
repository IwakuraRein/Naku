#include "resources/light.hpp"
#include "utils/swap_chain.hpp"

#include <glm/gtc/epsilon.hpp>

namespace naku { //Light

size_t Light::_instanceCount = 0;
std::shared_ptr<Image2D> Light::normalShadowmaps = nullptr;
std::shared_ptr<Image2D> Light::omniShadowmaps = nullptr;

void Light::copyShadowmap(
	Device& device, 
	VkCommandBuffer commandBuffer,
	FrameBufferAttachment& shadowmapAttachment,
	uint32_t no,
	int cubeFace)
{
	if (no >= MAX_NORMAL_SHADOWMAP_NUM) {
		std::cerr << "Error: Shadowmap number exceeds the limitation." << std::endl;
		return;
	}

	// shadowmap's mip level is 1
	// shadowmap doesn't use tripple buffering

	// Make sure color writes to the framebuffer are finished before using it as transfer source
	Image2D::transitImageLayout(
		device,
		commandBuffer,
		shadowmapAttachment.images[0],
		shadowmapAttachment.format,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		0, 1, 0, 1);

	// Change image layout of the shadowmap to transfer destination
	if (cubeFace < 0 || cubeFace > 5) {
		Image2D::transitImageLayout(
			device,
			commandBuffer,
			normalShadowmaps->image(),
			normalShadowmaps->format(),
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0, 1, no, 1);
	}
	else {
		Image2D::transitImageLayout(
			device,
			commandBuffer,
			omniShadowmaps->image(),
			omniShadowmaps->format(),
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0, 1, no*6+cubeFace, 1);
	}

	// Copy region for transfer from framebuffer to cube face
	VkImageCopy copyRegion = {};

	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	copyRegion.srcSubresource.baseArrayLayer = 0;
	copyRegion.srcSubresource.mipLevel = 0;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.srcOffset = { 0, 0, 0 };

	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (cubeFace < 0 || cubeFace > 5)
		copyRegion.dstSubresource.baseArrayLayer = no;
	else copyRegion.dstSubresource.baseArrayLayer = no * 6 + cubeFace;
	copyRegion.dstSubresource.mipLevel = 0;
	copyRegion.dstSubresource.layerCount = 1;
	copyRegion.dstOffset = { 0, 0, 0 };

	copyRegion.extent.width = SHADOWMAP_WIDTH;
	copyRegion.extent.height = SHADOWMAP_HEIGHT;
	copyRegion.extent.depth = 1;

	// Put image copy into command buffer
	if (cubeFace < 0 || cubeFace > 5) {
		vkCmdCopyImage(
			commandBuffer,
			shadowmapAttachment.images[0],
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			normalShadowmaps->image(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&copyRegion);
	}
	else {
		vkCmdCopyImage(
			commandBuffer,
			shadowmapAttachment.images[0],
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			omniShadowmaps->image(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&copyRegion);
	}

	// Transform framebuffer color attachment back
	Image2D::transitImageLayout(
		device,
		commandBuffer,
		shadowmapAttachment.images[0],
		shadowmapAttachment.format,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		0, 1, 0, 1);

	//
	if (cubeFace < 0 || cubeFace > 5) {
		Image2D::transitImageLayout(
			device,
			commandBuffer,
			normalShadowmaps->image(),
			normalShadowmaps->format(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			0, 1, no, 1);
	}
	else {
		Image2D::transitImageLayout(
			device,
			commandBuffer,
			omniShadowmaps->image(),
			omniShadowmaps->format(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			0, 1, no * 6 + cubeFace, 1);
	}
}

Light::Light(
	Device& device,
	std::string name,
	Type type)
	: Resource{device, name}, _lightType { type },
	Object(device, name, Object::Type::LIGHT) {

	lightInfo.type = type;
	projector.setViewMatrix(_position, _rotation);
	lightInfo.direction = projector.forwardDir;
	lightInfo.position = _position;
	updateProjection();

	if (_instanceCount++ == 0) { //create shadowmap array
		auto createInfo = Image2D::getDefaultImageCreateInfo({ SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT });
		createInfo.format = device.findDepthFormat();
		createInfo.arrayLayers = MAX_NORMAL_SHADOWMAP_NUM;
		createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		normalShadowmaps = std::make_shared<Image2D>(device, "normalShadowmaps", createInfo);
		Image2D::transitImageLayout(
			device,
			normalShadowmaps->image(),
			normalShadowmaps->format(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			0, 1, 0, MAX_NORMAL_SHADOWMAP_NUM, true);

		createInfo = Image2D::getDefaultCubeMapCreateInfo({ SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT });
		createInfo.format = device.findDepthFormat();
		createInfo.arrayLayers = MAX_OMNI_SHADOWMAP_NUM * 6;
		createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		omniShadowmaps = std::make_shared<Image2D>(device, "omniShadowmaps", createInfo);
		Image2D::transitImageLayout(
			device,
			omniShadowmaps->image(),
			omniShadowmaps->format(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			0, 1, 0, MAX_OMNI_SHADOWMAP_NUM * 6, true);
	}
}

Light::~Light() {
	if (--_instanceCount == 0) {
		normalShadowmaps.reset();
		omniShadowmaps.reset();
	}
}

void Light::setId(ResId id) {
	Resource::_id = id;
}
void Light::updateProjection() {
	if (_lightType == Type::SPOT) {
		projector.setPerspProjection(
			lightInfo.outerAngle,
			1.f,
			LIGHT_PROJECT_NEAR,
			lightInfo.radius);
	}
	else if (_lightType == Type::DIRECTIONAL) {
		projector.setOrthoProjection(
			-lightInfo.radius,
			lightInfo.radius,
			-lightInfo.radius,
			lightInfo.radius,
			LIGHT_PROJECT_NEAR,
			lightInfo.radius);
	}

	lightInfo.projViewMat = projector.projMat * projector.viewMat;
}

void Light::update() {
	Object::update(); //update transform matrix
	//update view matrix
	projector.setViewMatrix(_position, _rotation);
	lightInfo.direction = projector.forwardDir;
	lightInfo.position = _position;

	//no need to update projection matrix

	lightInfo.projViewMat = projector.projMat * projector.viewMat;
}

void Light::setDirection(const glm::vec3& direction) {
	static const glm::vec3 up{ 0.f, -1.f, 0.f };
	_rotation = Projector::getRotationFromOrientation(direction, up);

    Object::update();
	projector.orientate(direction, up);
	lightInfo.direction = projector.forwardDir;
	lightInfo.position = _position;
	lightInfo.projViewMat = projector.projMat * projector.viewMat;
}

}