#pragma once

#include "vulkan/vulkan.h"
#include "Engine/Helpers/Utility.h"

class DummySkybox
{
public:
	DummySkybox();
	~DummySkybox();

	void								Init(VulkanDevice* pDevice);
	void								Render(VulkanDevice* pDevice, VkCommandBuffer cmdBuffer);
	void								Cleanup(VulkanDevice* pDevice);

private:
	void								CreateVertexBuffer(VulkanDevice* pDevice);
	void								CreateIndexBuffer(VulkanDevice* pDevice);

private:
	std::vector<Helper::App::VertexP>	m_vecVertices;
	std::vector<uint32_t>				m_vecIndices;
	VkBuffer							m_vkVertexBuffer;
	VkBuffer							m_vkIndexBuffer;
	VkDeviceMemory						m_vkVertexBufferMemory;
	VkDeviceMemory						m_vkIndexBufferMemory;
};

