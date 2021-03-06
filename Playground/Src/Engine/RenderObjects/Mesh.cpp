#include "PlaygroundPCH.h"
#include "Engine/Helpers/Utility.h"
#include "Engine/Helpers/Log.h"
#include "Mesh.h"

#include "Engine/Renderer/VulkanDevice.h"

//---------------------------------------------------------------------------------------------------------------------
Mesh::Mesh(VulkanDevice* device, const std::vector<App::VertexPNTBT>& vertices, const std::vector<uint32_t>& indices)
{
	m_uiVertexCount = vertices.size();
	m_uiIndexCount = indices.size();

	CreateVertexBuffer(device, vertices);
	CreateIndexBuffer(device, indices);

	m_pBottomLevelASInput = nullptr;

	//m_pushConstData.matModel = glm::mat4(1.0f);
}

//---------------------------------------------------------------------------------------------------------------------
Mesh::Mesh(VulkanDevice* device, const std::vector<App::VertexPNT>& vertices, const std::vector<uint32_t>& indices)
{
	m_uiVertexCount = vertices.size();
	m_uiIndexCount = indices.size();

	CreateVertexBuffer(device, vertices);
	CreateIndexBuffer(device, indices);

	m_pBottomLevelASInput = nullptr;

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
	SAFE_DELETE(m_pBottomLevelASInput);
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
void Mesh::CreateVertexBuffer(VulkanDevice* pDevice, const std::vector<App::VertexPNTBT>& vertices)
{
	// Get the size of buffer needed for vertices
	VkDeviceSize bufferSize = m_uiVertexCount * sizeof(App::VertexPNTBT);

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
void Mesh::CreateVertexBuffer(VulkanDevice* pDevice, const std::vector<App::VertexPNT>& vertices)
{
	// Get the size of buffer needed for vertices
	VkDeviceSize bufferSize = m_uiVertexCount * sizeof(App::VertexPNT);

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

//---------------------------------------------------------------------------------------------------------------------
void Mesh::MeshToVkGeometryKHR(VulkanDevice* pDevice)
{
	// BottomLevelAS requires raw device address!
	VkDeviceAddress vbAddress = Vulkan::GetBufferDeviceAddress(pDevice, m_vkVertexBuffer);
	VkDeviceAddress ibAddress = Vulkan::GetBufferDeviceAddress(pDevice, m_vkIndexBuffer);

	uint32_t maxPrimitiveCount = m_uiIndexCount / 3;

	// Describe buffer as array of VertexPNTBT
	VkAccelerationStructureGeometryTrianglesDataKHR trianglesData = {};
	trianglesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	trianglesData.vertexFormat = VK_FORMAT_R32G32B32A32_SFLOAT;			 
	trianglesData.vertexData.deviceAddress = vbAddress;
	trianglesData.vertexStride = sizeof(App::VertexPNTBT);
	trianglesData.indexType = VK_INDEX_TYPE_UINT32;
	trianglesData.indexData.deviceAddress = ibAddress;
	trianglesData.maxVertex = m_uiVertexCount;

	// Treat above data as containing Opaque Triangles!
	VkAccelerationStructureGeometryKHR geomAccelStruct = {};
	geomAccelStruct.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geomAccelStruct.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geomAccelStruct.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	geomAccelStruct.geometry.triangles = trianglesData;

	// The entire array will be used to build the BottomLevelAS!
	VkAccelerationStructureBuildRangeInfoKHR accelStructOffset = {};
	accelStructOffset.firstVertex = 0;
	accelStructOffset.primitiveCount = maxPrimitiveCount;
	accelStructOffset.primitiveOffset = 0;
	accelStructOffset.transformOffset = 0;

	// Our BottomLevelAS is built from this mesh
	m_pBottomLevelASInput = new BottomLevelASInput();
	m_pBottomLevelASInput->m_vkAccelStructGeometry = geomAccelStruct;
	m_pBottomLevelASInput->m_vkAccelStructBuildOffset = accelStructOffset;
}


