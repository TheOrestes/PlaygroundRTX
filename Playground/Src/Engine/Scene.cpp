#include "PlaygroundPCH.h"
#include "Scene.h"

#include "glm/gtc/matrix_access.hpp"

#include "Renderer/VulkanDevice.h"
#include "Renderer/VulkanSwapChain.h"
#include "Renderer/VulkanGraphicsPipeline.h"

#include "Engine/RenderObjects/HDRISkydome.h"
#include "Engine/RenderObjects/Model.h"

//---------------------------------------------------------------------------------------------------------------------
Scene::Scene()
{
	m_vecModels.clear();
}

//---------------------------------------------------------------------------------------------------------------------
Scene::~Scene()
{
	m_vecModels.clear();
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
	// Load Gun Model
	//Model* pModelGun = new Model(ModelType::STATIC_OPAQUE);
	//pModelGun->LoadModel(pDevice, "Models/Gun.fbx");
	//pModelGun->SetPosition(glm::vec3(0, 2, 0));
	//pModelGun->SetScale(glm::vec3(2.0f));
	//pModelGun->SetupDescriptors(pDevice, pSwapchain);
	//
	//m_vecModels.push_back(pModelGun);

	// Load AntMan Model
	Model* pModelAnt = new Model(ModelType::STATIC_OPAQUE);
	pModelAnt->LoadModel(pDevice, "Assets/Models/AntMan.fbx");
	pModelAnt->SetPosition(glm::vec3(0, 0, 0));
	pModelAnt->SetScale(glm::vec3(1.0f));
	pModelAnt->SetupDescriptors(pDevice, pSwapchain);
	
	m_vecModels.push_back(pModelAnt);

	// Load Leather Sphere
	//Model* pModelSphereLeather = new Model(ModelType::STATIC_OPAQUE);
	//pModelSphereLeather->LoadModel(pDevice, "Assets/Models/Sphere.fbx");
	//pModelSphereLeather->SetPosition(glm::vec3(0, 1, 0));
	//pModelSphereLeather->SetScale(glm::vec3(1.0f));
	//pModelSphereLeather->SetupDescriptors(pDevice, pSwapchain);
	//
	//m_vecModels.push_back(pModelSphereLeather);

	// Load Color Sphere
	//Model* pModelSphereColor = new Model(ModelType::STATIC_OPAQUE);
	//pModelSphereColor->LoadModel(pDevice, "Assets/Models/Sphere_Color.fbx");
	//pModelSphereColor->SetPosition(glm::vec3(5, 2.5, 0));
	//pModelSphereColor->SetScale(glm::vec3(1.5f));
	//pModelSphereColor->SetupDescriptors(pDevice, pSwapchain);
	//
	//m_vecModels.push_back(pModelSphereColor);

	// Load Rust Sphere
	//Model* pModelSphereRust = new Model(ModelType::STATIC_OPAQUE);
	//pModelSphereRust->LoadModel(pDevice, "Assets/Models/Sphere_Rust.fbx");
	//pModelSphereRust->SetPosition(glm::vec3(-5, 2.5, 0));
	//pModelSphereRust->SetScale(glm::vec3(1.0f));
	//pModelSphereRust->SetupDescriptors(pDevice, pSwapchain);
	//
	//m_vecModels.push_back(pModelSphereRust);

	// Load WoodenFloor Model
	Model* pWoodenFloor = new Model(ModelType::STATIC_OPAQUE);
	pWoodenFloor->LoadModel(pDevice, "Assets/Models/Plane_Oak.fbx");
	pWoodenFloor->SetPosition(glm::vec3(0, -2, 0));
	pWoodenFloor->SetScale(glm::vec3(4));
	pWoodenFloor->SetupDescriptors(pDevice, pSwapchain);
	pWoodenFloor->CreateBottomLevelAS(pDevice);
	
	m_vecModels.push_back(pWoodenFloor);
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt)
{
	// Update each model's uniform data
	for (Model* element : m_vecModels)
	{
		if (element != nullptr)
		{
			element->Update(pDevice, pSwapchain, dt);
		}
	}

	// Update Skydome data!
	HDRISkydome::getInstance().Update(pDevice, pSwapchain, dt);
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::UpdateUniforms(VulkanDevice* pDevice, uint32_t imageIndex)
{
	// Update all models!
	for (Model* element :m_vecModels)
	{
		if (element != nullptr)
		{
			element->UpdateUniformBuffers(pDevice, imageIndex);
		}
	}

	// Update Skydome uniforms!
	HDRISkydome::getInstance().UpdateUniformBUffers(pDevice, imageIndex);
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::RenderOpaque(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipline, uint32_t imageIndex)
{
	// Draw Scene!
	for (Model* element : m_vecModels)
	{
		if (element != nullptr)
		{
			element->Render(pDevice, pPipline, imageIndex);
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::RenderSkydome(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipline, uint32_t imageIndex)
{
	HDRISkydome::getInstance().Render(pDevice, pPipline, imageIndex);
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
	for (Model* element : m_vecModels)
	{
		element->Cleanup(pDevice);
	}	
}



