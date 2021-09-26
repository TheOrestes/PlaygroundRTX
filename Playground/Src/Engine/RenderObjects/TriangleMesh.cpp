#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"
#include "TriangleMesh.h"

//---------------------------------------------------------------------------------------------------------------------
TriangleMesh::TriangleMesh(const std::string filepath)
{
	m_TriangleCount = 0;

    m_vecVertices.clear();
    m_vecIndices.clear();

    m_Filepath = filepath;
}

//---------------------------------------------------------------------------------------------------------------------
TriangleMesh::~TriangleMesh()
{
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleMesh::Initialize(VulkanDevice* pDevice)
{
    // Get the ray tracing & AS related function ptrs
    vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(pDevice->m_vkLogicalDevice, "vkCreateAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(pDevice->m_vkLogicalDevice, "vkGetAccelerationStructureBuildSizesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(pDevice->m_vkLogicalDevice, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(pDevice->m_vkLogicalDevice, "vkCmdBuildAccelerationStructuresKHR"));    

    LoadModel(m_Filepath);

	CreateBottomLevelAS(pDevice);
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleMesh::Update(float dt)
{
    static float angle = 0.0f;
    angle += dt;

    m_pMeshInstanceData->Update(dt);
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleMesh::Render()
{
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleMesh::Cleanup(VulkanDevice* pDevice)
{
    m_pMeshData->Cleanup(pDevice);
    m_BottomLevelAS.Cleanup(pDevice);

    m_vecVertices.clear();
    m_vecIndices.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleMesh::LoadModel(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        MessageBox(0, L"Assimp Error!", L"Error", MB_OK);
        return;
    }

    // process root node recursively!
    ProcessNode(scene->mRootNode, scene);
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleMesh::ProcessNode(aiNode* node, const aiScene* scene)
{
    // node only contains indices to actual objects in the scene. But scene,
    // conatins all the data, node is just to keep things organized.
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, scene);
    }

    // Once we have processed all the meshes, we recursively process
    // each child node
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(node->mChildren[i], scene);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleMesh::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		App::VertexP vertex;

		vertex.Position = glm::vec3(mesh->mVertices[i][0], mesh->mVertices[i][1], mesh->mVertices[i][2]);
		//vertex.normal = glm::vec3(mesh->mNormals[i][0], mesh->mNormals[i][1], mesh->mNormals[i][2]);
		//
		//if (mesh->mTextureCoords[0])
		//{
		//	//vertex.uv = glm::clamp(glm::vec2(mesh->mTextureCoords[0][i][0], mesh->mTextureCoords[0][i][1]), 0.0f, 1.0f);
		//	vertex.uv = glm::vec2(mesh->mTextureCoords[0][i][0], mesh->mTextureCoords[0][i][1]);
		//}

		m_vecVertices.push_back(vertex);
	}

	// process materials
	//if (mesh->mMaterialIndex >= 0)
	//{
	//	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	//	aiString aiName;
	//
	//	if (AI_SUCCESS == aiGetMaterialString(material, AI_MATKEY_NAME, &aiName))
	//	{
	//		std::string name = aiName.C_Str();
	//
	//		if (name.find("lambert") != std::string::npos)
	//		{
	//			// Extract texture info if filepath or flat color?
	//			Texture* textureInfo = nullptr;
	//
	//			// Look if material has texture info...
	//			aiString path;
	//			if (AI_SUCCESS == aiGetMaterialTexture(material, aiTextureType_DIFFUSE, 0, &path))
	//			{
	//				std::string filePath = std::string(path.C_Str());
	//				textureInfo = new ImageTexture("models/" + filePath);
	//				m_ptrMaterial = new Lambertian(textureInfo);
	//			}
	//			else
	//			{
	//				// Check if we have set albedo color explicitly or not, if not then use Maya's 
	//				// set color from the properties!
	//				glm::vec4 albedoCol = m_ptrMeshInfo->matInfo.albedoColor;
	//				if (glm::length(albedoCol) == 0)
	//				{
	//					aiColor4D diffuseColor;
	//					aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor);
	//					albedoCol = glm::vec4(diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);
	//				}
	//
	//				if (m_ptrMeshInfo->isLightSource)
	//				{
	//					textureInfo = new ConstantTexture(albedoCol);
	//					m_ptrMaterial = new DiffuseLight(textureInfo);
	//				}
	//				else
	//				{
	//					textureInfo = new ConstantTexture(albedoCol);
	//					m_ptrMaterial = new Lambertian(textureInfo);
	//				}
	//			}
	//		}
	//		else if (name.find("metal") != std::string::npos)
	//		{
	//			// Extract texture info if filepath or flat color?
	//			Texture* textureInfo = nullptr;
	//			float roughness = m_ptrMeshInfo->matInfo.roughness;
	//
	//			// Look if material has texture info...
	//			aiString path;
	//			if (AI_SUCCESS == aiGetMaterialTexture(material, aiTextureType_DIFFUSE, 0, &path))
	//			{
	//				std::string filePath = std::string(path.C_Str());
	//				textureInfo = new ImageTexture("models/" + filePath);
	//				m_ptrMaterial = new Metal(textureInfo, roughness);
	//			}
	//			else
	//			{
	//				// Check if we have set albedo color explicitly or not, if not then use Maya's 
	//				// set color from the properties!
	//				glm::vec4 albedoCol = m_ptrMeshInfo->matInfo.albedoColor;
	//				if (glm::length(albedoCol) == 0)
	//				{
	//					aiColor4D diffuseColor;
	//					aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor);
	//					albedoCol = glm::vec4(diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);
	//				}
	//
	//				textureInfo = new ConstantTexture(albedoCol);
	//				m_ptrMaterial = new Metal(textureInfo, roughness);
	//			}
	//		}
	//		else if (name.find("transparent") != std::string::npos)
	//		{
	//			// Extract texture info if filepath or flat color?
	//			Texture* textureInfo = nullptr;
	//			float r_i = m_ptrMeshInfo->matInfo.refrIndex;
	//
	//			// Check if we have set albedo color explicitly or not, if not then use Maya's 
	//			// set color from the properties!
	//			glm::vec4 albedoCol = m_ptrMeshInfo->matInfo.albedoColor;
	//			if (glm::length(albedoCol) == 0)
	//			{
	//				aiColor4D diffuseColor;
	//				aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor);
	//				albedoCol = glm::vec4(diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);
	//			}
	//
	//			textureInfo = new ConstantTexture(albedoCol);
	//			m_ptrMaterial = new Transparent(textureInfo, r_i);
	//		}
	//		else
	//		{
	//			MessageBox(0, L"Unknown Material", L"Error", MB_OK);
	//			return;
	//		}
	//	}
	//}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		// Get a face
		aiFace face = mesh->mFaces[i];

		// go through face's indices & add to the list
		for (uint16_t j = 0; j < face.mNumIndices; j++)
		{
			m_vecIndices.push_back(face.mIndices[j]);
		}

		// Update the triangle count!
		++m_TriangleCount;
	}

	//AABB box;
	//for (unsigned int i = 0; i < m_vecTriangles.size(); i++)
	//{
	//	m_vecTriangles[i]->BoundingBox(box);
	//	m_ptrAABB->ExpandBoundingBox(box);
	//}
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleMesh::CreateBottomLevelAS(VulkanDevice* pDevice)
{	  
	m_pMeshData = new Vulkan::MeshData();
	m_pMeshInstanceData = new Vulkan::MeshInstance();

    // 2. Create buffers for Mesh Data
    // Create VB
    pDevice->CreateBufferAndCopyData(m_vecVertices.size() * sizeof(App::VertexP),
                                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                     &(m_pMeshData->vertexBuffer.buffer),
                                     &(m_pMeshData->vertexBuffer.memory),
                                     m_vecVertices.data(),
                                     "BLAS_CUBE_MESH_VB");

    // Create IB
    pDevice->CreateBufferAndCopyData(m_vecIndices.size() * sizeof(uint32_t),
                                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    &(m_pMeshData->indexBuffer.buffer),
                                    &(m_pMeshData->indexBuffer.memory),
                                    m_vecIndices.data(),
                                    "BLAS_CUBE_MESH_IB");


    // 3. Get Device addresses of buffers just created
    VkDeviceOrHostAddressConstKHR vbAddress = {};
    VkDeviceOrHostAddressConstKHR ibAddress = {};
    VkDeviceOrHostAddressConstKHR trAddress = {};

    vbAddress.deviceAddress = Vulkan::GetBufferDeviceAddress(pDevice, m_pMeshData->vertexBuffer.buffer);
    ibAddress.deviceAddress = Vulkan::GetBufferDeviceAddress(pDevice, m_pMeshData->indexBuffer.buffer);

    m_pMeshInstanceData->verticesAddress = vbAddress.deviceAddress;
    m_pMeshInstanceData->indicesAddress = ibAddress.deviceAddress;

    // 4. Define AS Geometry by providing vb, ib & tb addresses 
    VkAccelerationStructureGeometryKHR accelStructureGeometry = {};
    accelStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelStructureGeometry.geometry.triangles.vertexData = vbAddress;
    accelStructureGeometry.geometry.triangles.maxVertex = static_cast<uint32_t>(m_vecVertices.size());
    accelStructureGeometry.geometry.triangles.vertexStride = sizeof(App::VertexP);
    accelStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    accelStructureGeometry.geometry.triangles.indexData = ibAddress;
    accelStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
    accelStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
    accelStructureGeometry.geometry.triangles.transformData = trAddress;

    // 5. Get AS Build size estimate
    VkAccelerationStructureBuildGeometryInfoKHR accelStructBuildGeomInfo = {};
    accelStructBuildGeomInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelStructBuildGeomInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelStructBuildGeomInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    accelStructBuildGeomInfo.geometryCount = 1;
    accelStructBuildGeomInfo.pGeometries = &accelStructureGeometry;

    const uint32_t nTriangles = static_cast<uint32_t>(m_vecIndices.size()) / 3;
    VkAccelerationStructureBuildSizesInfoKHR accelStructBuildSizesInfo = {};
    accelStructBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    vkGetAccelerationStructureBuildSizesKHR(pDevice->m_vkLogicalDevice,
                                            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &accelStructBuildGeomInfo,
                                            &nTriangles,
                                            &accelStructBuildSizesInfo);

    // 6. Create buffer for holding AS and Create AS handle!
    pDevice->CreateBuffer(accelStructBuildSizesInfo.accelerationStructureSize,
                          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                          VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
                          &m_BottomLevelAS.buffer,
                          &m_BottomLevelAS.memory,
                          "BLAS_MESH");

    VkAccelerationStructureCreateInfoKHR accelStructCreateInfo = {};
    accelStructCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelStructCreateInfo.buffer = m_BottomLevelAS.buffer;
    accelStructCreateInfo.size = accelStructBuildSizesInfo.accelerationStructureSize;
    accelStructCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    VKRESULT_CHECK_INFO(vkCreateAccelerationStructureKHR(pDevice->m_vkLogicalDevice,
                        &accelStructCreateInfo,
                        nullptr,
                        &m_BottomLevelAS.handle),
                        "Failed to create Mesh BLAS",
                        "Successfully created Mesh BLAS!");

    // 7. Create a small scratch buffer used during build of BLAS
    Vulkan::RTScratchBuffer scratchBuffer;

    pDevice->CreateBuffer(accelStructBuildSizesInfo.buildScratchSize,
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          &scratchBuffer.handle,
                          &scratchBuffer.memory,
                          "BLAS_SCRATCH_BUFFER");

    scratchBuffer.deviceAddress = Vulkan::GetBufferDeviceAddress(pDevice, scratchBuffer.handle);

    VkAccelerationStructureBuildGeometryInfoKHR accelStructBuildGeomInfo2 = {};
    accelStructBuildGeomInfo2.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelStructBuildGeomInfo2.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelStructBuildGeomInfo2.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    accelStructBuildGeomInfo2.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelStructBuildGeomInfo2.dstAccelerationStructure = m_BottomLevelAS.handle;
    accelStructBuildGeomInfo2.geometryCount = 1;
    accelStructBuildGeomInfo2.pGeometries = &accelStructureGeometry;
    accelStructBuildGeomInfo2.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR accelStructBuildRangeInfo = {};
    accelStructBuildRangeInfo.primitiveCount = nTriangles;
    accelStructBuildRangeInfo.primitiveOffset = 0;
    accelStructBuildRangeInfo.firstVertex = 0;
    accelStructBuildRangeInfo.transformOffset = 0;

    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> vecAccelStructRangeInfos;
    vecAccelStructRangeInfos.push_back(&accelStructBuildRangeInfo);

    VkCommandBuffer commandBuffer = pDevice->BeginCommandBuffer("BuildBLAS");

    // Accel Struct needs to be built on Device!
    vkCmdBuildAccelerationStructuresKHR(commandBuffer,
                                        static_cast<uint32_t>(vecAccelStructRangeInfos.size()),
                                        &accelStructBuildGeomInfo2,
                                        vecAccelStructRangeInfos.data());

    pDevice->EndAndSubmitCommandBuffer(commandBuffer);

    // 9. Finally, get hold of device address of BLAS!
    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = m_BottomLevelAS.handle;
    m_BottomLevelAS.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(pDevice->m_vkLogicalDevice, &accelerationDeviceAddressInfo);

    // 10. Scratch buffer no lonoger needed!
    scratchBuffer.Cleanup(pDevice);
}
