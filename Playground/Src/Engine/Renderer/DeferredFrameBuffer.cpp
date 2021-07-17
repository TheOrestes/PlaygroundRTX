#include "PlaygroundPCH.h"
#include "DeferredFrameBuffer.h"

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "Engine/Helpers/Log.h"
#include "Engine/Helpers/Utility.h"
#include "PlaygroundHeaders.h"

//---------------------------------------------------------------------------------------------------------------------
DeferredFrameBuffer::DeferredFrameBuffer()
{
	m_pAlbedoAttachment		= nullptr;
	m_pDepthAttachment		= nullptr;
	m_pNormalAttachment		= nullptr;
	m_pPositionAttachment	= nullptr;
	m_pEmissionAttachment	= nullptr;
	m_pPBRAttachment		= nullptr;
	m_pBackgroundAttachment = nullptr;
	m_pObjectIDAttachment	= nullptr;

	m_vecFramebuffers.clear();
	m_vecAttachments.resize(8);			// Swapchain Image + 7 Attachments!
}

//---------------------------------------------------------------------------------------------------------------------
DeferredFrameBuffer::~DeferredFrameBuffer()
{
	SAFE_DELETE(m_pAlbedoAttachment);
	SAFE_DELETE(m_pDepthAttachment);
	SAFE_DELETE(m_pNormalAttachment);
	SAFE_DELETE(m_pPositionAttachment);
	SAFE_DELETE(m_pBackgroundAttachment);
	SAFE_DELETE(m_pObjectIDAttachment);
	SAFE_DELETE(m_pEmissionAttachment);
	SAFE_DELETE(m_pPBRAttachment);

	m_vecFramebuffers.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void DeferredFrameBuffer::CreateAttachment(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain, AttachmentType eType)
{
	switch (eType)
	{
		case AttachmentType::FB_ATTACHMENT_ALBEDO:
		{
			m_pAlbedoAttachment = new FramebufferAttachment();

			m_pAlbedoAttachment->vecAttachmentImage.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pAlbedoAttachment->vecAttachmentImageView.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pAlbedoAttachment->vecAttachmentImageMemory.resize(pSwapChain->m_vecSwapchainImages.size());

			std::vector<VkFormat> formats = { VK_FORMAT_B8G8R8A8_UNORM };
			m_pAlbedoAttachment->attachmentFormat = ChooseSupportedFormats(pDevice, formats, VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);


			for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
			{
				// Create color buffer image
				m_pAlbedoAttachment->vecAttachmentImage[i] = Helper::Vulkan::CreateImage(	pDevice,
																							pSwapChain->m_vkSwapchainExtent.width,
																							pSwapChain->m_vkSwapchainExtent.height,
																							m_pAlbedoAttachment->attachmentFormat,
																							VK_IMAGE_TILING_OPTIMAL,
																							VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
																							VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
																							&(m_pAlbedoAttachment->vecAttachmentImageMemory[i]));

				// Create color buffer image view!
				m_pAlbedoAttachment->vecAttachmentImageView[i] = Helper::Vulkan::CreateImageView(	pDevice,
																									m_pAlbedoAttachment->vecAttachmentImage[i],
																									m_pAlbedoAttachment->attachmentFormat,
																									VK_IMAGE_ASPECT_COLOR_BIT);
			}

			break;
		}
			
		case AttachmentType::FB_ATTACHMENT_POSITION:
		{

			m_pPositionAttachment = new FramebufferAttachment();

			m_pPositionAttachment->vecAttachmentImage.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pPositionAttachment->vecAttachmentImageView.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pPositionAttachment->vecAttachmentImageMemory.resize(pSwapChain->m_vecSwapchainImages.size());

			std::vector<VkFormat> formats = { VK_FORMAT_B8G8R8A8_UNORM };
			m_pPositionAttachment->attachmentFormat = ChooseSupportedFormats(pDevice, formats, VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);


			for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
			{
				// Create color buffer image
				m_pPositionAttachment->vecAttachmentImage[i] = Helper::Vulkan::CreateImage(pDevice,
					pSwapChain->m_vkSwapchainExtent.width,
					pSwapChain->m_vkSwapchainExtent.height,
					m_pPositionAttachment->attachmentFormat,
					VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					&(m_pPositionAttachment->vecAttachmentImageMemory[i]));

				// Create color buffer image view!
				m_pPositionAttachment->vecAttachmentImageView[i] = Helper::Vulkan::CreateImageView(pDevice,
					m_pPositionAttachment->vecAttachmentImage[i],
					m_pPositionAttachment->attachmentFormat,
					VK_IMAGE_ASPECT_COLOR_BIT);
			}

			break;
		}
			

		case AttachmentType::FB_ATTACHMENT_NORMAL:
		{
			m_pNormalAttachment = new FramebufferAttachment();

			m_pNormalAttachment->vecAttachmentImage.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pNormalAttachment->vecAttachmentImageView.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pNormalAttachment->vecAttachmentImageMemory.resize(pSwapChain->m_vecSwapchainImages.size());

			std::vector<VkFormat> formats = { VK_FORMAT_B8G8R8A8_UNORM };
			m_pNormalAttachment->attachmentFormat = ChooseSupportedFormats(pDevice, formats, VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);


			for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
			{
				// Create Normal buffer image
				m_pNormalAttachment->vecAttachmentImage[i] = Helper::Vulkan::CreateImage(pDevice,
					pSwapChain->m_vkSwapchainExtent.width,
					pSwapChain->m_vkSwapchainExtent.height,
					m_pNormalAttachment->attachmentFormat,
					VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					&(m_pNormalAttachment->vecAttachmentImageMemory[i]));

				// Create normal buffer image view!
				m_pNormalAttachment->vecAttachmentImageView[i] = Helper::Vulkan::CreateImageView(pDevice,
					m_pNormalAttachment->vecAttachmentImage[i],
					m_pNormalAttachment->attachmentFormat,
					VK_IMAGE_ASPECT_COLOR_BIT);
			}

			break;
		}
			
		case AttachmentType::FB_ATTACHMENT_DEPTH:
		{
			m_pDepthAttachment = new FramebufferAttachment();

			m_pDepthAttachment->vecAttachmentImage.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pDepthAttachment->vecAttachmentImageView.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pDepthAttachment->vecAttachmentImageMemory.resize(pSwapChain->m_vecSwapchainImages.size());

			std::vector<VkFormat> formats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };
			m_pDepthAttachment->attachmentFormat = ChooseSupportedFormats(pDevice, formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

			for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
			{
				// Create color buffer image
				m_pDepthAttachment->vecAttachmentImage[i] = Helper::Vulkan::CreateImage(pDevice,
					pSwapChain->m_vkSwapchainExtent.width,
					pSwapChain->m_vkSwapchainExtent.height,
					m_pDepthAttachment->attachmentFormat,
					VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					&(m_pDepthAttachment->vecAttachmentImageMemory[i]));

				// Create color buffer image view!
				m_pDepthAttachment->vecAttachmentImageView[i] = Helper::Vulkan::CreateImageView(pDevice,
					m_pDepthAttachment->vecAttachmentImage[i],
					m_pDepthAttachment->attachmentFormat,
					VK_IMAGE_ASPECT_DEPTH_BIT);
			}

			break;
		}
			
		case AttachmentType::FB_ATTACHMENT_PBR:
		{
			m_pPBRAttachment = new FramebufferAttachment();

			m_pPBRAttachment->vecAttachmentImage.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pPBRAttachment->vecAttachmentImageView.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pPBRAttachment->vecAttachmentImageMemory.resize(pSwapChain->m_vecSwapchainImages.size());

			std::vector<VkFormat> formats = { VK_FORMAT_B8G8R8A8_UNORM };
			m_pPBRAttachment->attachmentFormat = ChooseSupportedFormats(pDevice, formats, VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);


			for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
			{
				// Create Normal buffer image
				m_pPBRAttachment->vecAttachmentImage[i] = Helper::Vulkan::CreateImage(	pDevice,
																						pSwapChain->m_vkSwapchainExtent.width,
																						pSwapChain->m_vkSwapchainExtent.height,
																						m_pPBRAttachment->attachmentFormat,
																						VK_IMAGE_TILING_OPTIMAL,
																						VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
																						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
																						&(m_pPBRAttachment->vecAttachmentImageMemory[i]));

				// Create normal buffer image view!
				m_pPBRAttachment->vecAttachmentImageView[i] = Helper::Vulkan::CreateImageView(	pDevice,
																								m_pPBRAttachment->vecAttachmentImage[i],
																								m_pPBRAttachment->attachmentFormat,
																								VK_IMAGE_ASPECT_COLOR_BIT);
			}
				
			break;
		}

		case AttachmentType::FB_ATTACHMENT_EMISSION:
		{
			m_pEmissionAttachment = new FramebufferAttachment();

			m_pEmissionAttachment->vecAttachmentImage.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pEmissionAttachment->vecAttachmentImageView.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pEmissionAttachment->vecAttachmentImageMemory.resize(pSwapChain->m_vecSwapchainImages.size());

			std::vector<VkFormat> formats = { VK_FORMAT_B8G8R8A8_UNORM };
			m_pEmissionAttachment->attachmentFormat = ChooseSupportedFormats(pDevice, formats, VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);


			for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
			{
				// Create Normal buffer image
				m_pEmissionAttachment->vecAttachmentImage[i] = Helper::Vulkan::CreateImage(	pDevice,
																							pSwapChain->m_vkSwapchainExtent.width,
																							pSwapChain->m_vkSwapchainExtent.height,
																							m_pEmissionAttachment->attachmentFormat,
																							VK_IMAGE_TILING_OPTIMAL,
																							VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
																							VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
																							&(m_pEmissionAttachment->vecAttachmentImageMemory[i]));

				// Create normal buffer image view!
				m_pEmissionAttachment->vecAttachmentImageView[i] = Helper::Vulkan::CreateImageView(	pDevice,
																									m_pEmissionAttachment->vecAttachmentImage[i],
																									m_pEmissionAttachment->attachmentFormat,
																									VK_IMAGE_ASPECT_COLOR_BIT);
			}
			break;
		}

		case AttachmentType::FB_ATTACHMENT_BACKGROUND:
		{
			m_pBackgroundAttachment = new FramebufferAttachment();

			m_pBackgroundAttachment->vecAttachmentImage.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pBackgroundAttachment->vecAttachmentImageView.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pBackgroundAttachment->vecAttachmentImageMemory.resize(pSwapChain->m_vecSwapchainImages.size());

			std::vector<VkFormat> formats = { VK_FORMAT_B8G8R8A8_UNORM };
			m_pBackgroundAttachment->attachmentFormat = ChooseSupportedFormats(pDevice, formats, VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);


			for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
			{
				// Create Normal buffer image
				m_pBackgroundAttachment->vecAttachmentImage[i] = Helper::Vulkan::CreateImage(pDevice,
																							 pSwapChain->m_vkSwapchainExtent.width,
																							 pSwapChain->m_vkSwapchainExtent.height,
																							 m_pBackgroundAttachment->attachmentFormat,
																							 VK_IMAGE_TILING_OPTIMAL,
																							 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
																							 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
																							 &(m_pBackgroundAttachment->vecAttachmentImageMemory[i]));

				// Create normal buffer image view!
				m_pBackgroundAttachment->vecAttachmentImageView[i] = Helper::Vulkan::CreateImageView(pDevice,
																									 m_pBackgroundAttachment->vecAttachmentImage[i],
																									 m_pBackgroundAttachment->attachmentFormat,
																									 VK_IMAGE_ASPECT_COLOR_BIT);
			}
			break;
		}

		case AttachmentType::FB_ATTACHMENT_OBJECTID:
		{
			m_pObjectIDAttachment = new FramebufferAttachment();

			m_pObjectIDAttachment->vecAttachmentImage.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pObjectIDAttachment->vecAttachmentImageView.resize(pSwapChain->m_vecSwapchainImages.size());
			m_pObjectIDAttachment->vecAttachmentImageMemory.resize(pSwapChain->m_vecSwapchainImages.size());

			std::vector<VkFormat> formats = { VK_FORMAT_B8G8R8A8_UNORM };
			m_pObjectIDAttachment->attachmentFormat = ChooseSupportedFormats(pDevice, formats, VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);


			for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
			{
				// Create Normal buffer image
				m_pObjectIDAttachment->vecAttachmentImage[i] = Helper::Vulkan::CreateImage(pDevice,
					pSwapChain->m_vkSwapchainExtent.width,
					pSwapChain->m_vkSwapchainExtent.height,
					m_pObjectIDAttachment->attachmentFormat,
					VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					&(m_pObjectIDAttachment->vecAttachmentImageMemory[i]));

				// Create normal buffer image view!
				m_pObjectIDAttachment->vecAttachmentImageView[i] = Helper::Vulkan::CreateImageView(pDevice,
					m_pObjectIDAttachment->vecAttachmentImage[i],
					m_pObjectIDAttachment->attachmentFormat,
					VK_IMAGE_ASPECT_COLOR_BIT);
			}
			break;
		}
	}
	
}

//---------------------------------------------------------------------------------------------------------------------
VkFormat DeferredFrameBuffer::ChooseSupportedFormats(VulkanDevice* pDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
	// Loop through options & find the compatible one
	for (VkFormat format : formats)
	{
		// Get properties for given formats on this device
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(pDevice->m_vkPhysicalDevice, format, &properties);

		// depending on tiling choice, need to check for different bit flag
		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}

		LOG_ERROR("Failed to find matching format!");
	}
}


//---------------------------------------------------------------------------------------------------------------------
void DeferredFrameBuffer::CreateFrameBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain, VkRenderPass renderPass)
{
	if (!pDevice || !pSwapChain)
		return;

	// resize framebuffer count to equal swap chain image views count
	m_vecFramebuffers.resize(pSwapChain->m_vecSwapchainImages.size());

	// create framebuffer for each swap chain image view
	for (uint32_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); ++i)
	{
		m_vecAttachments = {	pSwapChain->m_vecSwapchainImageViews[i],
								m_pAlbedoAttachment->vecAttachmentImageView[i],
								m_pDepthAttachment->vecAttachmentImageView[i],
								m_pNormalAttachment->vecAttachmentImageView[i],
								m_pPositionAttachment->vecAttachmentImageView[i],
								m_pPBRAttachment->vecAttachmentImageView[i],
								m_pEmissionAttachment->vecAttachmentImageView[i],
								m_pBackgroundAttachment->vecAttachmentImageView[i],
								m_pObjectIDAttachment->vecAttachmentImageView[i]
							};


		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass;											// Render pass layout the framebuffer will be used with					 
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(m_vecAttachments.size());
		framebufferCreateInfo.pAttachments = m_vecAttachments.data();							// List of attachments
		framebufferCreateInfo.width = pSwapChain->m_vkSwapchainExtent.width;					// framebuffer width
		framebufferCreateInfo.height = pSwapChain->m_vkSwapchainExtent.height;					// framebuffer height
		framebufferCreateInfo.layers = 1;														// framebuffer layers
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.pNext = nullptr;

		if (vkCreateFramebuffer(pDevice->m_vkLogicalDevice, &framebufferCreateInfo, nullptr, &m_vecFramebuffers[i]) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create Framebuffer");
		}
		else
			LOG_INFO("Framebuffer created!");
	}
}

//---------------------------------------------------------------------------------------------------------------------
void DeferredFrameBuffer::Cleanup(VulkanDevice* pDevice)
{
	m_pAlbedoAttachment->Cleanup(pDevice);
	m_pDepthAttachment->Cleanup(pDevice);
	m_pNormalAttachment->Cleanup(pDevice);
	m_pPositionAttachment->Cleanup(pDevice);
	m_pPBRAttachment->Cleanup(pDevice);
	m_pEmissionAttachment->Cleanup(pDevice);
	m_pBackgroundAttachment->Cleanup(pDevice);
	m_pObjectIDAttachment->Cleanup(pDevice);

	// Destroy frame buffers!
	for (uint32_t i = 0; i < m_vecFramebuffers.size(); ++i)
	{
		vkDestroyFramebuffer(pDevice->m_vkLogicalDevice, m_vecFramebuffers[i], nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void DeferredFrameBuffer::CleanupOnWindowResize(VulkanDevice* pDevice)
{
	m_pAlbedoAttachment->CleanupOnWindowResize(pDevice);
	m_pDepthAttachment->CleanupOnWindowResize(pDevice);
	m_pNormalAttachment->CleanupOnWindowResize(pDevice);
	m_pPositionAttachment->CleanupOnWindowResize(pDevice);
	m_pPBRAttachment->CleanupOnWindowResize(pDevice);
	m_pEmissionAttachment->CleanupOnWindowResize(pDevice);
	m_pBackgroundAttachment->CleanupOnWindowResize(pDevice);
	m_pObjectIDAttachment->CleanupOnWindowResize(pDevice);
		
	// Destroy frame buffers!
	for (uint32_t i = 0; i < m_vecFramebuffers.size(); ++i)
	{
		vkDestroyFramebuffer(pDevice->m_vkLogicalDevice, m_vecFramebuffers[i], nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void FramebufferAttachment::Cleanup(VulkanDevice* pDevice)
{
	// Cleanup depth related buffers, textures, memory etc.
	for (uint16_t i = 0; i < vecAttachmentImage.size(); i++)
	{
		vkDestroyImageView(pDevice->m_vkLogicalDevice, vecAttachmentImageView[i], nullptr);
		vkDestroyImage(pDevice->m_vkLogicalDevice, vecAttachmentImage[i], nullptr);
		vkFreeMemory(pDevice->m_vkLogicalDevice, vecAttachmentImageMemory[i], nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void FramebufferAttachment::CleanupOnWindowResize(VulkanDevice* pDevice)
{
	// Cleanup depth related buffers, textures, memory etc.
	for (uint16_t i = 0; i < vecAttachmentImage.size(); i++)
	{
		vkDestroyImageView(pDevice->m_vkLogicalDevice, vecAttachmentImageView[i], nullptr);
		vkDestroyImage(pDevice->m_vkLogicalDevice, vecAttachmentImage[i], nullptr);
		vkFreeMemory(pDevice->m_vkLogicalDevice, vecAttachmentImageMemory[i], nullptr);
	}
}
