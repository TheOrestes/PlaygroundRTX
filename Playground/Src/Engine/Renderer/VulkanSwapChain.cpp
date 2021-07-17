
#include "PlaygroundPCH.h"
#include "VulkanSwapChain.h"

#include "PlaygroundHeaders.h"
#include "Engine/Helpers/Log.h"
#include "Engine/Helpers/Utility.h"

//---------------------------------------------------------------------------------------------------------------------
VulkanSwapChain::VulkanSwapChain()
{
    m_vkSwapchain = nullptr;

    m_vecSwapchainImages.clear();
    m_vecSwapchainImageViews.clear();
}

//---------------------------------------------------------------------------------------------------------------------
VulkanSwapChain::~VulkanSwapChain()
{
    m_vecSwapchainImages.clear();
    m_vecSwapchainImageViews.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanSwapChain::CreateSwapChain(VulkanDevice* pDevice, VkSurfaceKHR surface, GLFWwindow* pWindow)
{
    // Get swap chain details so we can pick the best setting!
    SwapChainSupportDetails* pSwapChainSupport = QuerySwapChainSupport(pDevice, surface);

    // 1. CHOOSE BEST SURFACE FORMAT
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(pSwapChainSupport->formats);

    // 2. CHOOSE BEST PRESENTATION MODE
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(pSwapChainSupport->presentModes);

    // 3. CHOOSE SWAPCHAIN IMAGE RESOLUTION
    VkExtent2D extent = ChooseSwapExtent(pSwapChainSupport->capabilities, pWindow);

    // decide how many images to have in the swap chain, it's good practice to have an extra count.
    // Also make sure it does not exceed maximum number of images
    m_uiMinImageCount = pSwapChainSupport->capabilities.minImageCount;
    m_uiImageCount = m_uiMinImageCount + 1;
    if (pSwapChainSupport->capabilities.maxImageCount > 0 && m_uiImageCount > pSwapChainSupport->capabilities.maxImageCount)
    {
        m_uiImageCount = pSwapChainSupport->capabilities.maxImageCount;
    }

    // Create SwapChain object
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.clipped = VK_TRUE;									// don't care about the obscured pixels by other window
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;	// ignore alpha channel to blend with other windows
    createInfo.flags = 0;
    createInfo.imageArrayLayers = 1;								// Number of layers each image has, always 1, except for stereo app.
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;	// Ray tracing shaders will write to image, 
                                                                                                    // but, we need to copy that image to swapchain
                                                                                                    // image before presenting. Hence, we need both
                                                                                                    // Color attachment & transfer dst bit set. 
    createInfo.minImageCount = m_uiImageCount;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    createInfo.pNext = nullptr;
    createInfo.presentMode = presentMode;
    createInfo.preTransform = pSwapChainSupport->capabilities.currentTransform;	// transform to image, say 90 degrees!
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    // Specify how to handle swap chain images that will be used across multiple queue families. That will be the case 
    // in our application if the graphics queue family is different from the presentation queue. We'll be drawing on 
    // the images in the swap chain from the graphics queue and then submitting them on the presentation queue. 
    // There are two ways to handle images that are accessed from multiple queues. 

    std::array<uint32_t, 2> queueFamilyIndices = { pDevice->m_pQueueFamilyIndices->m_uiGraphicsFamily.value(), pDevice->m_pQueueFamilyIndices->m_uiPresentFamily.value() };

    // check if Graphics & Presentation family share the same index or not!
    if (queueFamilyIndices[0] != queueFamilyIndices[1])
    {
        // Images can be used across multiple queue families without explicit ownership transfers.
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    }
    else
    {
        // An image is owned by one queue family at a time and ownership must be explicitly transferred before using 
        // it in another queue family. This option offers the best performance.
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    // Finally, create the swap chain...
    if (vkCreateSwapchainKHR(pDevice->m_vkLogicalDevice, &createInfo, nullptr, &m_vkSwapchain) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create Swap chain!");
        return;
    }

    LOG_INFO("Swapchain created!");

    // Retrieve handle to swapchain images...
    vkGetSwapchainImagesKHR(pDevice->m_vkLogicalDevice, m_vkSwapchain, &m_uiImageCount, nullptr);
    m_vecSwapchainImages.resize(m_uiImageCount);
    vkGetSwapchainImagesKHR(pDevice->m_vkLogicalDevice, m_vkSwapchain, &m_uiImageCount, m_vecSwapchainImages.data());

    LOG_INFO("Swapchain images created!");

    // store format & extent for later usage...
    m_vkSwapchainImageFormat = surfaceFormat.format;
    m_vkSwapchainExtent = extent;

    // Create SwapChain ImageViews!
    CreateSwapChainImageViews(pDevice);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanSwapChain::CreateSwapChainImageViews(VulkanDevice* pDevice)
{
    m_vecSwapchainImageViews.resize(m_vecSwapchainImages.size());

    for (uint32_t i = 0; i < m_vecSwapchainImageViews.size(); ++i)
    {
        m_vecSwapchainImageViews[i] = Helper::Vulkan::CreateImageView(pDevice,
            m_vecSwapchainImages[i],
            m_vkSwapchainImageFormat,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }

    LOG_INFO("Swapchain Imageviews created!");
}

//---------------------------------------------------------------------------------------------------------------------
SwapChainSupportDetails* VulkanSwapChain::QuerySwapChainSupport(VulkanDevice* pDevice, VkSurfaceKHR surface)
{
    SwapChainSupportDetails* swapChainDetails = new SwapChainSupportDetails();

    // Start with basic surface capabilities...
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pDevice->m_vkPhysicalDevice, surface, &(swapChainDetails->capabilities));

    // Now query supported surface formats...
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice->m_vkPhysicalDevice, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        swapChainDetails->formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice->m_vkPhysicalDevice, surface, &formatCount, swapChainDetails->formats.data());
    }

    // Finally, query supported presentation modes...
    uint32_t presentationModesCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice->m_vkPhysicalDevice, surface, &presentationModesCount, nullptr);

    if (presentationModesCount != 0)
    {
        swapChainDetails->presentModes.resize(presentationModesCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice->m_vkPhysicalDevice, surface, &presentationModesCount, swapChainDetails->presentModes.data());
    }

    return swapChainDetails;
}

//---------------------------------------------------------------------------------------------------------------------
VkSurfaceFormatKHR VulkanSwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    // If only 1 format available and is undefined, then this means ALL formats are available (no restrictions)
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    // If restricted, search for optimal format
    for (const auto& format : availableFormats)
    {
        if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
            && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    // If can't find optimal format, then just return first format
    return availableFormats[0];
}

//---------------------------------------------------------------------------------------------------------------------
VkPresentModeKHR VulkanSwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    // out of all Mailbox allows triple buffering, so if available use it, else use FIFO mode.
    for (uint32_t i = 0; i < availablePresentModes.size(); ++i)
    {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentModes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

//---------------------------------------------------------------------------------------------------------------------
VkExtent2D VulkanSwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* pWindow)
{
    // The swap extent is the resolution of the swap chain images and it's almost always exactly equal to the 
    // resolution of the window that we're drawing to.The range of the possible resolutions is defined in the 
    // VkSurfaceCapabilitiesKHR structure.Vulkan tells us to match the resolution of the window by setting the 
    // width and height in the currentExtent member.However, some window managers do allow us to differ here 
    // and this is indicated by setting the width and height in currentExtent to a special value : the maximum 
    // value of uint32_t. In that case we'll pick the resolution that best matches the window within the 
    // minImageExtent and maxImageExtent bounds.
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        // To handle window resize properly, query current width-height of framebuffer, instead of global value!
        int width, height;
        glfwGetFramebufferSize(pWindow, &width, &height);

        //VkExtent2D actualExtent = { Helper::App::WIDTH, Helper::App::HEIGHT };
        VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanSwapChain::Cleanup(VulkanDevice* pDevice)
{
    for (uint32_t i = 0; i < m_vecSwapchainImageViews.size(); ++i)
    {
        //vkDestroyImage(pDevice->m_vkLogicalDevice, m_vecSwapchainImages[i], nullptr);
        vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vecSwapchainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(pDevice->m_vkLogicalDevice, m_vkSwapchain, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanSwapChain::CleanupOnWindowResize(VulkanDevice* pDevice)
{
    for (uint32_t i = 0; i < m_vecSwapchainImageViews.size(); ++i)
    {
        //vkDestroyImage(pDevice->m_vkLogicalDevice, m_vecSwapchainImages[i], nullptr);
        vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vecSwapchainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(pDevice->m_vkLogicalDevice, m_vkSwapchain, nullptr);
}
