#ifndef NAKU_HPP
#define NAKU_HPP

#include <glm/glm.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <json.hpp>

#include <cassert>
#include <string>
#include <codecvt>
#include <vector>
#include <array>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <stdexcept>
#include <filesystem>
#include <memory>
#include <limits>
#include <typeinfo>
#include <cstdlib>
#include <cstring>

#ifndef MapHas
#define MapHas(X, Y) (X.find(Y) != X.end())
#endif

#ifndef VectorHas
#define VectorHas(X, Y) (find(X.begin(), X.end(), Y) != X.end())
#endif

namespace naku{

using ResId = size_t;

static constexpr unsigned int GLOBAL_SETS_NUM           = 1;
static constexpr unsigned int MAX_FRAMES_IN_FLIGHT      = 2;
static constexpr unsigned int MAX_OBJECT_NUM            = 65536;
static constexpr unsigned int MAX_LIGHT_NUM             = 64;
static constexpr unsigned int MAX_NORMAL_SHADOWMAP_NUM  = 24;
static constexpr unsigned int MAX_OMNI_SHADOWMAP_NUM    = 16;
static constexpr unsigned int THUMBNAIL_WIDTH           = 256;
static constexpr unsigned int THUMBNAIL_HEIGHT          = 256;
static constexpr ResId ERROR_RES_ID                     = 18446744073709551615;

static constexpr float MIN_DEPTH                        = 0.f;
static constexpr float MAX_DEPTH                        = 1.f;

static constexpr uint32_t SHADOWMAP_WIDTH               = 1024;
static constexpr uint32_t SHADOWMAP_HEIGHT              = 1024;
static constexpr float LIGHT_PROJECT_NEAR               = 0.01f;

// base texture, normal texture, pbr texture, occlusion texture, emission texture
enum class MaterialTextures {
	BASE = 0,
	NORMAL = 1,
	METALNESS = 2,
	ROUGHNESS = 3,
	OCCLUSION = 4,
	EMISSION = 5,
	SIZE = 6,
};

// color, albedo, normal_occlusion, metal_rough, emission, obj_id, depth
enum class GBuffers {
	COLOR = 0,
	ALBEDO = 1,
	NORMAL_OCCLUSION = 2,
	METAL_ROUGH = 3,
	EMISSION = 4,
	OBJ_ID = 5,
	DEPTH = 6,
	SIZE = 7,
};

enum FormatBit {
	UNDEFINED = 0,
	R         = (1 << 0),
	RG        = (1 << 1),
	RGB       = (1 << 2),
	RGBA      = (1 << 3),
	D         = (1 << 4),
	BIT8      = (1 << 5),
	BIT16     = (1 << 6),
	BIT24     = (1 << 7),
	BIT32     = (1 << 8),
	SINT      = (1 << 9),
	UINT      = (1 << 10),
	UNORM     = (1 << 11),
	SNORM     = (1 << 12),
	USCALED   = (1 << 13),
	SSCALED   = (1 << 14),
	SRGB      = (1 << 15),
	SFLOAT    = (1 << 16),
	ERROR     = 0xFFFFFFFF
};

inline VkFormat getVkFormat(int bits) {
	if ((bits & FormatBit::RGBA) != 0) {
		if ((bits & FormatBit::BIT8) != 0) {
			if ((bits & FormatBit::SINT) != 0) return VK_FORMAT_R8G8B8A8_SINT;
			if ((bits & FormatBit::UINT) != 0) return VK_FORMAT_R8G8B8A8_UINT;
			if ((bits & FormatBit::UNORM) != 0) return VK_FORMAT_R8G8B8A8_UNORM;
			if ((bits & FormatBit::SNORM) != 0) return VK_FORMAT_R8G8B8A8_SNORM;
			if ((bits & FormatBit::USCALED) != 0) return VK_FORMAT_R8G8B8A8_USCALED;
			if ((bits & FormatBit::SSCALED) != 0) return VK_FORMAT_R8G8B8A8_SSCALED;
			if ((bits & FormatBit::SRGB) != 0) return VK_FORMAT_R8G8B8A8_SRGB;
		}
		if ((bits & FormatBit::BIT16) != 0) {
			if ((bits & FormatBit::SINT) != 0) return VK_FORMAT_R16G16B16A16_SINT;
			if ((bits & FormatBit::UINT) != 0) return VK_FORMAT_R16G16B16A16_UINT;
			if ((bits & FormatBit::UNORM) != 0) return VK_FORMAT_R16G16B16A16_UNORM;
			if ((bits & FormatBit::SNORM) != 0) return VK_FORMAT_R16G16B16A16_SNORM;
			if ((bits & FormatBit::USCALED) != 0) return VK_FORMAT_R16G16B16A16_USCALED;
			if ((bits & FormatBit::SSCALED) != 0) return VK_FORMAT_R16G16B16A16_SSCALED;
			if ((bits & FormatBit::SFLOAT) != 0) return VK_FORMAT_R16G16B16A16_SFLOAT;
		}
		if ((bits & FormatBit::BIT32) != 0) {
			if ((bits & FormatBit::SINT) != 0) return VK_FORMAT_R32G32B32A32_SINT;
			if ((bits & FormatBit::UINT) != 0) return VK_FORMAT_R32G32B32A32_UINT;
			if ((bits & FormatBit::SFLOAT) != 0) return VK_FORMAT_R32G32B32A32_SFLOAT;
		}
	}
	else if ((bits & FormatBit::RGB) != 0) {
		if ((bits & FormatBit::BIT8) != 0) {
			if ((bits & FormatBit::SINT) != 0) return VK_FORMAT_R8G8B8_SINT;
			if ((bits & FormatBit::UINT) != 0) return VK_FORMAT_R8G8B8_UINT;
			if ((bits & FormatBit::UNORM) != 0) return VK_FORMAT_R8G8B8_UNORM;
			if ((bits & FormatBit::SNORM) != 0) return VK_FORMAT_R8G8B8_SNORM;
			if ((bits & FormatBit::USCALED) != 0) return VK_FORMAT_R8G8B8_USCALED;
			if ((bits & FormatBit::SSCALED) != 0) return VK_FORMAT_R8G8B8_SSCALED;
			if ((bits & FormatBit::SRGB) != 0) return VK_FORMAT_R8G8B8_SRGB;
		}
		if ((bits & FormatBit::BIT16) != 0) {
			if ((bits & FormatBit::SINT) != 0) return VK_FORMAT_R16G16B16_SINT;
			if ((bits & FormatBit::UINT) != 0) return VK_FORMAT_R16G16B16_UINT;
			if ((bits & FormatBit::UNORM) != 0) return VK_FORMAT_R16G16B16_UNORM;
			if ((bits & FormatBit::SNORM) != 0) return VK_FORMAT_R16G16B16_SNORM;
			if ((bits & FormatBit::USCALED) != 0) return VK_FORMAT_R16G16B16_USCALED;
			if ((bits & FormatBit::SSCALED) != 0) return VK_FORMAT_R16G16B16_SSCALED;
			if ((bits & FormatBit::SFLOAT) != 0) return VK_FORMAT_R16G16B16_SFLOAT;
		}
		if ((bits & FormatBit::BIT32) != 0) {
			if ((bits & FormatBit::SINT) != 0) return VK_FORMAT_R32G32B32_SINT;
			if ((bits & FormatBit::UINT) != 0) return VK_FORMAT_R32G32B32_UINT;
			if ((bits & FormatBit::SFLOAT) != 0) return VK_FORMAT_R32G32B32_SFLOAT;
		}
	}
	else if ((bits & FormatBit::RG) != 0) {
		if ((bits & FormatBit::BIT8) != 0) {
			if ((bits & FormatBit::SINT) != 0) return VK_FORMAT_R8G8_SINT;
			if ((bits & FormatBit::UINT) != 0) return VK_FORMAT_R8G8_UINT;
			if ((bits & FormatBit::UNORM) != 0) return VK_FORMAT_R8G8_UNORM;
			if ((bits & FormatBit::SNORM) != 0) return VK_FORMAT_R8G8_SNORM;
			if ((bits & FormatBit::USCALED) != 0) return VK_FORMAT_R8G8_USCALED;
			if ((bits & FormatBit::SSCALED) != 0) return VK_FORMAT_R8G8_SSCALED;
			if ((bits & FormatBit::SRGB) != 0) return VK_FORMAT_R8G8_SRGB;
		}
		if ((bits & FormatBit::BIT16) != 0) {
			if ((bits & FormatBit::SINT) != 0) return VK_FORMAT_R16G16_SINT;
			if ((bits & FormatBit::UINT) != 0) return VK_FORMAT_R16G16_UINT;
			if ((bits & FormatBit::UNORM) != 0) return VK_FORMAT_R16G16_UNORM;
			if ((bits & FormatBit::SNORM) != 0) return VK_FORMAT_R16G16_SNORM;
			if ((bits & FormatBit::USCALED) != 0) return VK_FORMAT_R16G16_USCALED;
			if ((bits & FormatBit::SSCALED) != 0) return VK_FORMAT_R16G16_SSCALED;
			if ((bits & FormatBit::SFLOAT) != 0) return VK_FORMAT_R16G16_SFLOAT;
		}
		if ((bits & FormatBit::BIT32) != 0) {
			if ((bits & FormatBit::SINT) != 0) return VK_FORMAT_R32G32_SINT;
			if ((bits & FormatBit::UINT) != 0) return VK_FORMAT_R32G32_UINT;
			if ((bits & FormatBit::SFLOAT) != 0) return VK_FORMAT_R32G32_SFLOAT;
		}
	}
	else if ((bits & FormatBit::R) != 0) {
		if ((bits & FormatBit::BIT8) != 0) {
			if ((bits & FormatBit::SINT) != 0) return VK_FORMAT_R8_SINT;
			if ((bits & FormatBit::UINT) != 0) return VK_FORMAT_R8_UINT;
			if ((bits & FormatBit::UNORM) != 0) return VK_FORMAT_R8_UNORM;
			if ((bits & FormatBit::SNORM) != 0) return VK_FORMAT_R8_SNORM;
			if ((bits & FormatBit::USCALED) != 0) return VK_FORMAT_R8_USCALED;
			if ((bits & FormatBit::SSCALED) != 0) return VK_FORMAT_R8_SSCALED;
			if ((bits & FormatBit::SRGB) != 0) return VK_FORMAT_R8_SRGB;
		}
		if ((bits & FormatBit::BIT16) != 0) {
			if ((bits & FormatBit::SINT) != 0) return VK_FORMAT_R16_SINT;
			if ((bits & FormatBit::UINT) != 0) return VK_FORMAT_R16_UINT;
			if ((bits & FormatBit::UNORM) != 0) return VK_FORMAT_R16_UNORM;
			if ((bits & FormatBit::SNORM) != 0) return VK_FORMAT_R16_SNORM;
			if ((bits & FormatBit::USCALED) != 0) return VK_FORMAT_R16_USCALED;
			if ((bits & FormatBit::SSCALED) != 0) return VK_FORMAT_R16_SSCALED;
			if ((bits & FormatBit::SFLOAT) != 0) return VK_FORMAT_R16_SFLOAT;
		}
		if ((bits & FormatBit::BIT32) != 0) {
			if ((bits & FormatBit::SINT) != 0) return VK_FORMAT_R32_SINT;
			if ((bits & FormatBit::UINT) != 0) return VK_FORMAT_R32_UINT;
			if ((bits & FormatBit::SFLOAT) != 0) return VK_FORMAT_R32_SFLOAT;
		}
	}
	else if ((bits & FormatBit::D) != 0) {
		if ((bits & FormatBit::BIT16) != 0) {
			if ((bits & FormatBit::UNORM) != 0) return VK_FORMAT_D16_UNORM;
		}
		if ((bits & FormatBit::BIT24) != 0) {
			//TODO
		}
		if ((bits & FormatBit::BIT32) != 0) {
			if ((bits & FormatBit::SFLOAT) != 0) return VK_FORMAT_D32_SFLOAT;
		}
	}
	return VK_FORMAT_UNDEFINED;
}

inline int getFormatBit(VkFormat format) {
	switch (format)
	{
	case VK_FORMAT_R8_SINT:
		return FormatBit::R| FormatBit::BIT8 | FormatBit::SINT;
		break;
	case VK_FORMAT_R8_UINT:
		return FormatBit::R | FormatBit::BIT8 | FormatBit::UINT;
		break;
	case VK_FORMAT_R8_UNORM:
		return FormatBit::R | FormatBit::BIT8 | FormatBit::UNORM;
		break;
	case VK_FORMAT_R8_SNORM:
		return FormatBit::R | FormatBit::BIT8 | FormatBit::SNORM;
		break;
	case VK_FORMAT_R8_USCALED:
		return FormatBit::R | FormatBit::BIT8 | FormatBit::USCALED;
		break;
	case VK_FORMAT_R8_SSCALED:
		return FormatBit::R | FormatBit::BIT8 | FormatBit::SSCALED;
		break;
	case VK_FORMAT_R8_SRGB:
		return FormatBit::R | FormatBit::BIT8 | FormatBit::SRGB;
		break;
	case VK_FORMAT_R8G8_UNORM:
		return FormatBit::RG | FormatBit::BIT8 | FormatBit::UNORM;
		break;
	case VK_FORMAT_R8G8_SNORM:
		return FormatBit::RG | FormatBit::BIT8 | FormatBit::SNORM;
		break;
	case VK_FORMAT_R8G8_USCALED:
		return FormatBit::RG | FormatBit::BIT8 | FormatBit::UNORM;
		break;
	case VK_FORMAT_R8G8_SSCALED:
		return FormatBit::RG | FormatBit::BIT8 | FormatBit::SSCALED;
		break;
	case VK_FORMAT_R8G8_UINT:
		return FormatBit::RG | FormatBit::BIT8 | FormatBit::UINT;
		break;
	case VK_FORMAT_R8G8_SINT:
		return FormatBit::RG | FormatBit::BIT8 | FormatBit::SINT;
		break;
	case VK_FORMAT_R8G8_SRGB:
		return FormatBit::RG | FormatBit::BIT8 | FormatBit::SRGB;
		break;
	case VK_FORMAT_R8G8B8_UNORM:
		return FormatBit::RGB | FormatBit::BIT8 | FormatBit::UNORM;
		break;
	case VK_FORMAT_R8G8B8_SNORM:
		return FormatBit::RGB | FormatBit::BIT8 | FormatBit::SNORM;
		break;
	case VK_FORMAT_R8G8B8_USCALED:
		return FormatBit::RGB | FormatBit::BIT8 | FormatBit::USCALED;
		break;
	case VK_FORMAT_R8G8B8_SSCALED:
		return FormatBit::RGB | FormatBit::BIT8 | FormatBit::SSCALED;
		break;
	case VK_FORMAT_R8G8B8_UINT:
		return FormatBit::RGB | FormatBit::BIT8 | FormatBit::UINT;
		break;
	case VK_FORMAT_R8G8B8_SINT:
		return FormatBit::RGB | FormatBit::BIT8 | FormatBit::SINT;
		break;
	case VK_FORMAT_R8G8B8_SRGB:
		return FormatBit::RGB | FormatBit::BIT8 | FormatBit::SRGB;
		break;
	case VK_FORMAT_R8G8B8A8_UINT:
		return FormatBit::RGBA | FormatBit::BIT8 | FormatBit::UINT;
		break;
	case VK_FORMAT_R8G8B8A8_SINT:
		return FormatBit::RGBA | FormatBit::BIT8 | FormatBit::SINT;
		break;
	case VK_FORMAT_R8G8B8A8_SRGB:
		return FormatBit::RGBA | FormatBit::BIT8 | FormatBit::SRGB;
		break;
	case VK_FORMAT_R8G8B8A8_UNORM:
		return FormatBit::RGBA | FormatBit::BIT8 | FormatBit::UNORM;
		break;
	case VK_FORMAT_R8G8B8A8_SNORM:
		return FormatBit::RGBA | FormatBit::BIT8 | FormatBit::SNORM;
		break;
	case VK_FORMAT_R16_SINT:
		return FormatBit::R | FormatBit::BIT16 | FormatBit::SINT;
		break;
	case VK_FORMAT_R16_UINT:
		return FormatBit::R | FormatBit::BIT16 | FormatBit::UINT;
		break;
	case VK_FORMAT_R16_UNORM:
		return FormatBit::R | FormatBit::BIT16 | FormatBit::UNORM;
		break;
	case VK_FORMAT_R16_SNORM:
		return FormatBit::R | FormatBit::BIT16 | FormatBit::SNORM;
		break;
	case VK_FORMAT_R16_USCALED:
		return FormatBit::R | FormatBit::BIT16 | FormatBit::USCALED;
		break;
	case VK_FORMAT_R16_SSCALED:
		return FormatBit::R | FormatBit::BIT16 | FormatBit::SSCALED;
		break;
	case VK_FORMAT_R16G16_UNORM:
		return FormatBit::RG | FormatBit::BIT16 | FormatBit::UNORM;
		break;
	case VK_FORMAT_R16G16_SNORM:
		return FormatBit::RG | FormatBit::BIT16 | FormatBit::SNORM;
		break;
	case VK_FORMAT_R16G16_USCALED:
		return FormatBit::RG | FormatBit::BIT16 | FormatBit::UNORM;
		break;
	case VK_FORMAT_R16G16_SSCALED:
		return FormatBit::RG | FormatBit::BIT16 | FormatBit::SSCALED;
		break;
	case VK_FORMAT_R16G16_UINT:
		return FormatBit::RG | FormatBit::BIT16 | FormatBit::UINT;
		break;
	case VK_FORMAT_R16G16_SINT:
		return FormatBit::RG | FormatBit::BIT16 | FormatBit::SINT;
		break;
	case VK_FORMAT_R16G16B16_UNORM:
		return FormatBit::RGB | FormatBit::BIT16 | FormatBit::UNORM;
		break;
	case VK_FORMAT_R16G16B16_SNORM:
		return FormatBit::RGB | FormatBit::BIT16 | FormatBit::SNORM;
		break;
	case VK_FORMAT_R16G16B16_USCALED:
		return FormatBit::RGB | FormatBit::BIT16 | FormatBit::USCALED;
		break;
	case VK_FORMAT_R16G16B16_SSCALED:
		return FormatBit::RGB | FormatBit::BIT16 | FormatBit::SSCALED;
		break;
	case VK_FORMAT_R16G16B16_UINT:
		return FormatBit::RGB | FormatBit::BIT16 | FormatBit::UINT;
		break;
	case VK_FORMAT_R16G16B16_SINT:
		return FormatBit::RGB | FormatBit::BIT16 | FormatBit::SINT;
		break;
	case VK_FORMAT_R16G16B16A16_UINT:
		return FormatBit::RGBA | FormatBit::BIT16 | FormatBit::UINT;
		break;
	case VK_FORMAT_R16G16B16A16_SINT:
		return FormatBit::RGBA | FormatBit::BIT16 | FormatBit::SINT;
		break;
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		return FormatBit::RGBA | FormatBit::BIT16 | FormatBit::SFLOAT;
		break;
	case VK_FORMAT_R32_SINT:
		return FormatBit::R | FormatBit::BIT32 | FormatBit::SINT;
		break;
	case VK_FORMAT_R32_UINT:
		return FormatBit::R | FormatBit::BIT32 | FormatBit::UINT;
		break;
	case VK_FORMAT_R32G32_UINT:
		return FormatBit::RG | FormatBit::BIT32 | FormatBit::UINT;
		break;
	case VK_FORMAT_R32G32_SINT:
		return FormatBit::RG | FormatBit::BIT32 | FormatBit::SINT;
		break;
	case VK_FORMAT_R32G32B32_UINT:
		return FormatBit::RGB | FormatBit::BIT32 | FormatBit::UINT;
		break;
	case VK_FORMAT_R32G32B32_SINT:
		return FormatBit::RGB | FormatBit::BIT32 | FormatBit::SINT;
		break;
	case VK_FORMAT_R32G32B32A32_UINT:
		return FormatBit::RGBA | FormatBit::BIT32 | FormatBit::UINT;
		break;
	case VK_FORMAT_R32G32B32A32_SINT:
		return FormatBit::RGBA | FormatBit::BIT32 | FormatBit::SINT;
		break;
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return FormatBit::RGBA | FormatBit::BIT32 | FormatBit::SFLOAT;
		break;
	case VK_FORMAT_D32_SFLOAT:
		return FormatBit::D | FormatBit::BIT32 | FormatBit::SFLOAT;
		break;
	default:
		return FormatBit::UNDEFINED;
		break;
	}
}

inline const struct AddressTypes {
	VkSamplerAddressMode repeat = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode mirrored_repeat = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	VkSamplerAddressMode clamp_to_edge = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VkSamplerAddressMode mirror_clamp_to_edge = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	VkSamplerAddressMode clamp_to_border = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
}AddressTypes;

inline const struct MipmapModes {
	VkSamplerMipmapMode nearest = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	VkSamplerMipmapMode linear = VK_SAMPLER_MIPMAP_MODE_LINEAR;
}MipmapModes;

inline const struct FilterTypes {
	VkFilter nearest = VK_FILTER_NEAREST;
	VkFilter linear = VK_FILTER_LINEAR;
	VkFilter cubic_img = VK_FILTER_CUBIC_IMG;
}FilterTypes;

inline std::string readFile(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file: " + filePath);
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();
	return buffer.str();
}

inline nlohmann::json readJson(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file: " + filePath);
	}
	nlohmann::json j;
	file >> j;
	return j;
}

inline std::vector<char> readSpv(const std::string& filePath) {
	std::ifstream file{ filePath, std::ios::ate | std::ios::binary };

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file: " + filePath);
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}

inline bool doesFileExist(const std::string& name) {
	if (FILE* file = fopen(name.c_str(), "r")) {
		fclose(file);
		return true;
	}
	else {
		return false;
	}
}

inline void* alignedAlloc(size_t size, size_t alignment) {
#if defined(_MSC_VER) || defined(__MINGW32__)
	return _aligned_malloc(size, alignment);
#else 
	void* data = nullptr;
	if (posix_memalign(&data, alignment, size) == 0)
		return data;
	return nullptr
#endif
}

inline void alignedFree(void* data) {
#if	defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else
	free(data);
#endif
}

inline void paddingInline(std::string& str, const size_t& length) {
	assert(str.size() <= length && "Error: String length cannot be longer than padding length.");
	const size_t offset = length - str.size();
	const size_t left = offset / 2;
	const size_t right = offset - left;
	str.insert(str.begin(), left, ' ');
	str.insert(str.end(), right, ' ');
}

inline void rightPaddingInline(std::string& str, const size_t& length) {
	assert(str.size() <= length && "Error: String length cannot be longer than padding length.");
	const size_t offset = length - str.size();
	str.insert(str.end(), offset, ' ');
}

inline void leftPaddingInline(std::string& str, const size_t& length) {
	assert(str.size() <= length && "Error: String length cannot be longer than padding length.");
	const size_t offset = length - str.size();
	str.insert(str.begin(), offset, ' ');
}

inline std::string padding(std::string str, const size_t& length) {
	assert(str.size() <= length && "Error: String length cannot be longer than padding length.");
	paddingInline(str, length);
	return str;
}

inline std::string rightPadding(std::string str, const size_t& length) {
	assert(str.size() <= length && "Error: String length cannot be longer than padding length.");
	rightPaddingInline(str, length);
	return str;
}

inline std::string leftPadding(std::string str, const size_t& length) {
	assert(str.size() <= length && "Error: String length cannot be longer than padding length.");
	leftPaddingInline(str, length);
	return str;
}

inline bool strEndWith(const std::string& str, const std::string& tail) {
	return str.compare(str.size() - tail.size(), tail.size(), tail) == 0;
}

inline bool strStartWith(const std::string& str, const std::string& head) {
	return str.compare(0, head.size(), head) == 0;
}

inline std::string getFileName(const std::string& str) {
	int i = str.size() - 1;
	for (; i != 0; i--) {
		if (str[i] == '/')
			break;
	}
	if (str[i] == '/') i++;
	return str.substr(i, str.size() - i);
}

template<typename T>
inline void overwriteVector(std::vector<T>& target, std::vector<T> const& source, size_t beginIdx = 0)
{
	size_t end = std::min(target.size(), source.size() + beginIdx);
	size_t i = 0;

	while (beginIdx < end)
		target[beginIdx++] = source[i++];
}

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> _wstring2charConverter{};

inline std::string wstr2u8str(const std::wstring& string) {
	return _wstring2charConverter.to_bytes(string);
}

inline std::wstring u8str2wstr(const std::string& string) {
	return _wstring2charConverter.from_bytes(string);
}

inline void hashCombine(std::size_t& seed) { }

template <typename T, typename... Rest>
inline void hashCombine(std::size_t& seed, const T& v, Rest... rest) {
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	hashCombine(seed, rest...);
}

template<typename T>
inline bool isZero(const T& num) {
	return abs(num) <= std::numeric_limits<T>::epsilon();
}

template <typename T>
inline std::vector<T> reverseVector(const std::vector<T>& vec) {
	std::vector<T> vec2;
	vec2.reserve(vec.size());
	for (auto it = vec.end() - 1; it != vec.begin(); it--) {
		vec2.push_back(*it);
	}
	vec2.push_back(*vec.begin());
	return std::move(vec2);
}

}

#endif