#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"
#include "SceneObject.h"

//---------------------------------------------------------------------------------------------------------------------
SceneObject::SceneObject()
{
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
    m_pMeshInstanceData->Update(dt);
}

//---------------------------------------------------------------------------------------------------------------------
void SceneObject::Render()
{
}

//---------------------------------------------------------------------------------------------------------------------
void SceneObject::Cleanup(VulkanDevice* pDevice)
{
}

