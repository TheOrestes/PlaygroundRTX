#pragma once

#include "glm/glm.hpp"

class VulkanDevice;
class VulkanSwapChain;
class VulkanGraphicsPipeline;
class Model;

class Scene
{
public:
	Scene();
	~Scene();
	
	void						LoadScene(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void						Cleanup(VulkanDevice* pDevice);

	void						Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt);
	void						UpdateUniforms(VulkanDevice* pDevice, uint32_t imageIndex);
	void						RenderOpaque(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipline, uint32_t imageIndex);
	void						RenderSkybox(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipline, uint32_t imageIndex);
	void						RenderSkydome(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipline, uint32_t imageIndex);

	void						SetLightDirection(const glm::vec3& eulerAngles);
	
	inline glm::vec3			GetLightEulerAngles()	{ return m_LightAngleEuler; }
	inline std::vector<Model*>	GetModelList()			{ return m_vecModels; }

public:
	glm::vec3					m_LightDirection;
	float						m_LightIntensity;

private:
	void						LoadModels(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);

private:
	glm::vec3					m_LightAngleEuler;
	std::vector<Model*>			m_vecModels;
};

