#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "VulkanDevice.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Once we know that swap-chains are available, we need to check its compatibility with our window surface. 
// We need to check following properties:
// 1. Surface capabilities (min-max number of images in swap chain, width-height of images)
// 2. Surface formats (pixel formats, color spaces)
// 3. Available presentation modes
struct SwapChainSupportDetails
{
	SwapChainSupportDetails()
	{
		capabilities = {};
		formats.clear();
		presentModes.clear();
	}

	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class VulkanSwapChain
{
public:
	VulkanSwapChain();
	~VulkanSwapChain();

	void							CreateSwapChain(VulkanDevice* pDevice, VkSurfaceKHR surface, GLFWwindow* pWindow);

	void							Cleanup(VulkanDevice* pDevice);
	void							CleanupOnWindowResize(VulkanDevice* pDevice);
	

private:
	void							CreateSwapChainImageViews(VulkanDevice* pDevice);
	SwapChainSupportDetails*		QuerySwapChainSupport(VulkanDevice* pDevice, VkSurfaceKHR surface);
	VkSurfaceFormatKHR				ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR				ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D						ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* pWindow);

public:
	uint32_t						m_uiMinImageCount;
	uint32_t						m_uiImageCount;
	VkSwapchainKHR					m_vkSwapchain;
	VkFormat						m_vkSwapchainImageFormat;
	VkExtent2D						m_vkSwapchainExtent;

	std::vector<VkImage>			m_vecSwapchainImages;
	std::vector<VkImageView>		m_vecSwapchainImageViews;
};

