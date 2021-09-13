#pragma once
#include "Engine/Helpers/Utility.h"

class RTXCube
{
public:
    RTXCube();
    ~RTXCube();

public:
    void                                            Initialize(VulkanDevice* pDevice);
    void                                            Update(float dt);
    void                                            Render();
    void                                            Cleanup(VulkanDevice* pDevice);

private:

    PFN_vkCreateAccelerationStructureKHR            vkCreateAccelerationStructureKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR         vkCmdBuildAccelerationStructuresKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR  vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR     vkGetAccelerationStructureBuildSizesKHR;
    
    void                                            CreateBottomLevelAS(VulkanDevice* pDevice);

private:
    Vulkan::MeshData*                                m_pMeshData;

    // Actual Data
    std::vector<App::VertexP>                       m_vecVertices;
    std::vector<uint32_t>                           m_vecIndices;

public:
    Vulkan::RTAccelerationStructure                 m_BottomLevelAS;
    Vulkan::MeshInstance*                           m_pMeshInstanceData;
};
