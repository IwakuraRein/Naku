#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "naku.hpp"
#include "utils/device.hpp"
#include "resources/mesh.hpp"

namespace naku {
struct PipelineConfig {
	PipelineConfig(const PipelineConfig&) = delete;
	PipelineConfig() = default;
	PipelineConfig& operator=(const PipelineConfig&) = delete;

	std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineViewportStateCreateInfo viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	std::vector<VkDynamicState> dynamicStateEnables;
	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
	VkPipelineLayout pipelineLayout = nullptr;
	VkRenderPass renderPass = nullptr;
	VkPipelineShaderStageCreateInfo vertStageCreateInfo;
	VkPipelineShaderStageCreateInfo fragStageCreateInfo;

	uint32_t subpass = 0;
};

class GraphicsPipeline {
public:
	GraphicsPipeline(
		Device& _device,
		const PipelineConfig& config);
	~GraphicsPipeline();
	GraphicsPipeline(const GraphicsPipeline&) = delete;
	GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
	GraphicsPipeline() = default;

	VkDevice device() const { return _device.device(); };

	static void defaultPipelineConfig(PipelineConfig& config);
	static void enableAlphaBlending(std::vector<VkPipelineColorBlendAttachmentState>& states);
	static void defaultBlending(std::vector<VkPipelineColorBlendAttachmentState>& states);
	static void enableAlphaBlending(VkPipelineColorBlendAttachmentState& state);
	static void defaultBlending(VkPipelineColorBlendAttachmentState& state);

	void cmdBind(VkCommandBuffer commandBuffer);
private:
	Device& _device;
	VkPipeline _pipeline;
	void createGraphicsPipeline(const PipelineConfig & config);
};
class VklRayTracingPipeline {};
}

#endif