
#include "PlaygroundPCH.h"
#include "Engine/Helpers/Utility.h"
#include "Engine/Helpers/Camera.h"
#include "Engine/Renderer/VulkanDevice.h"
#include "Engine/Renderer/VulkanSwapChain.h"
#include "Engine/Renderer/VulkanMaterial.h"
#include "Engine/Renderer/VulkanTexture2D.h"
#include "Engine/Renderer/VulkanGraphicsPipeline.h"

#include "Engine/ImGui/imgui.h"
#include "Model.h"

//---------------------------------------------------------------------------------------------------------------------
Model::Model(ModelType typeID)
{
	m_eType = typeID;
	
	m_vecMeshes.clear();

	m_pMaterial = nullptr;

	m_pShaderUniforms = new ShaderUniforms();

	m_vecPosition = glm::vec3(0);
	m_vecRotationAxis = glm::vec3(0, 1, 0);
	m_vecScale = glm::vec3(1);
	m_fAngle = 0.0f;
	m_fCurrentAngle = 0.0f;
	m_bAutoRotate = false;
	m_fAutoRotateSpeed = 1.0f;
	
	m_mapTextures.clear();

	m_vkDescriptorPool = VK_NULL_HANDLE;
	m_vkDescriptorSetLayout = VK_NULL_HANDLE;
	m_vecDescriptorSet.clear();
}

//---------------------------------------------------------------------------------------------------------------------
Model::~Model()
{
	m_mapTextures.clear();
	m_vecMeshes.clear();

	SAFE_DELETE(m_pShaderUniforms);
	SAFE_DELETE(m_pMaterial);
}

//---------------------------------------------------------------------------------------------------------------------
std::vector<Mesh> Model::LoadModel(VulkanDevice* device, const std::string& filePath)
{
	LOG_DEBUG("Loading {0} Model...", filePath);
	
	// Import Model scene
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);
	if (!scene)
		LOG_CRITICAL("Failed to Assimp ReadFile {0} model!", filePath);

	// Get list of textures based on materials!
	LoadMaterials(device, scene);

	return LoadNode(device, scene->mRootNode, scene);
}

//---------------------------------------------------------------------------------------------------------------------
void Model::UpdateUniformBuffers(VulkanDevice* pDevice, uint32_t index)
{
	// Copy View Projection data
	void* data;
	vkMapMemory(pDevice->m_vkLogicalDevice, m_pShaderUniforms->vecMemory[index], 0, sizeof(ShaderData), 0, &data);
	memcpy(data, &m_pShaderUniforms->shaderData, sizeof(ShaderData));
	vkUnmapMemory(pDevice->m_vkLogicalDevice, m_pShaderUniforms->vecMemory[index]);
}

//---------------------------------------------------------------------------------------------------------------------
std::vector<Mesh> Model::LoadNode(VulkanDevice* device, aiNode* node, const aiScene* scene)
{
	// Go through each mesh at this node & create it, then add it to our mesh list
	for (uint64_t i = 0; i < node->mNumMeshes; i++)
	{
		m_vecMeshes.push_back(LoadMesh(device,
								scene->mMeshes[node->mMeshes[i]],
								scene));
	}

	// Go through each node attached to this node & load it, then append their meshes to this node's mesh list
	for (uint64_t i = 0; i < node->mNumChildren; i++)
	{
		LoadNode(device, node->mChildren[i], scene);
		//m_vecMeshes.insert(m_vecMeshes.end(), newList.begin(), newList.end());
	}

	return m_vecMeshes;
}

//---------------------------------------------------------------------------------------------------------------------
void Model::SetDefaultValues(aiTextureType eType)
{
	// if some texture is missing, we still allow to load the default texture to maintain proper descriptor bindings
	// in shader. Else we would need to manage that dynamically. In shader, for now, we are using boolean flag to decide
	// if we read from texture or use the color from Editor! 
	switch (eType)
	{
		case aiTextureType_BASE_COLOR:
		{
			m_pShaderUniforms->shaderData.hasTextureAEN.r = 0.0f;
			m_mapTextures.emplace("MissingAlbedo.png", TextureType::TEXTURE_ALBEDO);
			LOG_ERROR("BaseColor texture not found, using default texture!");
			break;
		}
		
		case aiTextureType_EMISSION_COLOR:
		{
			m_pShaderUniforms->shaderData.hasTextureAEN.g = 0.0f;
			m_mapTextures.emplace("MissingEmissive.png", TextureType::TEXTURE_EMISSIVE);
			LOG_ERROR("Emissive texture not found, using default texture!");
			break;
		}

		case aiTextureType_NORMAL_CAMERA:
		{
			m_pShaderUniforms->shaderData.hasTextureAEN.b = 0.0f;
			m_mapTextures.emplace("MissingNormal.png", TextureType::TEXTURE_NORMAL);
			LOG_ERROR("Normal texture not found, using default texture!");
			break;
		}

		case aiTextureType_DIFFUSE_ROUGHNESS:
		{
			m_pShaderUniforms->shaderData.hasTextureRMO.r = 0.0f;
			m_mapTextures.emplace("MissingRoughness.png", TextureType::TEXTURE_ROUGHNESS);
			LOG_ERROR("Roughness texture not found, using default texture!");
			break;
		}
		
		case aiTextureType_METALNESS:
		{
			m_pShaderUniforms->shaderData.hasTextureRMO.g = 0.0f;
			m_mapTextures.emplace("MissingMetalness.png", TextureType::TEXTURE_METALNESS);
			LOG_ERROR("Metalness texture not found, using default texture!");
			break;
		}
		
		case aiTextureType_AMBIENT_OCCLUSION:
		{
			m_pShaderUniforms->shaderData.hasTextureRMO.b = 0.0f;
			m_mapTextures.emplace("MissingAO.png", TextureType::TEXTURE_AO);
			LOG_ERROR("AO texture not found, using default texture!");
			break;
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
void Model::ExtractTextureFromMaterial(aiMaterial* pMaterial, aiTextureType eType)
{
	if (pMaterial->GetTextureCount(eType))
	{
		// get the path of the texture file
		aiString path;
		if (pMaterial->GetTexture(eType, 0, &path) == AI_SUCCESS)
		{
			// cut off any directory information already present
			int idx = std::string(path.data).rfind("\\");
			std::string fileName = std::string(path.data).substr(idx + 1);

			// Inside shader, if texture is available then we sample texture to get color values else use Color values
			// provided. For roughness, metalness & AO property, we simply multiply texture color * editor value! 
			if(!fileName.empty())
			{
				switch (eType)
				{
					case aiTextureType_BASE_COLOR:
					{
						m_pShaderUniforms->shaderData.hasTextureAEN.r = 1.0f;
						m_mapTextures.emplace(fileName, TextureType::TEXTURE_ALBEDO);
						break;
					}

					case aiTextureType_EMISSION_COLOR:
					{
						m_pShaderUniforms->shaderData.hasTextureAEN.g = 1.0f;
						m_mapTextures.emplace(fileName, TextureType::TEXTURE_EMISSIVE);
						break;
					}

					case aiTextureType_NORMAL_CAMERA:
					{
						m_pShaderUniforms->shaderData.hasTextureAEN.b = 1.0f;
						m_mapTextures.emplace(fileName, TextureType::TEXTURE_NORMAL);
						break;
					}

					case aiTextureType_DIFFUSE_ROUGHNESS:
					{
						m_pShaderUniforms->shaderData.hasTextureRMO.r = 1.0f;
						m_mapTextures.emplace(fileName, TextureType::TEXTURE_ROUGHNESS);
						break;
					}
					
					case aiTextureType_METALNESS:
					{
						m_pShaderUniforms->shaderData.hasTextureRMO.g = 1.0f;
						m_mapTextures.emplace(fileName, TextureType::TEXTURE_METALNESS);
						break;
					}
					
					case aiTextureType_AMBIENT_OCCLUSION:
					{
						m_pShaderUniforms->shaderData.hasTextureRMO.b = 1.0f;
						m_mapTextures.emplace(fileName, TextureType::TEXTURE_AO);
						break;
					}

					
				}
			}
			else
			{
				// If due to some reasons, texture slot is assigned but no filename is mentioned, load default!
				SetDefaultValues(eType);
			}
		}
	}
	else
	{
		// if particular type texture is not available, load default!
		SetDefaultValues(eType);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void Model::LoadMaterials(VulkanDevice* pDevice, const aiScene* scene)
{
	// Go through each material and copy its texture file name
	for (uint32_t i = 0; i < scene->mNumMaterials; i++)
	{
		// Get the material
		aiMaterial* material = scene->mMaterials[i];
		
		// We use Maya's Stingray PBS material for mapping following textures!
		ExtractTextureFromMaterial(material, aiTextureType_BASE_COLOR);
		ExtractTextureFromMaterial(material, aiTextureType_NORMAL_CAMERA);
		ExtractTextureFromMaterial(material, aiTextureType_EMISSION_COLOR);
		ExtractTextureFromMaterial(material, aiTextureType_METALNESS);
		ExtractTextureFromMaterial(material, aiTextureType_DIFFUSE_ROUGHNESS);
		ExtractTextureFromMaterial(material, aiTextureType_AMBIENT_OCCLUSION);
	}

	m_pMaterial = new VulkanMaterial();

	std::map<std::string, TextureType>::iterator iter = m_mapTextures.begin();
	for (; iter != m_mapTextures.end(); ++iter)
	{
		m_pMaterial->LoadTexture(pDevice, iter->first, iter->second);
	}
}

//---------------------------------------------------------------------------------------------------------------------
Mesh Model::LoadMesh(VulkanDevice* pDevice, aiMesh* mesh, const aiScene* scene)
{
	std::vector<Helper::App::VertexPNTBT>	vertices;
	std::vector<uint32_t>					indices;
	std::vector<uint32_t>					textureIDs;

	vertices.resize(mesh->mNumVertices);

	// Loop through each vertex...
	for (uint64_t i = 0; i < mesh->mNumVertices; i++)
	{
		// Set position
		vertices[i].Position = { mesh->mVertices[i].x, mesh->mVertices[i].y,  mesh->mVertices[i].z };

		// Set Normals
		vertices[i].Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

		if (mesh->mTangents || mesh->mBitangents)
		{
			vertices[i].Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
			vertices[i].BiNormal = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
		}

		// Set texture coords (if they exists)
		if (mesh->mTextureCoords[0])
		{
			vertices[i].UV = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}
		else
		{
			vertices[i].UV = { 0.0f, 0.0f };
		}
	}

	// iterate over indices thorough faces for index data...
	for (uint64_t i = 0; i < mesh->mNumFaces; i++)
	{
		// Get a face
		aiFace face = mesh->mFaces[i];

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
void Model::Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt)
{
	// Update angle
	if (m_bAutoRotate)
	{
		m_fCurrentAngle += dt * m_fAutoRotateSpeed;
		if (m_fCurrentAngle > 360.0f) { m_fCurrentAngle = 0.0f; }
	}
	
	m_fAngle = m_fCurrentAngle;

	// Update Model matrix!
	m_pShaderUniforms->shaderData.model = glm::mat4(1);
	m_pShaderUniforms->shaderData.model = glm::translate(m_pShaderUniforms->shaderData.model, m_vecPosition);
	m_pShaderUniforms->shaderData.model = glm::rotate(m_pShaderUniforms->shaderData.model, m_fAngle, m_vecRotationAxis);
	m_pShaderUniforms->shaderData.model = glm::scale(m_pShaderUniforms->shaderData.model, m_vecScale);

	// Fetch View & Projection matrices from the Camera!	
	m_pShaderUniforms->shaderData.projection = Camera::getInstance().m_matProjection;

	m_pShaderUniforms->shaderData.view = Camera::getInstance().m_matView;
	m_pShaderUniforms->shaderData.projection[1][1] *= -1.0f;

	// Update object ID
	m_pShaderUniforms->shaderData.objectID = static_cast<uint32_t>(m_eType);
}

//---------------------------------------------------------------------------------------------------------------------
void Model::Render (VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipeline, uint32_t index)
{
	for (int i = 0; i < m_vecMeshes.size(); ++i)
	{

		VkBuffer vertexBuffers[] = { m_vecMeshes[i].m_vkVertexBuffer };										// Buffers to bind
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
void Model::SetupDescriptors(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	m_pShaderUniforms->CreateBuffers(pDevice, pSwapchain);

	// *** Create Descriptor pool
	std::array<VkDescriptorPoolSize, 2> arrDescriptorPoolSize = {};

	//-- Uniform Buffer
	arrDescriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	arrDescriptorPoolSize[0].descriptorCount = static_cast<uint32_t>(pSwapchain->m_vecSwapchainImages.size());

	//-- Texture samplers
	arrDescriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorPoolSize[1].descriptorCount = static_cast<uint32_t>(m_mapTextures.size());

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(m_mapTextures.size()) + static_cast<uint32_t>(pSwapchain->m_vecSwapchainImages.size());
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(arrDescriptorPoolSize.size());
	poolCreateInfo.pPoolSizes = arrDescriptorPoolSize.data();

	if (vkCreateDescriptorPool(pDevice->m_vkLogicalDevice, &poolCreateInfo, nullptr, &m_vkDescriptorPool) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Sampler Descriptor Pool");
	}
	else
		LOG_DEBUG("Successfully created Descriptor Pool");

	// *** Create Descriptor Set Layout
	std::array<VkDescriptorSetLayoutBinding, 7> arrDescriptorSetLayoutBindings = {};

	//-- Uniform Buffer
	arrDescriptorSetLayoutBindings[0].binding = 0;																// binding point in shader, binding = ?
	arrDescriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;						// type of descriptor (uniform, dynamic uniform etc.) 
	arrDescriptorSetLayoutBindings[0].descriptorCount = 1;														// number of descriptors
	arrDescriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;	// Shader stage to bind to
	arrDescriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;												// For textures!

	//-- BaseColor Texture
	arrDescriptorSetLayoutBindings[1].binding = 1;
	arrDescriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;						
	arrDescriptorSetLayoutBindings[1].descriptorCount = 1;														
	arrDescriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;									
	arrDescriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;

	//-- Metalness Texture
	arrDescriptorSetLayoutBindings[2].binding = 2;
	arrDescriptorSetLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorSetLayoutBindings[2].descriptorCount = 1;
	arrDescriptorSetLayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	arrDescriptorSetLayoutBindings[2].pImmutableSamplers = nullptr;

	//-- Normal Texture
	arrDescriptorSetLayoutBindings[3].binding = 3;
	arrDescriptorSetLayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorSetLayoutBindings[3].descriptorCount = 1;
	arrDescriptorSetLayoutBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	arrDescriptorSetLayoutBindings[3].pImmutableSamplers = nullptr;

	//-- Roughness Texture
	arrDescriptorSetLayoutBindings[4].binding = 4;
	arrDescriptorSetLayoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorSetLayoutBindings[4].descriptorCount = 1;
	arrDescriptorSetLayoutBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	arrDescriptorSetLayoutBindings[4].pImmutableSamplers = nullptr;

	//-- AO Texture
	arrDescriptorSetLayoutBindings[5].binding = 5;
	arrDescriptorSetLayoutBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorSetLayoutBindings[5].descriptorCount = 1;
	arrDescriptorSetLayoutBindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	arrDescriptorSetLayoutBindings[5].pImmutableSamplers = nullptr;

	//-- Emission Texture
	arrDescriptorSetLayoutBindings[6].binding = 6;
	arrDescriptorSetLayoutBindings[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorSetLayoutBindings[6].descriptorCount = 1;
	arrDescriptorSetLayoutBindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	arrDescriptorSetLayoutBindings[6].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descSetlayoutCreateInfo = {};
	descSetlayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetlayoutCreateInfo.bindingCount = arrDescriptorSetLayoutBindings.size();
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
		ubBufferInfo.buffer = m_pShaderUniforms->vecBuffer[i];			// buffer to get data from
		ubBufferInfo.offset = 0;											// position of start of data
		ubBufferInfo.range = sizeof(ShaderUniforms);						// size of data

		// Data about connection between binding & buffer
		VkWriteDescriptorSet ubSetWrite = {};
		ubSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		ubSetWrite.dstSet = m_vecDescriptorSet[i];							// Descriptor set to update
		ubSetWrite.dstBinding = 0;											// binding to update
		ubSetWrite.dstArrayElement = 0;										// index in array to update
		ubSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;		// type of descriptor
		ubSetWrite.descriptorCount = 1;										// amount to update		
		ubSetWrite.pBufferInfo = &ubBufferInfo;
		
		//-- Base Texture
		VkDescriptorImageInfo albedoImageInfo = {};
		albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;											// Image layout when in use
		albedoImageInfo.imageView = m_pMaterial->m_mapTextures.at(TextureType::TEXTURE_ALBEDO)->m_vkTextureImageView;	// image to bind to set
		albedoImageInfo.sampler = m_pMaterial->m_mapTextures.at(TextureType::TEXTURE_ALBEDO)->m_vkTextureSampler;		// sampler to use for the set

		// Descriptor write info
		VkWriteDescriptorSet albedoSetWrite = {};
		albedoSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		albedoSetWrite.dstSet = m_vecDescriptorSet[i];
		albedoSetWrite.dstBinding = 1;
		albedoSetWrite.dstArrayElement = 0;
		albedoSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		albedoSetWrite.descriptorCount = 1;
		albedoSetWrite.pImageInfo = &albedoImageInfo;

		//-- Metalness Texture
		VkDescriptorImageInfo metalnessImageInfo = {};
		metalnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;											// Image layout when in use
		metalnessImageInfo.imageView = m_pMaterial->m_mapTextures.at(TextureType::TEXTURE_METALNESS)->m_vkTextureImageView;	// image to bind to set
		metalnessImageInfo.sampler = m_pMaterial->m_mapTextures.at(TextureType::TEXTURE_METALNESS)->m_vkTextureSampler;		// sampler to use for the set

		// Descriptor write info
		VkWriteDescriptorSet metalnessSetWrite = {};
		metalnessSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		metalnessSetWrite.dstSet = m_vecDescriptorSet[i];
		metalnessSetWrite.dstBinding = 2;
		metalnessSetWrite.dstArrayElement = 0;
		metalnessSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		metalnessSetWrite.descriptorCount = 1;
		metalnessSetWrite.pImageInfo = &metalnessImageInfo;

		//-- Normal Texture
		VkDescriptorImageInfo normalImageInfo = {};
		normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;											// Image layout when in use
		normalImageInfo.imageView = m_pMaterial->m_mapTextures.at(TextureType::TEXTURE_NORMAL)->m_vkTextureImageView;	// image to bind to set
		normalImageInfo.sampler = m_pMaterial->m_mapTextures.at(TextureType::TEXTURE_NORMAL)->m_vkTextureSampler;		// sampler to use for the set

		// Descriptor write info
		VkWriteDescriptorSet normalSetWrite = {};
		normalSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		normalSetWrite.dstSet = m_vecDescriptorSet[i];
		normalSetWrite.dstBinding = 3;
		normalSetWrite.dstArrayElement = 0;
		normalSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalSetWrite.descriptorCount = 1;
		normalSetWrite.pImageInfo = &normalImageInfo;

		//-- Roughness Texture
		VkDescriptorImageInfo roughnessImageInfo = {};
		roughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;											// Image layout when in use
		roughnessImageInfo.imageView = m_pMaterial->m_mapTextures.at(TextureType::TEXTURE_ROUGHNESS)->m_vkTextureImageView;	// image to bind to set
		roughnessImageInfo.sampler = m_pMaterial->m_mapTextures.at(TextureType::TEXTURE_ROUGHNESS)->m_vkTextureSampler;		// sampler to use for the set

		// Descriptor write info
		VkWriteDescriptorSet roughnessSetWrite = {};
		roughnessSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		roughnessSetWrite.dstSet = m_vecDescriptorSet[i];
		roughnessSetWrite.dstBinding = 4;
		roughnessSetWrite.dstArrayElement = 0;
		roughnessSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		roughnessSetWrite.descriptorCount = 1;
		roughnessSetWrite.pImageInfo = &roughnessImageInfo;

		//-- AO Texture
		VkDescriptorImageInfo AOImageInfo = {};
		AOImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;											// Image layout when in use
		AOImageInfo.imageView = m_pMaterial->m_mapTextures.at(TextureType::TEXTURE_AO)->m_vkTextureImageView;		// image to bind to set
		AOImageInfo.sampler = m_pMaterial->m_mapTextures.at(TextureType::TEXTURE_AO)->m_vkTextureSampler;			// sampler to use for the set

		// Descriptor write info
		VkWriteDescriptorSet AOSetWrite = {};
		AOSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		AOSetWrite.dstSet = m_vecDescriptorSet[i];
		AOSetWrite.dstBinding = 5;
		AOSetWrite.dstArrayElement = 0;
		AOSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		AOSetWrite.descriptorCount = 1;
		AOSetWrite.pImageInfo = &AOImageInfo;

		//-- Emission Texture
		VkDescriptorImageInfo emissionImageInfo = {};
		emissionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;											// Image layout when in use
		emissionImageInfo.imageView = m_pMaterial->m_mapTextures.at(TextureType::TEXTURE_EMISSIVE)->m_vkTextureImageView;	// image to bind to set
		emissionImageInfo.sampler = m_pMaterial->m_mapTextures.at(TextureType::TEXTURE_EMISSIVE)->m_vkTextureSampler;		// sampler to use for the set

		// Descriptor write info
		VkWriteDescriptorSet emissionSetWrite = {};
		emissionSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		emissionSetWrite.dstSet = m_vecDescriptorSet[i];
		emissionSetWrite.dstBinding = 6;
		emissionSetWrite.dstArrayElement = 0;
		emissionSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		emissionSetWrite.descriptorCount = 1;
		emissionSetWrite.pImageInfo = &emissionImageInfo;

		// List of Descriptor set writes
		std::vector<VkWriteDescriptorSet> setWrites = { ubSetWrite, albedoSetWrite, metalnessSetWrite, normalSetWrite,
														roughnessSetWrite, AOSetWrite, emissionSetWrite };
		
		// Update the descriptor sets with new buffer/binding info
		vkUpdateDescriptorSets(pDevice->m_vkLogicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void Model::Cleanup(VulkanDevice* pDevice)
{
	m_pShaderUniforms->Cleanup(pDevice);
	m_pMaterial->Cleanup(pDevice);

	std::vector<Mesh>::iterator iter = m_vecMeshes.begin();
	for (; iter != m_vecMeshes.end(); iter++)
	{
		(*iter).Cleanup(pDevice);
	}

	vkDestroyDescriptorPool(pDevice->m_vkLogicalDevice, m_vkDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(pDevice->m_vkLogicalDevice, m_vkDescriptorSetLayout, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void Model::CleanupOnWindowResize(VulkanDevice* pDevice)
{

}

//---------------------------------------------------------------------------------------------------------------------
void ShaderUniforms::CreateBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain)
{
	// ViewProjection buffer size
	VkDeviceSize vpBufferSize = sizeof(ShaderUniforms);

	// one uniform buffer for each image (and by extension command buffer)
	vecBuffer.resize(pSwapChain->m_vecSwapchainImages.size());
	vecMemory.resize(pSwapChain->m_vecSwapchainImages.size());

	// create uniform buffers
	for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
	{
		pDevice->CreateBuffer(vpBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vecBuffer[i],
			&vecMemory[i]);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void ShaderUniforms::Cleanup(VulkanDevice* pDevice)
{
	for (uint16_t i = 0; i < vecBuffer.size(); ++i)
	{
		vkDestroyBuffer(pDevice->m_vkLogicalDevice, vecBuffer[i], nullptr);
		vkFreeMemory(pDevice->m_vkLogicalDevice, vecMemory[i], nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void ShaderUniforms::CleanupOnWindowResize(VulkanDevice* pDevice)
{

}


