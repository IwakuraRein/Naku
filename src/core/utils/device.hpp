#ifndef DEVICE_HPP
#define DEVICE_HPP

#include "naku.hpp"
#include "io/window.hpp"

namespace naku {

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
	uint32_t graphicsFamily;
	uint32_t presentFamily;
	uint32_t computeFamily;
	bool hasGraphicsFamily = false;
	bool hasPresentFamily = false;
	bool hasComputeFamily = false;
	bool isComplete() { return hasGraphicsFamily && hasPresentFamily && hasComputeFamily; }
};

class Device {
public:
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	Device(Window& _window);
	~Device();

	// Not copyable or movable
	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;
	Device(Device&&) = delete;
	Device& operator=(Device&&) = delete;
	Device() = default;

	VkCommandPool getCommandPool() { return _commandPool; }
	VkDevice device() { return _device; }
	VmaAllocator allocator() { return _allocator; }
	VkSurfaceKHR surface() { return _surface; }
	VkQueue graphicsQueue() { return _graphicsQueue; }
	VkQueue presentQueue() { return _presentQueue; }
	VkQueue computeQueue() { return _computeQueue; }
	VkInstance instance() { return _instance; }
	VkPhysicalDevice physicalDevice() { return _physicalDevice; }

	SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(_physicalDevice); }
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(_physicalDevice); }
	VkFormat findSupportedFormat(
		const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat() {
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	// Buffer Helper Functions

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	VkPhysicalDeviceProperties properties;

private:
	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createCommandPool();
	void createAllocator();

	// helper functions
	bool isDeviceSuitable(VkPhysicalDevice device);
	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayerSupport();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void hasGflwRequiredInstanceExtensions();
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debugMessenger;
	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
	Window& _window;
	VkCommandPool _commandPool;

	VkDevice _device;
	VkSurfaceKHR _surface;
	VkQueue _graphicsQueue;
	VkQueue _presentQueue;
	VkQueue _computeQueue;

	VmaAllocator _allocator;

	const std::vector<const char*> _validationLayers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> _deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME };
};

}

#endif
