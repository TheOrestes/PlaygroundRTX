#pragma once

#include "PlaygroundPCH.h"
#include "vulkan/vulkan.h"

#include "Engine/Helpers/Utility.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

class VulkanDevice;

//---------------------------------------------------------------------------------------------------------------------
class Mesh
{
public:
	Mesh() {};
	Mesh(VulkanDevice* device, const std::vector<App::VertexPNTBT>& vertices, const std::vector<uint32_t>& indices);
	Mesh(VulkanDevice* device, const std::vector<App::VertexPNT>& vertices, const std::vector<uint32_t>& indices);
	Mesh(VulkanDevice* device, const std::vector<App::VertexP>& vertices, const std::vector<uint32_t>& indices);

	void						SetPushConstantData(glm::mat4 modelMatrix);
	//inline PushConstantData		GetPushConstantData() { return m_pushConstData; }

	

	~Mesh();

	void						Cleanup(VulkanDevice* pDevice);
	void						CleanupOnWindowsResize(VulkanDevice* pDevice);

	void						MeshToVkGeometryKHR(VulkanDevice* pDevice);

public:
	Vulkan::MeshData*			m_pMeshData;
	Vulkan::MeshInstance*		m_pMeshInstanceData;
	

private:
	void						CreateVertexBuffer(VulkanDevice* device, const std::vector<App::VertexPNT>& vertices);
	void						CreateVertexBuffer(VulkanDevice* device, const std::vector<App::VertexPNTBT>& vertices);
	void						CreateIndexBuffer(VulkanDevice* device, const std::vector<uint32_t>& indices);
};

