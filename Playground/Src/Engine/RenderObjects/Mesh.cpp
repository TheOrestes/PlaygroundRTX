#include "PlaygroundPCH.h"
#include "Engine/Helpers/Utility.h"
#include "Engine/Helpers/Log.h"
#include "Mesh.h"

#include "Engine/Renderer/VulkanDevice.h"

//---------------------------------------------------------------------------------------------------------------------
Mesh::Mesh(VulkanDevice* device,
	const std::vector<Helper::App::VertexPNTBT>& vertices,
	const std::vector<uint32_t>& indices)
{
	m_uiVertexCount = vertices.size();
	m_uiIndexCount = indices.size();

	CreateVertexBuffer(device, vertices);
	CreateIndexBuffer(device, indices);

	//m_pushConstData.matModel = glm::mat4(1.0f);
}

//---------------------------------------------------------------------------------------------------------------------
Mesh::Mesh(VulkanDevice* device,
	const std::vector<Helper::App::VertexPNT>& vertices,
	const std::vector<uint32_t>& indices)
{
	m_uiVertexCount = vertices.size();
	m_uiIndexCount = indices.size();

	CreateVertexBuffer(device, vertices);
	CreateIndexBuffer(device, indices);

	//m_pushConstData.matModel = glm::mat4(1.0f);
}

//---------------------------------------------------------------------------------------------------------------------
void Mesh::SetPushConstantData(glm::mat4 modelMatrix)
{
	//m_pushConstData.matModel = modelMatrix;
}

//---------------------------------------------------------------------------------------------------------------------
Mesh::~Mesh()
{
}

//---------------------------------------------------------------------------------------------------------------------
void Mesh::Cleanup(VulkanDevice* pDevice)
{
	vkDestroyBuffer(pDevice->m_vkLogicalDevice, m_vkVertexBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkVertexBufferMemory, nullptr);

	vkDestroyBuffer(pDevice->m_vkLogicalDevice, m_vkIndexBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkIndexBufferMemory, nullptr);
}

void Mesh::CleanupOnWindowsResize(VulkanDevice* pDevice)
{

}

//---------------------------------------------------------------------------------------------------------------------
void Mesh::CreateVertexBuffer(VulkanDevice* pDevice, const std::vector<Helper::App::VertexPNTBT>& vertices)
{
	// Get the size of buffer needed for vertices
	VkDeviceSize bufferSize = m_uiVertexCount * sizeof(Helper::App::VertexPNTBT);

	// Temporary buffer to "stage" vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Create buffer & allocate memory to it!
	pDevice->CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		&stagingBufferMemory);

	//-- MAP MEMORY TO VERTEX BUFFER
	void* data;																					// 1. Create pointer to a point in normal memory
	vkMapMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);		// 2. Map the vertex buffer memory to that point
	memcpy(data, vertices.data(), (size_t)bufferSize);											// 3. Copy memory from vertices vector to the point
	vkUnmapMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory);								// 4. Unmap the vertex buffer memory

	// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER_BIT)
	// Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU & accessible by it & not CPU!
	pDevice->CreateBuffer(	bufferSize,
							VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
							VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
							&m_vkVertexBuffer,
							&m_vkVertexBufferMemory);

	// Copy staging buffer to vertex buffer on GPU using Command buffer!
	pDevice->CopyBuffer(stagingBuffer, m_vkVertexBuffer, bufferSize);

	// Clean up staging buffers
	vkDestroyBuffer(pDevice->m_vkLogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void Mesh::CreateVertexBuffer(VulkanDevice* pDevice, const std::vector<Helper::App::VertexPNT>& vertices)
{
	// Get the size of buffer needed for vertices
	VkDeviceSize bufferSize = m_uiVertexCount * sizeof(Helper::App::VertexPNT);

	// Temporary buffer to "stage" vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Create buffer & allocate memory to it!
	pDevice->CreateBuffer(	bufferSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							&stagingBuffer,
							&stagingBufferMemory);

	//-- MAP MEMORY TO VERTEX BUFFER
	void* data;																					// 1. Create pointer to a point in normal memory
	vkMapMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);		// 2. Map the vertex buffer memory to that point
	memcpy(data, vertices.data(), (size_t)bufferSize);											// 3. Copy memory from vertices vector to the point
	vkUnmapMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory);								// 4. Unmap the vertex buffer memory

	// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER_BIT)
	// Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU & accessible by it & not CPU!
	pDevice->CreateBuffer(	bufferSize,
							VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
							VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
							&m_vkVertexBuffer,
							&m_vkVertexBufferMemory);

	// Copy staging buffer to vertex buffer on GPU using Command buffer!
	pDevice->CopyBuffer(stagingBuffer, m_vkVertexBuffer, bufferSize);

	// Clean up staging buffers
	vkDestroyBuffer(pDevice->m_vkLogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void Mesh::CreateIndexBuffer(VulkanDevice* pDevice, const std::vector<uint32_t>& indices)
{
	// Get size of buffer needed for indices
	VkDeviceSize bufferSize = m_uiIndexCount * sizeof(uint32_t);

	// Temporary buffer to "stage" index data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	pDevice->CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		&stagingBufferMemory);

	// Map memory to Index buffer
	void* data;
	vkMapMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory);

	// Create buffer for index data on GPU access only area
	pDevice->CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&m_vkIndexBuffer,
		&m_vkIndexBufferMemory);

	// Copy from staging buffer to GPU access buffer
	pDevice->CopyBuffer(stagingBuffer, m_vkIndexBuffer, bufferSize);

	// Clean up staging buffers
	vkDestroyBuffer(pDevice->m_vkLogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory, nullptr);
}


