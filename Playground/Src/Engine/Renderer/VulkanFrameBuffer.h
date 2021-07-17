#pragma once

#include "vulkan/vulkan.h"

class VulkanDevice;
class VulkanSwapChain;


class VulkanFrameBuffer
{
public:
	VulkanFrameBuffer();
	~VulkanFrameBuffer();

	void							CreateAttachment(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain);
	void							CreateFrameBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain, VkRenderPass renderPass);

	void							Cleanup(VulkanDevice* pDevice);
	void							CleanupOnWindowResize(VulkanDevice* pDevice);

public:
	VkFormat						m_attachmentFormat;
	std::vector<VkImage>			m_vecAttachmentImage;					// Size equals to number of swapchain images!
	std::vector<VkImageView>		m_vecAttachmentImageView;				// Size equals to number of swapchain images!
	std::vector<VkDeviceMemory>		m_vecAttachmentImageMemory;				// Size equals to number of swapchain images!
	std::vector<VkFramebuffer>		m_vecFramebuffer;						// Size equals to number of swapchain images!

private:

	VkFormat						ChooseSupportedFormats(	VulkanDevice* pDevice, const std::vector<VkFormat>& formats,
															VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
};

