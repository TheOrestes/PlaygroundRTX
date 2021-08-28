#pragma once

#include "vulkan/vulkan.h"

class VulkanDevice;
class VulkanSwapChain;

enum class PipelineType
{
	GBUFFER_OPAQUE,
	HDRI_SKYDOME,
	DEFERRED
};

struct PipelineConfigInfo
{
	VkViewport											viewport;
	VkRect2D											scissorRect;
	VkPipelineViewportStateCreateInfo					viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo				inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo				rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo				multisampleInfo;
	std::vector<VkPipelineColorBlendAttachmentState>	vecColorBlendAttachments;
	VkPipelineColorBlendStateCreateInfo					colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo				depthStencilInfo;
	VkPipelineLayout									pipelineLayout		= nullptr;
	VkRenderPass										renderPass			= nullptr;
	uint32_t											subpass				= 0;
};

class VulkanGraphicsPipeline
{
public:
	VulkanGraphicsPipeline(PipelineType type, VulkanSwapChain* pSwapChain);
	~VulkanGraphicsPipeline();

	void												CreatePipelineLayout(VulkanDevice* pDevice, const std::vector<VkDescriptorSetLayout>& layouts, 
																			const std::vector<VkPushConstantRange> pushConstantRanges);
														
	void												CreateGraphicsPipeline(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, 
																				VkRenderPass renderPass, uint32_t subPass, uint32_t nOutputAttachments);
														
	void												Cleanup(VulkanDevice* pDevice);
	void												CleanupOnWindowResize(VulkanDevice* pDevice);
														
public:													
	VkPipeline											m_vkGraphicsPipeline;
	VkPipelineLayout									m_vkPipelineLayout;
														
private:												
	void												CreateDefaultPipelineConfigInfo(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, 
																						VkRenderPass renderPass, uint32_t subPass, VkPipelineLayout layout,
																						PipelineConfigInfo& outInfo);
														
private:												
	PipelineType										m_eType;
	//PipelineConfigInfo									m_sPipelineConfigInfo;
														
	std::string											m_strVertexShader;
	std::string											m_strFragmentShader;
};

