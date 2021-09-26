#pragma once

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "Engine/Helpers/Utility.h"

class TriangleMesh
{
public:
    TriangleMesh(const std::string filepath);
    ~TriangleMesh();

    void                                            Initialize(VulkanDevice* pDevice);
    void                                            Update(float dt);
    void                                            Render();
    void                                            Cleanup(VulkanDevice* pDevice);

private:

    PFN_vkCreateAccelerationStructureKHR            vkCreateAccelerationStructureKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR         vkCmdBuildAccelerationStructuresKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR  vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR     vkGetAccelerationStructureBuildSizesKHR;

    void                                            LoadModel(const std::string& path);
    void                                            ProcessNode(aiNode* node, const aiScene* scene);
    void                                            ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void                                            CreateBottomLevelAS(VulkanDevice* pDevice);

public:
    Vulkan::RTAccelerationStructure                 m_BottomLevelAS;
    Vulkan::MeshInstance*                           m_pMeshInstanceData;
    uint64_t                                        m_TriangleCount;

private:
    Vulkan::MeshData*                               m_pMeshData;

    std::vector<App::VertexP>                       m_vecVertices;
    std::vector<uint32_t>                           m_vecIndices;

    std::string                                     m_Filepath;
};

