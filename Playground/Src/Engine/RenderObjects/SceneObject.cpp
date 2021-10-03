#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"
#include "SceneObject.h"

//---------------------------------------------------------------------------------------------------------------------
SceneObject::SceneObject()
{
    m_bUpdate = false;
}

//---------------------------------------------------------------------------------------------------------------------
SceneObject::~SceneObject()
{
}

//---------------------------------------------------------------------------------------------------------------------
void SceneObject::Initialize(VulkanDevice* pDevice)
{
    // Get the ray tracing & AS related function ptrs
    vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(pDevice->m_vkLogicalDevice, "vkCreateAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(pDevice->m_vkLogicalDevice, "vkGetAccelerationStructureBuildSizesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(pDevice->m_vkLogicalDevice, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(pDevice->m_vkLogicalDevice, "vkCmdBuildAccelerationStructuresKHR"));

    m_pMeshInstanceData = new Vulkan::MeshInstance();
}

//---------------------------------------------------------------------------------------------------------------------
void SceneObject::Update(float dt)
{
    if (m_bUpdate)
    {
        m_pMeshInstanceData->Update(dt);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void SceneObject::Render()
{
}

//---------------------------------------------------------------------------------------------------------------------
void SceneObject::Cleanup(VulkanDevice* pDevice)
{
}

//---------------------------------------------------------------------------------------------------------------------
void SceneObject::SetPosition(const glm::vec3& pos)
{
    m_pMeshInstanceData->position = pos;
    m_pMeshInstanceData->Update(0.0f);
}

//---------------------------------------------------------------------------------------------------------------------
void SceneObject::SetScale(const glm::vec3& sc)
{
    m_pMeshInstanceData->scale = sc;
    m_pMeshInstanceData->Update(0.0f);
}

//---------------------------------------------------------------------------------------------------------------------
void SceneObject::SetRotation(const glm::vec3& axis, float angle)
{
    m_pMeshInstanceData->rotationAxis = axis;
    m_pMeshInstanceData->angle = angle;
    m_pMeshInstanceData->Update(0.0f);
}

void SceneObject::SetUpdate(bool flag)
{
    m_bUpdate = flag;
}

