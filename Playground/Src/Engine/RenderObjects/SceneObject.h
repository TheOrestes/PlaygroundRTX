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

    virtual void                                    SetPosition(const glm::vec3& pos);
    virtual void                                    SetScale(const glm::vec3& sc);
    virtual void                                    SetRotation(const glm::vec3& axis, float angle);
    virtual void                                    SetUpdate(bool flag);

protected:
    PFN_vkCreateAccelerationStructureKHR            vkCreateAccelerationStructureKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR         vkCmdBuildAccelerationStructuresKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR  vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR     vkGetAccelerationStructureBuildSizesKHR;

    bool                                            m_bUpdate;

public:
    Vulkan::RTAccelerationStructure                 m_BottomLevelAS;
    Vulkan::MeshInstance*                           m_pMeshInstanceData;
    
};

