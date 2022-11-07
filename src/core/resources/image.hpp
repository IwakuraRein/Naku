#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "naku.hpp"
#include "resources/resource.hpp"
#include "utils/buffer.hpp"
#include "utils/device.hpp"

namespace naku {

class Sampler {
public:
	Sampler(Device& device, VkSamplerCreateInfo createInfo);
	~Sampler();
	Sampler(const Sampler&) = delete;
	Sampler& operator=(const Sampler&) = delete;
	Sampler() = default;

	static VkSamplerCreateInfo getDefaultSamplerCreateInfo();

	VkDevice device() const { return _device.device(); }
	VkSampler sampler() const { return _sampler; }

private:
	Device& _device;
	VkSampler _sampler;
};

class Image2D : public Resource {
public:
	Image2D(
		Device& device,
		const std::string& name,
		const VkImageCreateInfo& imageCreateInfo);
	~Image2D();
	Image2D(const Image2D&) = delete;
	Image2D& operator=(const Image2D&) = delete;
	Image2D() = default;

	static std::shared_ptr<Image2D> loadImageFromFile(
		Device& device,
		const std::string& name,
		const std::string& filePath,
		uint32_t layer = 0,
		bool mipmap = true,
		bool forceRGBA = true);
	static std::shared_ptr<Image2D> loadCubeMapFromFile(std::string filePath, int formatBit, bool mipmap = true, bool forceRGBA=true);
	static VkImageCreateInfo getDefaultImageCreateInfo(VkExtent2D extent);
	static VkImageCreateInfo getDefaultCubeMapCreateInfo(VkExtent2D extent);
	static VkImageViewCreateInfo getDefaultImageViewCreateInfo(VkImage image, VkFormat format);
	static VkImage createImage(
		Device& device,
		const VkImageCreateInfo& imageInfo,
		VkMemoryPropertyFlags properties,
		VmaAllocation* pAllocation);
	static void copyBufferToImage(
		const Buffer& buffer,
		const Image2D& image,
		uint32_t mipLevel = 0,
		uint32_t baseArrayLayer = 0,
		uint32_t layerCount = 1);
	static void transitImageLayout(
		Device& device,
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		uint32_t baseMipLevel = 0,
		uint32_t levelCount = 1,
		uint32_t baseArrayLayer = 0,
		uint32_t layerCount = 1,
		bool stateAllCommands = false
	);
	static void transitImageLayout(
		Device& device,
		VkCommandBuffer commandBuffer,
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		uint32_t baseMipLevel = 0,
		uint32_t levelCount = 1,
		uint32_t baseArrayLayer = 0,
		uint32_t layerCount = 1,
		bool stateAllCommands = false
	);
	static void generateMipmaps(
		Device& device,
		VkImage image,
		VkFormat imageFormat,
		int32_t texWidth,
		int32_t texHeight,
		uint32_t mipLevels,
		uint32_t imageLayers=1);
	void generateMipmaps();

	VkExtent2D extent() const { return { _width, _height }; }
	uint32_t width() const { return _width; }
	uint32_t height() const { return _height; }
	VkDevice device() const { return _device.device(); }
	std::string filePath() const { return _filePath; }
	VkImage image() const { return _image; }
	VkImageView defaultImageView() const { return _defaultImageView; }

	uint32_t mipLevels() const {return _mipLevels;}
	uint32_t layers() const { return _layers; }
	VkFormat format() const { return _format; }

	friend class GUI;
	friend class Texture;

	std::unique_ptr<VkDescriptorSet> ImGuiImageId;

private:
	uint32_t _width, _height, _channel;
	//unsigned char _thumbnail[THUMBNAIL_WIDTH* THUMBNAIL_HEIGHT*4];
	std::string _filePath;
	uint32_t _mipLevels;
	uint32_t _layers;
	VkFormat _format;
	int _formatBit;
	VkImageUsageFlags _usage;
	//VkImageLayout _layout;
	VkImageTiling _tiling;

	VkImage _image;
	VkImageView _defaultImageView;
	//VkImageView _imageView;
	//VkDeviceMemory _imageMemory;
	VmaAllocation _allocation;
	uint32_t _valueCount;
};

}

#endif