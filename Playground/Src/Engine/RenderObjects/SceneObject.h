#pragma once

#include "Engine/Helpers/Utility.h"

class SceneObject
{
public:
    SceneObject();
    virtual ~SceneObject();

    virtual void                                    Initialize(VulkanDevice* pDevice);
    virtual void                                    Update(float dt);
    virtual void                                    Render();
    virtual void                                    Cleanup(VulkanDevice* pDevice);

protected:
    PFN_vkCreateAccelerationStructureKHR            vkCreateAccelerationStructureKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR         vkCmdBuildAccelerationStructuresKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR  vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR     vkGetAccelerationStructureBuildSizesKHR;

public:
    Vulkan::RTAccelerationStructure                 m_BottomLevelAS;
    Vulkan::MeshInstance*                           m_pMeshInstanceData;
};

