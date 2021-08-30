#include "PlaygroundPCH.h"
#include "HDRISkydome.h"

#include "Engine/Helpers/Utility.h"
#include "Engine/Helpers/Camera.h"
#include "Engine/Renderer/VulkanDevice.h"
#include "Engine/Renderer/VulkanSwapChain.h"
#include "Engine/Renderer/VulkanMaterial.h"
#include "Engine/Renderer/VulkanTexture2D.h"
#include "Engine/Renderer/VulkanGraphicsPipeline.h"
#include "Model.h"

//---------------------------------------------------------------------------------------------------------------------
HDRISkydome::HDRISkydome()
{
	m_vecMeshes.clear();

    m_pSkydomeUniforms			= nullptr;
	m_pHDRI						= nullptr;

    m_vkDescriptorPool			= VK_NULL_HANDLE;
    m_vkDescriptorSetLayout		= VK_NULL_HANDLE;
    m_vecDescriptorSet.clear();
}

//---------------------------------------------------------------------------------------------------------------------
HDRISkydome::~HDRISkydome()
{
	m_vecMeshes.clear();

    SAFE_DELETE(m_pSkydomeUniforms);
	SAFE_DELETE(m_pHDRI);
}

//---------------------------------------------------------------------------------------------------------------------
void HDRISkydome::LoadSkydome(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	// Create HDRI texture
	m_pHDRI = new VulkanTexture2D();
	m_pHDRI->CreateTexture(pDevice, "old_hall_2k.hdr", TextureType::TEXTURE_HDRI);

    LOG_DEBUG("Loading Skydome Model...");

    // Import Model scene
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("Assets/Models/SkySphere.fbx", aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if (!scene)
        LOG_CRITICAL("Failed to load Skydome model!");

	SetupDescriptors(pDevice, pSwapchain);

    LoadNode(pDevice, scene->mRootNode, scene);
}

//---------------------------------------------------------------------------------------------------------------------
std::vector<Mesh> HDRISkydome::LoadNode(VulkanDevice* pDevice, aiNode* pNode, const aiScene* pScene)
{
	// Go through each mesh at this node & create it, then add it to our mesh list
	for (uint64_t i = 0; i < pNode->mNumMeshes; i++)
	{
		m_vecMeshes.push_back(LoadMesh(pDevice, pScene->mMeshes[pNode->mMeshes[i]], pScene));
	}

	// Go through each node attached to this node & load it, then append their meshes to this node's mesh list
	for (uint64_t i = 0; i < pNode->mNumChildren; i++)
	{
		LoadNode(pDevice, pNode->mChildren[i], pScene);
	}

	return m_vecMeshes;
}

//---------------------------------------------------------------------------------------------------------------------
Mesh HDRISkydome::LoadMesh(VulkanDevice* pDevice, aiMesh* pMesh, const aiScene* pScene)
{
    std::vector<App::VertexPNT>	    vertices;
	vertices.resize(pMesh->mNumVertices);

    std::vector<uint32_t>					indices;

	// Loop through each vertex...
	for (uint64_t i = 0; i < pMesh->mNumVertices; i++)
	{
		// Set position
		vertices[i].Position = glm::vec3( pMesh->mVertices[i].x, pMesh->mVertices[i].y,  pMesh->mVertices[i].z );

		// Set Normals
		vertices[i].Normal = glm::vec3( pMesh->mNormals[i].x, pMesh->mNormals[i].y, pMesh->mNormals[i].z );

		// Set texture coords (if they exists)
		if (pMesh->mTextureCoords[0])
		{
			vertices[i].UV = glm::vec2( pMesh->mTextureCoords[0][i].x, pMesh->mTextureCoords[0][i].y );
		}
		else
		{
			vertices[i].UV = glm::vec2( 0.0f, 0.0f );
		}
	}

	// iterate over indices thorough faces for index data...
	for (uint64_t i = 0; i < pMesh->mNumFaces; i++)
	{
		// Get a face
		aiFace face = pMesh->mFaces[i];

		// go through face's indices & add to the list
		for (uint16_t j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// Create new mesh with details & return it!
	Mesh newMesh(pDevice, vertices, indices);
	return newMesh;
}

//---------------------------------------------------------------------------------------------------------------------
void HDRISkydome::UpdateUniformBUffers(VulkanDevice* pDevice, uint32_t index)
{
	// Copy View Projection data
	void* data;
	vkMapMemory(pDevice->m_vkLogicalDevice, m_pSkydomeUniforms->vecMemory[index], 0, sizeof(SkydomeShaderData), 0, &data);
	memcpy(data, &m_pSkydomeUniforms->shaderData, sizeof(SkydomeShaderData));
	vkUnmapMemory(pDevice->m_vkLogicalDevice, m_pSkydomeUniforms->vecMemory[index]);
}

//---------------------------------------------------------------------------------------------------------------------
void HDRISkydome::Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt)
{
	// Update Model matrix!
	m_pSkydomeUniforms->shaderData.model = glm::mat4(1);

	// Fetch View & Projection matrices from the Camera!	
	m_pSkydomeUniforms->shaderData.projection = Camera::getInstance().m_matProjection;

	m_pSkydomeUniforms->shaderData.view = Camera::getInstance().m_matView;
	m_pSkydomeUniforms->shaderData.projection[1][1] *= -1.0f;
}

//---------------------------------------------------------------------------------------------------------------------
void HDRISkydome::Render(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipeline, uint32_t index)
{
	for (int i = 0; i < m_vecMeshes.size(); ++i)
	{

		VkBuffer vertexBuffers[] = { m_vecMeshes[i].m_vkVertexBuffer };											// Buffers to bind
		VkBuffer indexBuffer = m_vecMeshes[i].m_vkIndexBuffer;
		VkDeviceSize offsets[] = { 0 };																			// offsets into buffers being bound
		vkCmdBindVertexBuffers(pDevice->m_vecCommandBufferGraphics[index], 0, 1, vertexBuffers, offsets);		// Command to bind vertex buffer before drawing with them

		// bind mesh index buffer, with zero offset & using uint32_t type
		vkCmdBindIndexBuffer(pDevice->m_vecCommandBufferGraphics[index], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// bind descriptor sets
		vkCmdBindDescriptorSets(pDevice->m_vecCommandBufferGraphics[index],
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								pPipeline->m_vkPipelineLayout,
								0,
								1,
								&(m_vecDescriptorSet[index]),
								0,
								nullptr);

		// Execute pipeline
		vkCmdDrawIndexed(pDevice->m_vecCommandBufferGraphics[index], m_vecMeshes[i].m_uiIndexCount, 1, 0, 0, 0);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void HDRISkydome::SetupDescriptors(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	m_pSkydomeUniforms = new SkydomeUniforms();
	m_pSkydomeUniforms->CreateBuffers(pDevice, pSwapchain);

	// *** Create Descriptor pool
	std::array<VkDescriptorPoolSize, 2> arrDescriptorPoolSize = {};

	//-- Uniform Buffer
	arrDescriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	arrDescriptorPoolSize[0].descriptorCount = static_cast<uint32_t>(pSwapchain->m_vecSwapchainImages.size());

	//-- HDRI sampler
	arrDescriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorPoolSize[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = 1 + static_cast<uint32_t>(pSwapchain->m_vecSwapchainImages.size());
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(arrDescriptorPoolSize.size());
	poolCreateInfo.pPoolSizes = arrDescriptorPoolSize.data();

	if (vkCreateDescriptorPool(pDevice->m_vkLogicalDevice, &poolCreateInfo, nullptr, &m_vkDescriptorPool) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Sampler Descriptor Pool");
	}
	else
		LOG_DEBUG("Successfully created Descriptor Pool");

	// *** Create Descriptor Set Layout
	std::array<VkDescriptorSetLayoutBinding, 2> arrDescriptorSetLayoutBindings = {};

	//-- Uniform Buffer
	arrDescriptorSetLayoutBindings[0].binding = 0;																// binding point in shader, binding = ?
	arrDescriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;						// type of descriptor (uniform, dynamic uniform etc.) 
	arrDescriptorSetLayoutBindings[0].descriptorCount = 1;														// number of descriptors
	arrDescriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;	// Shader stage to bind to
	arrDescriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;												// For textures!

	//-- HDRI Texture
	arrDescriptorSetLayoutBindings[1].binding = 1;
	arrDescriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorSetLayoutBindings[1].descriptorCount = 1;
	arrDescriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	arrDescriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descSetlayoutCreateInfo = {};
	descSetlayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetlayoutCreateInfo.bindingCount = static_cast<uint32_t>(arrDescriptorSetLayoutBindings.size());
	descSetlayoutCreateInfo.pBindings = arrDescriptorSetLayoutBindings.data();

	// Create descriptor set layout
	if (vkCreateDescriptorSetLayout(pDevice->m_vkLogicalDevice, &descSetlayoutCreateInfo, nullptr, &m_vkDescriptorSetLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create a Descriptor set layout");
	}
	else
		LOG_DEBUG("Successfully created a Descriptor set layout");

	// *** Create Descriptor Set per swapchain image!
	m_vecDescriptorSet.resize(pSwapchain->m_vecSwapchainImages.size());

	// we create copies of DescriptorSetLayout per swapchain image
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(pSwapchain->m_vecSwapchainImages.size(), m_vkDescriptorSetLayout);

	// Descriptor set allocation info 
	VkDescriptorSetAllocateInfo	setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = m_vkDescriptorPool;													// pool to allocate descriptor set from
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(pSwapchain->m_vecSwapchainImages.size());	// number of sets to allocate
	setAllocInfo.pSetLayouts = descriptorSetLayouts.data();												// layouts to use to allocate sets

	if (vkAllocateDescriptorSets(pDevice->m_vkLogicalDevice, &setAllocInfo, m_vecDescriptorSet.data()) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to allocated Descriptor sets");
	}
	else
		LOG_DEBUG("Successfully created Descriptor sets");


	// *** Update all the descriptor set bindings
	for (uint16_t i = 0; i < pSwapchain->m_vecSwapchainImages.size(); i++)
	{
		//-- Uniform Buffer
		VkDescriptorBufferInfo ubBufferInfo = {};
		ubBufferInfo.buffer = m_pSkydomeUniforms->vecBuffer[i];				// buffer to get data from
		ubBufferInfo.offset = 0;											// position of start of data
		ubBufferInfo.range = sizeof(SkydomeUniforms);						// size of data

		// Data about connection between binding & buffer
		VkWriteDescriptorSet ubSetWrite = {};
		ubSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		ubSetWrite.dstSet = m_vecDescriptorSet[i];							// Descriptor set to update
		ubSetWrite.dstBinding = 0;											// binding to update
		ubSetWrite.dstArrayElement = 0;										// index in array to update
		ubSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;		// type of descriptor
		ubSetWrite.descriptorCount = 1;										// amount to update		
		ubSetWrite.pBufferInfo = &ubBufferInfo;

		//-- HDRI Texture
		VkDescriptorImageInfo hdriImageInfo = {};
		hdriImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;			// Image layout when in use
		hdriImageInfo.imageView = m_pHDRI->m_vkTextureImageView;						// image to bind to set
		hdriImageInfo.sampler = m_pHDRI->m_vkTextureSampler;							// sampler to use for the set

		// Descriptor write info
		VkWriteDescriptorSet hdriSetWrite = {};
		hdriSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		hdriSetWrite.dstSet = m_vecDescriptorSet[i];
		hdriSetWrite.dstBinding = 1;
		hdriSetWrite.dstArrayElement = 0;
		hdriSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		hdriSetWrite.descriptorCount = 1;
		hdriSetWrite.pImageInfo = &hdriImageInfo;

		// List of Descriptor set writes
		std::vector<VkWriteDescriptorSet> setWrites = { ubSetWrite, hdriSetWrite };

		// Update the descriptor sets with new buffer/binding info
		vkUpdateDescriptorSets(pDevice->m_vkLogicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void HDRISkydome::Cleanup(VulkanDevice* pDevice)
{
	m_pSkydomeUniforms->Cleanup(pDevice);
	m_pHDRI->Cleanup(pDevice);

	std::vector<Mesh>::iterator iter = m_vecMeshes.begin();
	for (; iter != m_vecMeshes.end(); iter++)
	{
		(*iter).Cleanup(pDevice);
	}

	vkDestroyDescriptorPool(pDevice->m_vkLogicalDevice, m_vkDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(pDevice->m_vkLogicalDevice, m_vkDescriptorSetLayout, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void HDRISkydome::CleanupOnWindowResize(VulkanDevice* pDevice)
{
}

//---------------------------------------------------------------------------------------------------------------------
void SkydomeUniforms::CreateBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	// ViewProjection buffer size
	VkDeviceSize vpBufferSize = sizeof(SkydomeUniforms);
	
	// one uniform buffer for each image (and by extension command buffer)
	vecBuffer.resize(pSwapchain->m_vecSwapchainImages.size());
	vecMemory.resize(pSwapchain->m_vecSwapchainImages.size());
	
	// create uniform buffers
	for (uint16_t i = 0; i < pSwapchain->m_vecSwapchainImages.size(); i++)
	{
		pDevice->CreateBuffer(	vpBufferSize,
								VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
								VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
								&vecBuffer[i],
								&vecMemory[i]);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void SkydomeUniforms::Cleanup(VulkanDevice* pDevice)
{
	for (uint16_t i = 0; i < vecBuffer.size(); ++i)
	{
		vkDestroyBuffer(pDevice->m_vkLogicalDevice, vecBuffer[i], nullptr);
		vkFreeMemory(pDevice->m_vkLogicalDevice, vecMemory[i], nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void SkydomeUniforms::CleanupOnWindowResize(VulkanDevice* pDevice)
{
}
