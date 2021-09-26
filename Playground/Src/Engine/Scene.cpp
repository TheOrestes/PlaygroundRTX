#include "PlaygroundPCH.h"
#include "Scene.h"

#include "glm/gtc/matrix_access.hpp"

#include "Renderer/VulkanDevice.h"
#include "Renderer/VulkanSwapChain.h"
#include "Renderer/VulkanGraphicsPipeline.h"

#include "Engine/RenderObjects/TriangleMesh.h"


//---------------------------------------------------------------------------------------------------------------------
Scene::Scene()
{
	m_vecMeshes.clear();
}

//---------------------------------------------------------------------------------------------------------------------
Scene::~Scene()
{
	m_vecMeshes.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::LoadScene(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	// Load all 3D models...
	LoadModels(pDevice, pSwapchain);

	// Set light properties
	m_LightAngleEuler = glm::vec3(-90,80,40);
	SetLightDirection(m_LightAngleEuler);
	
	m_LightIntensity = 1.0f;
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::LoadModels(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt)
{
	// Update each model's uniform data
	for (TriangleMesh* mesh : m_vecMeshes)
	{
		if (mesh != nullptr)
		{
			mesh->Update(dt);
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::UpdateUniforms(VulkanDevice* pDevice, uint32_t imageIndex)
{
	
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::RenderOpaque(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipline, uint32_t imageIndex)
{
	// Draw Scene!
	for (TriangleMesh* mesh : m_vecMeshes)
	{
		if (mesh != nullptr)
		{
			mesh->Render();
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::SetLightDirection(const glm::vec3& eulerAngles)
{
	m_LightAngleEuler = eulerAngles;

	glm::mat4 rotateXYZ = glm::mat4(1);
	rotateXYZ = glm::rotate(rotateXYZ, glm::radians(m_LightAngleEuler.z), glm::vec3(0, 0, 1));
	rotateXYZ = glm::rotate(rotateXYZ, glm::radians(m_LightAngleEuler.y), glm::vec3(0, 1, 0));
	rotateXYZ = glm::rotate(rotateXYZ, glm::radians(m_LightAngleEuler.x), glm::vec3(1, 0, 0));

	m_LightDirection = glm::column(rotateXYZ, 1);
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::Cleanup(VulkanDevice* pDevice)
{
	for (TriangleMesh* mesh : m_vecMeshes)
	{
		mesh->Cleanup(pDevice);
	}	
}



