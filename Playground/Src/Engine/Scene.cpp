#include "PlaygroundPCH.h"
#include "Scene.h"

#include "glm/gtc/matrix_access.hpp"

#include "Renderer/VulkanDevice.h"
#include "Renderer/VulkanSwapChain.h"
#include "Renderer/VulkanGraphicsPipeline.h"

#include "Engine/RenderObjects/TriangleMesh.h"
#include "Engine/RenderObjects/RTXCube.h"


//---------------------------------------------------------------------------------------------------------------------
Scene::Scene()
{
	m_vecSceneObjects.clear();
}

//---------------------------------------------------------------------------------------------------------------------
Scene::~Scene()
{
	m_vecSceneObjects.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::LoadScene(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	// Load all 3D models...
	LoadModels(pDevice, pSwapchain);

	// Set light properties
	m_LightAngleEuler = glm::vec3(-90,80,40);
	m_LightIntensity = 1.0f;
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::LoadModels(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	TriangleMesh* pMesh = new TriangleMesh("Assets/Models/SteamPunk.fbx");
	pMesh->Initialize(pDevice);

	RTXCube* pCube = new RTXCube();
	pCube->Initialize(pDevice);

	m_vecSceneObjects.push_back(pMesh);
	m_vecSceneObjects.push_back(pCube);
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt)
{
	// Update each model's uniform data
	for (SceneObject* object : m_vecSceneObjects)
	{
		if (object != nullptr)
		{
			object->Update(dt);
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
	for (SceneObject* object : m_vecSceneObjects)
	{
		if (object != nullptr)
		{
			object->Render();
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
//void Scene::SetLightDirection(const glm::vec3& eulerAngles)
//{
//	m_LightAngleEuler = eulerAngles;
//
//	glm::mat4 rotateXYZ = glm::mat4(1);
//	rotateXYZ = glm::rotate(rotateXYZ, glm::radians(m_LightAngleEuler.z), glm::vec3(0, 0, 1));
//	rotateXYZ = glm::rotate(rotateXYZ, glm::radians(m_LightAngleEuler.y), glm::vec3(0, 1, 0));
//	rotateXYZ = glm::rotate(rotateXYZ, glm::radians(m_LightAngleEuler.x), glm::vec3(1, 0, 0));
//
//	m_LightDirection = glm::column(rotateXYZ, 1);
//}

//---------------------------------------------------------------------------------------------------------------------
void Scene::Cleanup(VulkanDevice* pDevice)
{
	for (SceneObject* object : m_vecSceneObjects)
	{
		object->Cleanup(pDevice);
	}	
}



