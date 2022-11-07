#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "naku.hpp"
#include "resources/resource.hpp"
#include "resources/shader.hpp"
#include "resources/image.hpp"
#include "utils/swap_chain.hpp"
#include "utils/descriptors.hpp"
#include "utils/buffer.hpp"
#include "utils/pipeline.hpp"

namespace naku {

class Texture {
public:

	Texture(
		Device& device,
		const std::string& name,
		std::shared_ptr<Image2D> pImage,
		bool anisotropic_filtering = false,
		VkFilter filterType = VK_FILTER_LINEAR,
		VkSamplerAddressMode addresType = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR);
	Texture(
		Device& device,
		const std::string& name,
		std::shared_ptr<Image2D> pImage,
		VkSamplerCreateInfo samplerInfo);
	~Texture();
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
	Texture() = default;

	VkDescriptorImageInfo descriptorInfo() const;
	std::string name() const { return _name; }
	std::string fileName() const { return _pImage->_name; }
	std::string filePath() const { return _pImage->filePath(); }

	friend class GUI;
	friend class Scene;

private:
	Device& _device;
	std::string _name;
	std::shared_ptr<Image2D> _pImage;
	bool _anisotropic_filtering;
	std::unique_ptr<Sampler> _pSampler;
	VkSamplerAddressMode _addressType;
	VkFilter _filterType;
	VkSamplerMipmapMode _mipmapMode;
};

class Material : public Resource {
// textures are stored in uniform buffer
// geometry infos are stored in push constant values

public:
	enum Type {
		NULL_TYPE = 0,
		OPAQUE = 1,
		TRANSPARENT = 2,
		EMITTER = 3,
		ERROR = 0xFFFFFFFF
	};

	struct PushConstants {
		alignas(16) glm::vec4 albedo{ 1.f };
		alignas(16) glm::vec4 emission{ 1.f, 1.f, 1.f, 0.f };
		alignas(16) glm::vec4 offsetTilling{ 0.f, 0.f, 1.f, 1.f };
		float metalness{ 0.f };
		float roughness{ 1.f };
		float ior{ 1.f };
		int side{ -1 }; // 0: front, 1: back, -1: double
		int alphaMode{ 0 };
		int hasTexture;
		int mtlId;
	};

	static std::shared_ptr<DescriptorSetLayout> textureInputSetLayout;

	Type type() const { return _type; }
	Material(
		Type type,
		Device& device,
		const std::string name,
		std::shared_ptr<Shader> pVertShader,
		std::shared_ptr<Shader> pFragShader,
		DescriptorPool& descriptorPool);
	~Material();
	Material(const Material&) = delete;
	Material& operator=(const Material&) = delete;
	Material() = default;
	
	friend class GUI;
	friend class Scene;

	PushConstants pushConstants{};
	void allocateSet(DescriptorPool& descriptorPool);
	void cmdBindSet(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, uint32_t firstset = 0);
	void update(size_t set, bool overwrite=true);
	void update(bool overwrite=true);

	void changeTexture(size_t binding, std::shared_ptr<Texture> newTex, bool update = true);
	void removeTexture(size_t binding, bool update = true);

	PipelineConfig& getConfig() { return _config; }
	VkDescriptorSet getDescriptorSet() const{ return _set; }
	
	static std::shared_ptr<Texture> DefaultTexture;
	static std::shared_ptr<Image2D> DefaultTexutreImage;

private:
	Type _type;
	std::shared_ptr<Shader> _vertShader;
	std::shared_ptr<Shader> _fragShader;
	PipelineConfig _config{};

	std::array<std::shared_ptr<Texture>, static_cast<size_t>(MaterialTextures::SIZE)> _textures;
	std::unique_ptr<DescriptorWriter> _writer;
	VkDescriptorSet _set;

	static size_t _instanceCount;
};

}

#endif