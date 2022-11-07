#include "render_systems/opaque_renderer.hpp"
#include "utils/engine.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <cassert>
#include <stdexcept>
#include <iostream>

namespace naku {

OpaqueRenderer::OpaqueRenderer(
    Engine& engine,
    RenderPass& renderPass,
    uint32_t subpass,
    std::vector<FrameBufferAttachment*> inputGbufferAttachments,
    std::vector<VkImageLayout> inputGbufferLayouts)
    : _engine{ engine },
    _device{ *engine.pDevice },
    _renderPass{ renderPass },
    _resources{ _engine.resources },
    _subpass{ renderPass.subpasses[subpass] },
    _imageCount{ renderPass.imageCount() } {

    if (_engine.resources.exist<Shader>("opaque.vert.spv"))
        _vert = _engine.resources.get<Shader>("opaque.vert.spv");
    else
        _vert = _engine.createShader("res/shader/opaque.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    if (_engine.resources.exist<Shader>("opaque.frag.spv"))
        _frag = _engine.resources.get<Shader>("opaque.frag.spv");
    else
        _frag = _engine.createShader("res/shader/opaque.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    auto samplerCreateInfo = Sampler::getDefaultSamplerCreateInfo();
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    _sampler = std::make_unique<Sampler>(_device, samplerCreateInfo);

    // create descriptor set
    {
        DescriptorSetLayout::Builder layoutBuilder(_device);
        for (uint32_t i = 0; i < inputGbufferAttachments.size(); i++) 
            layoutBuilder.addBinding(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        layoutBuilder.addBinding(inputGbufferAttachments.size(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); //shadowmap
        layoutBuilder.addBinding(inputGbufferAttachments.size()+1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); //shadowmap
        _setLayout = layoutBuilder.build();
        _writers.reserve(_imageCount);
        for (size_t i = 0; i < _imageCount; i++) {
            auto writer = std::make_unique<DescriptorWriter>(*_setLayout, *_engine.pDescriptorSetPool);
            for (uint32_t ii = 0; ii < inputGbufferAttachments.size(); ii++) {
                writer->writeImage(ii, { _sampler->sampler(), inputGbufferAttachments[ii]->views[i], inputGbufferLayouts[ii] });
            }

            // shadowmap doesn't have triple buffering
            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            imgInfo.imageView = Light::normalShadowmaps->defaultImageView(); 
            imgInfo.sampler = _sampler->sampler();
            writer->writeImage(inputGbufferAttachments.size(), imgInfo);

            imgInfo.imageView = Light::omniShadowmaps->defaultImageView();
            writer->writeImage(inputGbufferAttachments.size()+1, imgInfo);

            _writers.push_back(std::move(writer));
        }
        _sets.resize(_imageCount);
        for (size_t i = 0; i < _imageCount; i++) {
            _writers[i]->build(_sets[i]);
        }
    }
}

OpaqueRenderer::~OpaqueRenderer() {
    vkDestroyPipelineLayout(device(), _pipelineLayout, nullptr);
}

void OpaqueRenderer::createPipeline() {
    GraphicsPipeline::defaultPipelineConfig(_config);
    _config.renderPass = _renderPass.renderPass();
    _config.subpass = _subpass.subpass;
    _config.vertStageCreateInfo = _vert->getCreateInfo();
    _config.fragStageCreateInfo = _frag->getCreateInfo();
    static std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(1);
    GraphicsPipeline::defaultBlending(colorBlendAttachments);
    _config.colorBlendInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    _config.colorBlendInfo.pAttachments = colorBlendAttachments.data();
    _config.bindingDescriptions.clear();
    _config.attributeDescriptions.clear();

    std::array<VkDescriptorSetLayout, 2> descriptorLayouts{ _engine.pGlobalSetLayout->getDescriptorSetLayout(), _setLayout->getDescriptorSetLayout()};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();

    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("Error: Failed to create pipeline layout!");
    }

    _config.pipelineLayout = _pipelineLayout;

    _pipeline = std::make_unique<GraphicsPipeline>(_device, _config);
}

void OpaqueRenderer::render(FrameInfo frameInfo) {
    _pipeline->cmdBind(frameInfo.commandBuffer);

    auto sets = frameInfo.globalSets;
    sets.push_back(_sets[frameInfo.imageIndex]);
    
    static const std::array<uint32_t, 1> offsets{ 0 }; // offset doesn't matter.

    auto& Lights = _resources.getResource<Light>();
    auto light = Lights[0];
    vkCmdBindDescriptorSets(
        frameInfo.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _pipelineLayout,
        0,
        static_cast<uint32_t>(sets.size()),
        sets.data(),
        static_cast<uint32_t>(offsets.size()),
        offsets.data());
    vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
}

}