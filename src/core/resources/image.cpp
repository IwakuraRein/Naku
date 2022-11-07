#include "resources/image.hpp"

#include <imgui_impl_vulkan.h>

#include <stb_image.h>
#include <stb_image_resize.h>
#include <stb_image_write.h>

namespace naku {//Image

VkImageCreateInfo Image2D::getDefaultImageCreateInfo(VkExtent2D extent) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = extent.width;
	imageInfo.extent.height = extent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;

	return imageInfo;
}
VkImageCreateInfo Image2D::getDefaultCubeMapCreateInfo(VkExtent2D extent) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = extent.width;
	imageInfo.extent.height = extent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 6;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	return imageInfo;
}

VkImageViewCreateInfo Image2D::getDefaultImageViewCreateInfo(VkImage image, VkFormat format)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	return viewInfo;
}

VkImage Image2D::createImage(
	Device& device,
	const VkImageCreateInfo& imageInfo,
	VkMemoryPropertyFlags memoryPropertyFlags,
	VmaAllocation* pAllocation) {/*
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device.device(), image, &memRequirements);*/

	VmaAllocationCreateInfo allocCreateInfo{};
	if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	else if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

	VkImage image;
	if (vmaCreateImage(device.allocator(), &imageInfo, &allocCreateInfo, &image, pAllocation, nullptr)) {
		throw std::runtime_error("failed to allocate image memory!");
	}
	return image;
}

void Image2D::copyBufferToImage(
	const Buffer& buffer, 
	const Image2D& image, 
	uint32_t mipLevel,
	uint32_t baseArrayLayer,
	uint32_t layerCount
	) {
	VkCommandBuffer commandBuffer = image._device.beginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = mipLevel;
	region.imageSubresource.baseArrayLayer = baseArrayLayer;
	region.imageSubresource.layerCount = layerCount;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { image._width, image._height, 1 };

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer.getBuffer(),
		image._image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region);
	image._device.endSingleTimeCommands(commandBuffer);
}

std::shared_ptr<Image2D> Image2D::loadImageFromFile(
	Device& device,
	const std::string& name,
	const std::string& filePath,
	uint32_t layer,
	bool mipmap,
	bool forceRGBA)
{
	void* data = nullptr;
	int w, h, c;
	bool hdr{ false };
	hdr = strEndWith(filePath, ".exr") || strEndWith(filePath, ".hdr") || strEndWith(filePath, ".pfm");
	if (forceRGBA) {
		if (hdr) {
			//TODO
		}
		else {
			data = stbi_load(filePath.c_str(), &w, &h, &c, STBI_rgb_alpha); 
		}
	}
	else {
		if (hdr) {
			//TODO
		}
		else {
			data = stbi_load(filePath.c_str(), &w, &h, &c, STBI_default);
		}
	}

	if (data == nullptr) {
		std::cerr << "Error: Can not load image file at: " << filePath << std::endl;
		return nullptr;
	}
	if (forceRGBA) c = 4;

	int format;
	if (c == 1 && hdr) format = FormatBit::R | FormatBit::BIT16 | FormatBit::UNORM;
	if (c == 1 && !hdr) format = FormatBit::R | FormatBit::BIT8 | FormatBit::UNORM;
	if (c == 2 && hdr) format = FormatBit::RG | FormatBit::BIT16 | FormatBit::UNORM;
	if (c == 2 && !hdr) format = FormatBit::RG | FormatBit::BIT8 | FormatBit::UNORM;
	if (c == 3 && hdr) format = FormatBit::RGB | FormatBit::BIT16 | FormatBit::UNORM;
	if (c == 3 && !hdr) format = FormatBit::RGB | FormatBit::BIT8 | FormatBit::UNORM;
	if (c == 4 && hdr) format = FormatBit::RGBA | FormatBit::BIT16 | FormatBit::UNORM;
	if (c == 4 && !hdr) format = FormatBit::RGBA | FormatBit::BIT8 | FormatBit::UNORM;

	auto imageCreateInfo = getDefaultImageCreateInfo({ static_cast<uint32_t>(w), static_cast<uint32_t>(h) });
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.format = getVkFormat(format);
	if (mipmap) {
		imageCreateInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(w, h)))) + 1;
	}
	else imageCreateInfo.mipLevels = 1;

	auto image = std::make_shared<Image2D>(device, name, imageCreateInfo);

	size_t size;
	if (hdr) {
		size = sizeof(char) * 2;
	}
	else size = sizeof(char);
	Buffer stagingBuffer{
		device,
		size,
		image->_valueCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	//stagingBuffer.map();
	stagingBuffer.writeToBuffer(data);

	// The image was created with the VK_IMAGE_LAYOUT_UNDEFINED layout, 
	// so that one should be specified as old layout when transitioning image
	transitImageLayout(device, 
		image->_image, 
		image->_format, 
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		0, image->_mipLevels);
	copyBufferToImage(stagingBuffer, *image, layer);
	
	image->generateMipmaps();
	return image;
}

Image2D::Image2D(
	Device& device,
	const std::string& name,
	const VkImageCreateInfo& imageCreateInfo)
	: Resource{ device, name } {
	_width = imageCreateInfo.extent.width;
	_height = imageCreateInfo.extent.height;
	_format = imageCreateInfo.format;
	_formatBit = getFormatBit(_format);
	if (_formatBit & RGBA)
		_channel = 4;
	else if (_formatBit & RGB)
		_channel = 3;
	else if (_formatBit & RG)
		_channel = 2;
	else if ((_formatBit & R) || (_formatBit & D))
		_channel = 1;
	_valueCount = _width * _height * _channel;
	_mipLevels = imageCreateInfo.mipLevels;
	_layers = imageCreateInfo.arrayLayers;
	_usage = imageCreateInfo.usage;
	//_layout = imageCreateInfo.initialLayout;
	_tiling = imageCreateInfo.tiling;
	_mipLevels = imageCreateInfo.mipLevels;

	_image = createImage(_device, imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_allocation);
	auto imageViewCreateInfo = getDefaultImageViewCreateInfo(_image, imageCreateInfo.format);
	VkImageAspectFlags aspectMask = 0;
	//if (imageCreateInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
	//	aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//}
	if (imageCreateInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		if (_formatBit & FormatBit::D)
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		else
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	if (imageCreateInfo.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
		imageViewCreateInfo.viewType = _layers > 6? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
	else imageViewCreateInfo.viewType = _layers > 1? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.subresourceRange.levelCount = _mipLevels;
	imageViewCreateInfo.subresourceRange.layerCount = _layers;
	imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
	vkCreateImageView(_device.device(), &imageViewCreateInfo, nullptr, &_defaultImageView);
}

Image2D::~Image2D() {
	vkDestroyImageView(device(), _defaultImageView, nullptr);
	vmaDestroyImage(_device.allocator(), _image, _allocation);
}

void Image2D::generateMipmaps() {
	generateMipmaps(_device, _image, _format, _width, _height, _mipLevels);
}

void Image2D::generateMipmaps(
	Device& device,
	VkImage image,
	VkFormat imageFormat,
	int32_t texWidth,
	int32_t texHeight,
	uint32_t mipLevels,
	uint32_t imageLayers) {
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device.physicalDevice(), imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("Error: The image format does not support linear blitting!");
	}
	
	if (mipLevels <= 1) return;

	VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = imageLayers;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	//for (uint32_t layer = 0; layer < imageLayers; layer++) {
		for (uint32_t i = 1; i < mipLevels; i++) {
			// Vulkan allows us to transition each mip level of an image independently.
			// Each blit will only deal with two mip levels at a time, so we can transition each level into the optimal layout between blits commands.
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = imageLayers;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = imageLayers;

			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}
	//}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	device.endSingleTimeCommands(commandBuffer);
}

void Image2D::transitImageLayout(
	Device& device,
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t baseMipLevel,
	uint32_t levelCount,
	uint32_t baseArrayLayer,
	uint32_t layerCount,
	bool allCommands) {

	auto cmd = device.beginSingleTimeCommands();
	transitImageLayout(
		device, 
		cmd, 
		image, 
		format, 
		oldLayout, 
		newLayout, 
		baseMipLevel, 
		levelCount, 
		baseArrayLayer, 
		layerCount, 
		allCommands);
	device.endSingleTimeCommands(cmd);
}
	
void Image2D::transitImageLayout(
	Device& device,
	VkCommandBuffer cmd,
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t baseMipLevel,
	uint32_t levelCount,
	uint32_t baseArrayLayer,
	uint32_t layerCount,
	bool allCommands) {

	auto formatBit = getFormatBit(format);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = formatBit & FormatBit::D ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = baseMipLevel;
	barrier.subresourceRange.levelCount = levelCount;
	barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
	barrier.subresourceRange.layerCount = layerCount;
	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;


	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch (oldLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		// Image layout is undefined (or does not matter)
		// Only valid as initial layout
		// No flags required, listed only for completeness

		barrier.srcAccessMask = 0;

		//The top of pipe is considered to be hit as soon as the device starts processing the command.
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		break;

	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		// Image is preinitialized
		// Only valid as initial layout for linear images, preserves memory contents
		// Make sure host writes have been finished

		// The memory referenced is mapped and will be written to by the host.
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image is a color attachment
		// Make sure any writes to the color buffer have been finished

		// The memory referenced is used to back an image used as a color attachment that will be written to.
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image is a depth/stencil attachment
		// Make sure any writes to the depth/stencil buffer have been finished

		// The memory referenced is used to back an image used as a depth or stencil attachment
		// that will be written to because the relevant write mask is enabled.
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image is a transfer source
		// Make sure any reads from the image have been finished

		// The memory referenced is used as the source of data in a transfer operation 
		// such as vkCmdCopyImage(), vkCmdCopyBuffer(), or vkCmdCopyBufferToImage().
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		// Any pending transfers triggered as a result of calls to vkCmdCopyImage() or vkCmdCopyBuffer(),
		// for example, have completed.
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image is a transfer destination
		// Make sure any writes to the image have been finished
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image is read by a shader
		// Make sure any shader reads from the image have been finished
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	case  VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;

	default:
		throw std::invalid_argument("unsupported layout transition!");
		break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (newLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image will be used as a transfer destination
		// Make sure any writes to the image have been finished
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image will be used as a transfer source
		// Make sure any reads from the image have been finished
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image will be used as a color attachment
		// Make sure any writes to the color buffer have been finished
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image layout will be used as a depth/stencil attachment
		// Make sure any writes to depth/stencil buffer have been finished
		barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image will be read in a shader (sampler, input attachment)
		// Make sure any writes to the image have been finished
		if (barrier.srcAccessMask == 0)
		{
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		if (barrier.srcAccessMask == 0)
		{
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	default:
		throw std::invalid_argument("unsupported layout transition!");
		break;
	}

	if (!allCommands)
		vkCmdPipelineBarrier(
			cmd,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	else
		vkCmdPipelineBarrier(
			cmd,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
}

} //image

namespace naku { //sampler

VkSamplerCreateInfo Sampler::getDefaultSamplerCreateInfo() {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 1.f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	//anisotropic_filtering
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;

	return samplerInfo;
}

Sampler::Sampler(Device& device, VkSamplerCreateInfo createInfo)
	: _device{ device } {
	if (vkCreateSampler(_device.device(), &createInfo, nullptr, &_sampler) != VK_SUCCESS) {
		throw std::runtime_error("Error: Failed to create sampler!");
	}
}

Sampler::~Sampler() {
	vkDestroySampler(device(), _sampler, nullptr);
}

}