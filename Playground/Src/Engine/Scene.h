#pragma once

#include "glm/glm.hpp"

class VulkanDevice;
class VulkanSwapChain;
class VulkanGraphicsPipeline;
class TriangleMesh;

class Scene
{
public:
	Scene();
	~Scene();
	
	void								LoadScene(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void								Cleanup(VulkanDevice* pDevice);

	void								Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt);
	void								UpdateUniforms(VulkanDevice* pDevice, uint32_t imageIndex);
	void								RenderOpaque(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipline, uint32_t imageIndex);

	void								SetLightDirection(const glm::vec3& eulerAngles);
	
	inline glm::vec3					GetLightEulerAngles()	{ return m_LightAngleEuler; }
	inline std::vector<TriangleMesh*>	GetModelList()			{ return m_vecMeshes; }

public:
	glm::vec3							m_LightDirection;
	float								m_LightIntensity;
										
private:								
	void								LoadModels(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
										
private:								
	glm::vec3							m_LightAngleEuler;
	std::vector<TriangleMesh*>			m_vecMeshes;
};

