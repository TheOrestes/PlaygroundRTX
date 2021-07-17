#pragma once

#include "vulkan/vulkan.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

class VulkanDevice;
class VulkanSwapChain;
class VulkanFrameBuffer;
class Scene;

class UIManager
{
public:
	~UIManager();

	static UIManager& getInstance()
	{
		static UIManager manager;
		return manager;
	}

	void							Initialize(GLFWwindow* pWindow, VkInstance instance, VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void							HandleWindowResize(GLFWwindow* pWindow, VkInstance instance, VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void							Cleanup(VulkanDevice* pDevice);
	void							CleanupOnWindowResize(VulkanDevice* pDevice);
	void							BeginRender();
	void							EndRender(VulkanSwapChain* pSwapchain, uint32_t imageIndex);

	void							RenderSceneUI(Scene* pScene);
	void							RenderDebugStats();

private:
	UIManager();
	UIManager(const UIManager&);
	void operator=(const UIManager&);

	void							InitDescriptorPool(VulkanDevice* pDevice);
	void							InitCommandBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void							InitFramebuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void							InitRenderPass(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);

private:
	VkDescriptorPool				m_vkDescriptorPool;
	
	VkRenderPass					m_vkRenderPass;
	std::vector<VkFramebuffer>		m_vecFramebuffers;
	VkCommandPool					m_vkCommandPool;

public:
	std::vector<VkCommandBuffer>	m_vecCommandBuffers;

public:
	int								m_iPassID;
};

