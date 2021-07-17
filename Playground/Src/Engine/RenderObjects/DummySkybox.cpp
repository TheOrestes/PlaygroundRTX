#include "PlaygroundPCH.h"
#include "DummySkybox.h"

//---------------------------------------------------------------------------------------------------------------------
DummySkybox::DummySkybox()
{
	m_vecVertices.clear();
	m_vecIndices.clear();
	
	m_vkVertexBuffer = VK_NULL_HANDLE;
	m_vkIndexBuffer = VK_NULL_HANDLE;
	m_vkVertexBufferMemory = VK_NULL_HANDLE;
	m_vkIndexBufferMemory = VK_NULL_HANDLE;

}

//---------------------------------------------------------------------------------------------------------------------
DummySkybox::~DummySkybox()
{
}

//---------------------------------------------------------------------------------------------------------------------
void DummySkybox::Init(VulkanDevice* pDevice)
{
	// Create Vertex data for skybox
	m_vecVertices.reserve(8);
	m_vecVertices.emplace_back(glm::vec3(-1, -1, 1));
	m_vecVertices.emplace_back(glm::vec3(1, -1, 1));
	m_vecVertices.emplace_back(glm::vec3(1, 1, 1));
	m_vecVertices.emplace_back(glm::vec3(-1, 1, 1));
	m_vecVertices.emplace_back(glm::vec3(-1, -1, -1));
	m_vecVertices.emplace_back(glm::vec3(1, -1, -1));
	m_vecVertices.emplace_back(glm::vec3(1, 1, -1));
	m_vecVertices.emplace_back(glm::vec3(-1, 1, -1));

	CreateVertexBuffer(pDevice);

	// Create Index data for skybox
	m_vecIndices.reserve(36);
	m_vecIndices.emplace_back(0);	m_vecIndices.emplace_back(1);	m_vecIndices.emplace_back(2);
	m_vecIndices.emplace_back(2);	m_vecIndices.emplace_back(3);	m_vecIndices.emplace_back(0);

	m_vecIndices.emplace_back(3);	m_vecIndices.emplace_back(2);	m_vecIndices.emplace_back(6);
	m_vecIndices.emplace_back(6);	m_vecIndices.emplace_back(7);	m_vecIndices.emplace_back(3);

	m_vecIndices.emplace_back(7);	m_vecIndices.emplace_back(6);	m_vecIndices.emplace_back(5);
	m_vecIndices.emplace_back(5);	m_vecIndices.emplace_back(4);	m_vecIndices.emplace_back(7);

	m_vecIndices.emplace_back(4);	m_vecIndices.emplace_back(5);	m_vecIndices.emplace_back(1);
	m_vecIndices.emplace_back(1);	m_vecIndices.emplace_back(0);	m_vecIndices.emplace_back(4);

	m_vecIndices.emplace_back(4);	m_vecIndices.emplace_back(0);	m_vecIndices.emplace_back(3);
	m_vecIndices.emplace_back(3);	m_vecIndices.emplace_back(7);	m_vecIndices.emplace_back(4);

	m_vecIndices.emplace_back(1);	m_vecIndices.emplace_back(5);	m_vecIndices.emplace_back(6);
	m_vecIndices.emplace_back(6);	m_vecIndices.emplace_back(2);	m_vecIndices.emplace_back(1);

	CreateIndexBuffer(pDevice);
}

//---------------------------------------------------------------------------------------------------------------------
void DummySkybox::Render(VulkanDevice* pDevice, VkCommandBuffer cmdBuffer)
{

	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_vkVertexBuffer, offsets);

	// bind mesh index buffer, with zero offset & using uint32_t type
	vkCmdBindIndexBuffer(cmdBuffer, m_vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	// Execute pipeline
	vkCmdDrawIndexed(cmdBuffer, 36, 1, 0, 0, 0);
}

//---------------------------------------------------------------------------------------------------------------------
void DummySkybox::CreateVertexBuffer(VulkanDevice* pDevice)
{
	// Get the size of buffer needed for vertices
	VkDeviceSize bufferSize = 8 * sizeof(Helper::App::VertexP);

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
	memcpy(data, m_vecVertices.data(), (size_t)bufferSize);										// 3. Copy memory from vertices vector to the point
	vkUnmapMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory);								// 4. Unmap the vertex buffer memory

	// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER_BIT)
	// Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU & accessible by it & not CPU!
	pDevice->CreateBuffer(bufferSize,
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
void DummySkybox::CreateIndexBuffer(VulkanDevice* pDevice)
{
	// Get size of buffer needed for indices
	VkDeviceSize bufferSize = 36 * sizeof(uint32_t);

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
	memcpy(data, m_vecIndices.data(), (size_t)bufferSize);
	vkUnmapMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory);

	// Create buffer for index data on GPU access only area
	pDevice->CreateBuffer(	bufferSize,
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

//---------------------------------------------------------------------------------------------------------------------
void DummySkybox::Cleanup(VulkanDevice* pDevice)
{
	vkDestroyBuffer(pDevice->m_vkLogicalDevice, m_vkVertexBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkVertexBufferMemory, nullptr);

	vkDestroyBuffer(pDevice->m_vkLogicalDevice, m_vkIndexBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkIndexBufferMemory, nullptr);
}
