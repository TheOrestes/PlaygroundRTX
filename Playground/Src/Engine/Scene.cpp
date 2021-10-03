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
	//TriangleMesh* pMeshPunk = new TriangleMesh("Assets/Models/SteamPunk.fbx");
	//pMeshPunk->Initialize(pDevice);
	//pMeshPunk->SetPosition(glm::vec3(-1,0,0));
	//pMeshPunk->SetScale(glm::vec3(0.25f));
	//pMeshPunk->SetUpdate(true);

	TriangleMesh* pMeshBarb = new TriangleMesh("Assets/Models/barb1.fbx");
	pMeshBarb->Initialize(pDevice);
	pMeshBarb->SetPosition(glm::vec3(0, -0.5f, 0));
	pMeshBarb->SetScale(glm::vec3(1.0f));

	TriangleMesh* pMeshPlane = new TriangleMesh("Assets/Models/Plane_Oak.fbx");
	pMeshPlane->Initialize(pDevice);
	pMeshPlane->SetPosition(glm::vec3(0, -1, 0));
	pMeshPlane->SetScale(glm::vec3(1.0f));

	//RTXCube* pCube = new RTXCube();
	//pCube->Initialize(pDevice);
	//pCube->SetPosition(glm::vec3(3,0,0));
	//pCube->SetScale(glm::vec3(0.5));
	//pCube->SetUpdate(true);

	//m_vecSceneObjects.push_back(pMeshPunk);
	m_vecSceneObjects.push_back(pMeshBarb);
	m_vecSceneObjects.push_back(pMeshPlane);
	//m_vecSceneObjects.push_back(pCube);
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



