#pragma once

#include "Mesh.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

class VulkanDevice;
class VulkanSwapChain;
class VulkanMaterial;
class VulkanTexture2D;
class VulkanGraphicsPipeline;

//---------------------------------------------------------------------------------------------------------------------
struct SkydomeShaderData
{
	SkydomeShaderData()
	{
		model			= glm::mat4(1);
		view			= glm::mat4(1);
		projection		= glm::mat4(1);
	}

	// Data Specific
	alignas(64) glm::mat4				model;
	alignas(64) glm::mat4				view;
	alignas(64) glm::mat4				projection;
};

//---------------------------------------------------------------------------------------------------------------------
struct SkydomeUniforms
{
	SkydomeUniforms()
	{
		vecBuffer.clear();
		vecMemory.clear();
	}

	void								CreateBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);

	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

	SkydomeShaderData					shaderData;

	// Vulkan Specific
	std::vector<VkBuffer>				vecBuffer;
	std::vector<VkDeviceMemory>			vecMemory;
};

//---------------------------------------------------------------------------------------------------------------------
class HDRISkydome
{
public:
	static HDRISkydome& getInstance()
	{
		static HDRISkydome instance;
		return instance;
	}

	~HDRISkydome();

	void								LoadSkydome(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void								UpdateUniformBUffers(VulkanDevice* pDevice, uint32_t index);
	void								Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt);
	void								Render(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipeline, uint32_t index);
	void								SetupDescriptors(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

private:
	HDRISkydome();

	std::vector<Mesh>					LoadNode(VulkanDevice* device, aiNode* node, const aiScene* scene);
	Mesh								LoadMesh(VulkanDevice* device, aiMesh* mesh, const aiScene* scene);

private:
	std::vector<Mesh>					m_vecMeshes;

public:
	VkDescriptorPool					m_vkDescriptorPool;					
	VkDescriptorSetLayout				m_vkDescriptorSetLayout;			
	std::vector<VkDescriptorSet>		m_vecDescriptorSet;					// combination of sets of uniforms & samplers per swapchain image!

	SkydomeUniforms*					m_pSkydomeUniforms;
	VulkanTexture2D*					m_pHDRI;
};

