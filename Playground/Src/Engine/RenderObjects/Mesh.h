#pragma once

#include "PlaygroundPCH.h"
#include "vulkan/vulkan.h"

#include "Engine/Helpers/Utility.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

class VulkanDevice;

struct PushConstantData
{
	glm::mat4 matModel;
};

class Mesh
{
public:
	Mesh() {};
	Mesh(VulkanDevice* device,
		const std::vector<Helper::App::VertexPNTBT>& vertices,
		const std::vector<uint32_t>& indices);

	Mesh(VulkanDevice* device,
		const std::vector<Helper::App::VertexPNT>& vertices,
		const std::vector<uint32_t>& indices);

	Mesh(VulkanDevice* device,
		const std::vector<Helper::App::VertexP>& vertices,
		const std::vector<uint32_t>& indices);

	void						SetPushConstantData(glm::mat4 modelMatrix);
	//inline PushConstantData		GetPushConstantData() { return m_pushConstData; }

	inline uint32_t				getVertexCount() const { return m_uiVertexCount; }
	inline VkBuffer				getVertexBuffer() const { return m_vkVertexBuffer; }

	inline uint32_t				getIndexCount() const { return m_uiIndexCount; }
	inline VkBuffer				getIndexBuffer() const { return m_vkIndexBuffer; }

	~Mesh();

	void						Cleanup(VulkanDevice* pDevice);
	void						CleanupOnWindowsResize(VulkanDevice* pDevice);

public:
	uint32_t					m_uiVertexCount;
	uint32_t					m_uiIndexCount;

	VkBuffer					m_vkVertexBuffer;
	VkBuffer					m_vkIndexBuffer;

private:
	//PushConstantData			m_pushConstData;

	VkDeviceMemory				m_vkVertexBufferMemory;
	VkDeviceMemory				m_vkIndexBufferMemory;

	void						CreateVertexBuffer(VulkanDevice* device, const std::vector<Helper::App::VertexPNT>& vertices);
	void						CreateVertexBuffer(VulkanDevice* device, const std::vector<Helper::App::VertexPNTBT>& vertices);
	void						CreateIndexBuffer(VulkanDevice* device, const std::vector<uint32_t>& indices);
};

