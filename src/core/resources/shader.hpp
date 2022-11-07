#ifndef SHADER_HPP
#define SHADER_HPP

#include "naku.hpp"
#include "utils/device.hpp"
#include "utils/buffer.hpp"
#include "utils/pipeline.hpp"
#include "resources/resource.hpp"

namespace naku {
//a shader holds all of the shader related state that a pipeline needs to be built.
class Shader : public Resource {
public:
	Shader(
		Device& device,
		const std::string name,
		const std::string& filePath,
		VkShaderStageFlagBits stage);
	~Shader();
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;
	Shader() = default;

	VkDevice device() const { return _device.device(); }
	VkShaderStageFlagBits stage() const { return _stage; }
	VkShaderModule module() const { return _module; }
	VkPipelineShaderStageCreateInfo getCreateInfo();

	friend class ShaderPass;
	friend class MaterialBase;
	friend class Material;

private:
	VkShaderModule _module;
	VkShaderStageFlagBits _stage;
};
}

#endif