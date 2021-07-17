#pragma once

#include "vulkan/vulkan.h"

class VulkanDevice;
class VulkanSwapChain;

// **** Type of Framebuffer Attachment
enum class AttachmentType
{
	FB_ATTACHMENT_UNDEFINED,
	FB_ATTACHMENT_ALBEDO,
	FB_ATTACHMENT_POSITION,
	FB_ATTACHMENT_NORMAL,
	FB_ATTACHMENT_DEPTH,
	FB_ATTACHMENT_PBR,				// Metallic, Roughness, AO
	FB_ATTACHMENT_EMISSION,
	FB_ATTACHMENT_BACKGROUND,
	FB_ATTACHMENT_OBJECTID
};

// **** Inidvidual Framebuffer attachment
struct FramebufferAttachment
{
	FramebufferAttachment()
	{
		attachmentFormat = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
		vecAttachmentImage.clear();
		vecAttachmentImageView.clear();
		vecAttachmentImageMemory.clear();

		attachmentType = AttachmentType::FB_ATTACHMENT_UNDEFINED;
	}

	void	Cleanup(VulkanDevice* pDevice);
	void	CleanupOnWindowResize(VulkanDevice* pDevice);
	
	VkFormat						attachmentFormat;
	std::vector<VkImage>			vecAttachmentImage;					// Size equals to number of swapchain images!
	std::vector<VkImageView>		vecAttachmentImageView;				// Size equals to number of swapchain images!
	std::vector<VkDeviceMemory>		vecAttachmentImageMemory;			// Size equals to number of swapchain images!

	AttachmentType					attachmentType;
};

// **** Framebuffer Class
// ** Holds list of attchments & it's type. 
class DeferredFrameBuffer
{
public:
	DeferredFrameBuffer();
	~DeferredFrameBuffer();

	void								CreateAttachment(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain, AttachmentType eType);
	void								CreateFrameBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain, VkRenderPass renderPass);

	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

private:
	VkFormat							ChooseSupportedFormats(VulkanDevice* pDevice, const std::vector<VkFormat>& formats,
																VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	std::vector<VkImageView>			m_vecAttachments;

public:
	FramebufferAttachment*				m_pAlbedoAttachment;
	FramebufferAttachment*				m_pDepthAttachment;
	FramebufferAttachment*				m_pNormalAttachment;
	FramebufferAttachment*				m_pPositionAttachment;
	FramebufferAttachment*				m_pPBRAttachment;
	FramebufferAttachment*				m_pEmissionAttachment;
	FramebufferAttachment*				m_pBackgroundAttachment;
	FramebufferAttachment*				m_pObjectIDAttachment;

	std::vector<VkFramebuffer>			m_vecFramebuffers;				// Size equals to number of swapchain images
};

