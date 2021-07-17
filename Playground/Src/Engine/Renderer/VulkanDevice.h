#pragma once

#include "vulkan/vulkan.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Almost every operation in Vulkan, from drawing to uploading textures requires commands to be submitted to Queue. 
// There are different types of queues from different families. Each family allows only subset of commands! 
// We need to check which queue families are supported by the device & which one of these supports the command we 
// want to use. 
// Struct below keeps track of all such queues based on their types...
struct QueueFamilyIndices
{
	QueueFamilyIndices()
	{
		m_uiGraphicsFamily.reset();
		m_uiPresentFamily.reset();
	}

	std::optional<uint32_t> m_uiGraphicsFamily;
	std::optional<uint32_t>	m_uiPresentFamily;

	bool isComplete() { return m_uiGraphicsFamily.has_value() && m_uiPresentFamily.has_value(); }
};


class VulkanDevice
{
public:
	VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
	~VulkanDevice();

	void								PickPhysicalDevice();
	void								CreateLogicalDevice();

	void								CreateGraphicsCommandPool();
	void								CreateGraphicsCommandBuffers(uint32_t size);

	uint32_t							FindMemoryTypeIndex(uint32_t allowedTypeIndex, VkMemoryPropertyFlags props);

	void								CreateBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, 
													 VkMemoryPropertyFlags bufferProperties, VkBuffer* outBuffer, 
													 VkDeviceMemory* outBufferMemory);

	VkCommandBuffer						BeginCommandBuffer();
	void								EndAndSubmitCommandBuffer(VkCommandBuffer commandBuffer);
	void								CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);

	void								Cleanup();
	void								CleanupOnWindowResize();

private:
	bool								CheckDeviceExtensionSupport(VkPhysicalDevice device);
	void								FindQueueFamilies(VkPhysicalDevice device);

	VkInstance							m_vkInstance;

	VkPhysicalDeviceProperties			m_vkDeviceProperties;
	VkPhysicalDeviceFeatures			m_vkDeviceFeaturesAvailable;
	VkPhysicalDeviceFeatures			m_vkDeviceFeaturesEnabled;

	VkPhysicalDeviceMemoryProperties	m_vkDeviceMemoryProps;
	std::vector<VkExtensionProperties>	m_vecSupportedExtensions;

public:
	VkSurfaceKHR						m_vkSurface;
	VkPhysicalDevice					m_vkPhysicalDevice;
	VkDevice							m_vkLogicalDevice;

	QueueFamilyIndices*					m_pQueueFamilyIndices;

	VkCommandPool						m_vkCommandPoolGraphics;
	std::vector<VkCommandBuffer>		m_vecCommandBufferGraphics;

	VkQueue								m_vkQueueGraphics;
	VkQueue								m_vkQueuePresent;
};


