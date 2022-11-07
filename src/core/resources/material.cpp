#include "resources/material.hpp"


#include <cassert>
#include <iostream>
#include <exception>

namespace naku { //trexture

Texture::Texture(
	Device& device,
	const std::string& name,
	std::shared_ptr<Image2D> pImage,
	bool anisotropic_filtering,
	VkFilter filterType,
	VkSamplerAddressMode addresType,
	VkSamplerMipmapMode mipmapMode)
	: _device{ device }, _name{ name }, _pImage{ pImage }, _anisotropic_filtering{ anisotropic_filtering }, _filterType{ filterType }, _addressType{ addresType }, _mipmapMode{ mipmapMode }{
	auto createInfo = Sampler::getDefaultSamplerCreateInfo();
	createInfo.maxAnisotropy = _device.properties.limits.maxSamplerAnisotropy;
	createInfo.magFilter = _filterType;
	createInfo.minFilter = _filterType;
	createInfo.addressModeU = _addressType;
	createInfo.addressModeV = _addressType;
	createInfo.addressModeW = _addressType;

	if (_anisotropic_filtering)
		createInfo.maxLod = static_cast<float>(pImage->_mipLevels);
	else createInfo.maxLod = 1;
	createInfo.mipmapMode = _mipmapMode;
	_pSampler = std::make_unique<Sampler>(device, createInfo);
}

Texture::Texture(
	Device& device,
	const std::string& name,
	std::shared_ptr<Image2D> pImage,
	VkSamplerCreateInfo samplerInfo)
	: _device{ device }, _name{ name }, _pImage{ pImage } {
	_pSampler = std::make_unique<Sampler>(device, samplerInfo);
}

Texture::~Texture() {}

VkDescriptorImageInfo Texture::descriptorInfo() const {
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = _pImage->_defaultImageView;
	imageInfo.sampler = _pSampler->sampler();

	return imageInfo;
}


}

namespace naku { //material

std::shared_ptr<Texture> Material::DefaultTexture = nullptr;
std::shared_ptr<Image2D> Material::DefaultTexutreImage = nullptr;
std::shared_ptr<DescriptorSetLayout> Material::textureInputSetLayout = nullptr;
size_t Material::_instanceCount = 0;

Material::Material(
	Type type,
	Device& device,
	const std::string name,
	std::shared_ptr<Shader> pVertShader,
	std::shared_ptr<Shader> pFragShader,
	DescriptorPool& descriptorPool)
	: Resource{ device, name }, _type { type }, _vertShader{pVertShader}, _fragShader{pFragShader} {
	if (!pVertShader || !pFragShader)
		throw std::runtime_error("Error: Shader is null pointer.");

	pushConstants.mtlId = Resource::_id;

	GraphicsPipeline::defaultPipelineConfig(_config);
	_config.vertStageCreateInfo = _vertShader->getCreateInfo();
	_config.fragStageCreateInfo = _fragShader->getCreateInfo();

	if (_instanceCount == 0) {
		//DefaultTexutreImage = std::make_shared<Image2D>(_device, "_default_image", "res/texture/default.png");
		DefaultTexutreImage = Image2D::loadImageFromFile(_device, "_default_image", "res/texture/default.png");
		DefaultTexture = std::make_shared<Texture>(_device, "_default_texture", DefaultTexutreImage, true);

		DescriptorSetLayout::Builder layoutBuilder(_device);
		for (uint32_t i = 0; i < static_cast<uint32_t>(MaterialTextures::SIZE); i++)
			layoutBuilder.addBinding(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		textureInputSetLayout = layoutBuilder.build();
	}
	_instanceCount++;

	// textures
	for (size_t i = 0; i < _textures.size(); i++) {
		_textures[i] = DefaultTexture;
	}

	_writer = std::make_unique<DescriptorWriter>(*textureInputSetLayout, descriptorPool);
	if (!_writer->build(_set)) {
		throw std::runtime_error("Error: Failed to create descriptor set.");
	}
	update(true);
}

void Material::changeTexture(size_t binding, std::shared_ptr<Texture> newTex, bool Update) {
	_textures[binding] = newTex;
	pushConstants.hasTexture = pushConstants.hasTexture | (1 << binding);
	this->update(Update);
}

void Material::removeTexture(size_t binding, bool Update) {
	_textures[binding] = DefaultTexture;
	pushConstants.hasTexture = pushConstants.hasTexture & ~(1 << binding);
	this->update(Update);
}

Material::~Material() {
	if (--_instanceCount == 0) {
		if (DefaultTexture)
			DefaultTexture.reset();
		if (DefaultTexutreImage)
			DefaultTexutreImage.reset();
		if (textureInputSetLayout)
			textureInputSetLayout.reset();
	}
}

void Material::update(bool overwrite) {
	for (uint32_t binding = 0; binding < textureInputSetLayout->bindings.size(); binding ++) {
		auto pTexture = _textures[binding];
		_writer->writeImage(binding, pTexture->descriptorInfo());
	}
	if (overwrite) _writer->overwrite(_set);
}

void Material::cmdBindSet(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, uint32_t firstset) {
	vkCmdBindDescriptorSets(
		cmd,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		firstset,
		1,
		&_set,
		0,
		nullptr);
}

}