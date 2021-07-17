#include "PlaygroundPCH.h"
#include "VulkanFrameBuffer.h"

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "Engine/Helpers/Log.h"
#include "Engine/Helpers/Utility.h"
#include "PlaygroundHeaders.h"

//---------------------------------------------------------------------------------------------------------------------
VulkanFrameBuffer::VulkanFrameBuffer()
{
    m_attachmentFormat = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;

    m_vecAttachmentImage.clear();
    m_vecAttachmentImageMemory.clear();
    m_vecAttachmentImageView.clear();
    
    m_vecFramebuffer.clear();
}

//---------------------------------------------------------------------------------------------------------------------
VulkanFrameBuffer::~VulkanFrameBuffer()
{
    m_vecAttachmentImage.clear();
    m_vecAttachmentImageMemory.clear();
    m_vecAttachmentImageView.clear();

    m_vecFramebuffer.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanFrameBuffer::CreateAttachment(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain)
{
    m_vecAttachmentImage.resize(pSwapChain->m_vecSwapchainImages.size());
    m_vecAttachmentImageView.resize(pSwapChain->m_vecSwapchainImages.size());
    m_vecAttachmentImageMemory.resize(pSwapChain->m_vecSwapchainImages.size());	

    std::vector<VkFormat> formats = { pSwapChain->m_vkSwapchainImageFormat };//{ VK_FORMAT_R8G8B8A8_UNORM };
    m_attachmentFormat = ChooseSupportedFormats(pDevice, formats, 
                                                VK_IMAGE_TILING_OPTIMAL, 
                                                VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);

    for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
    {
        // Create color buffer image
        m_vecAttachmentImage[i] = Helper::Vulkan::CreateImage(	pDevice,
                                                                pSwapChain->m_vkSwapchainExtent.width,
                                                                pSwapChain->m_vkSwapchainExtent.height,
                                                                m_attachmentFormat,
                                                                VK_IMAGE_TILING_OPTIMAL,
                                                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                                &(m_vecAttachmentImageMemory[i]));

        // Create color buffer image view!
        m_vecAttachmentImageView[i] = Helper::Vulkan::CreateImageView(	pDevice,
                                                                        m_vecAttachmentImage[i],
                                                                        m_attachmentFormat,
                                                                        VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanFrameBuffer::CreateFrameBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain, VkRenderPass renderPass)
{
    if (!pDevice || !pSwapChain)
        return;
    
    // resize framebuffer count to equal swap chain image views count
    m_vecFramebuffer.resize(pSwapChain->m_vecSwapchainImages.size());
    
    // create framebuffer for each swap chain image view
    for (uint32_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); ++i)
    {
        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderPass;											// Render pass layout the framebuffer will be used with					 
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = &(m_vecAttachmentImageView[i]);					    // List of attachments
        framebufferCreateInfo.width = pSwapChain->m_vkSwapchainExtent.width;					// framebuffer width
        framebufferCreateInfo.height = pSwapChain->m_vkSwapchainExtent.height;					// framebuffer height
        framebufferCreateInfo.layers = 1;														// framebuffer layers
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.pNext = nullptr;

        if (vkCreateFramebuffer(pDevice->m_vkLogicalDevice, &framebufferCreateInfo, nullptr, &m_vecFramebuffer[i]) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create Framebuffer");
        }
        else
            LOG_INFO("Framebuffer created!");
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanFrameBuffer::Cleanup(VulkanDevice* pDevice)
{
    // Cleanup image, imageview, memory etc.
    for (uint16_t i = 0; i < m_vecAttachmentImage.size(); i++)
    {
        vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vecAttachmentImageView[i], nullptr);
        vkDestroyImage(pDevice->m_vkLogicalDevice, m_vecAttachmentImage[i], nullptr);
        vkFreeMemory(pDevice->m_vkLogicalDevice, m_vecAttachmentImageMemory[i], nullptr);
    }
    
    // Destroy frame buffers!
    for (uint16_t i = 0; i < m_vecFramebuffer.size(); ++i)
    {
        vkDestroyFramebuffer(pDevice->m_vkLogicalDevice, m_vecFramebuffer[i], nullptr);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanFrameBuffer::CleanupOnWindowResize(VulkanDevice* pDevice)
{
    // Cleanup image, imageview, memory etc.
    for (uint16_t i = 0; i < m_vecAttachmentImage.size(); i++)
    {
        vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vecAttachmentImageView[i], nullptr);
        vkDestroyImage(pDevice->m_vkLogicalDevice, m_vecAttachmentImage[i], nullptr);
        vkFreeMemory(pDevice->m_vkLogicalDevice, m_vecAttachmentImageMemory[i], nullptr);
    }
    
    // Destroy frame buffers!
    for (uint16_t i = 0; i < m_vecFramebuffer.size(); ++i)
    {
        vkDestroyFramebuffer(pDevice->m_vkLogicalDevice, m_vecFramebuffer[i], nullptr);
    }
}

//---------------------------------------------------------------------------------------------------------------------
VkFormat VulkanFrameBuffer::ChooseSupportedFormats(VulkanDevice* pDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
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
