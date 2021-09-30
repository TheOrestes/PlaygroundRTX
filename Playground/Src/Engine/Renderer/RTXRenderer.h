#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"

#include "VulkanRenderer.h"

class VulkanDevice;
class VulkanSwapChain;
class VulkanFrameBuffer;
class VulkanGraphicsPipeline;
class Scene;
class SceneObject;
class RTXCube;
class TriangleMesh;

//-----------------------------------------------------------------------------------------------------------------------
struct RTUniformData
{
    glm::mat4       view;
    glm::mat4       projection;
};

//---------------------------------------------------------------------------------------------------------------------
struct RTShaderUniforms
{
    RTShaderUniforms() {}

    void								CreateBuffer(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);

    void                                UpdateUniforms(VulkanDevice* pDevice);

    void								Cleanup(VulkanDevice* pDevice);
    void								CleanupOnWindowResize(VulkanDevice* pDevice);

    RTUniformData						uniformData;

    // Vulkan Specific
    Vulkan::Buffer                      uniformDataBuffer;
};

//-----------------------------------------------------------------------------------------------------------------------
class RTXRenderer : public VulkanRenderer
{
public:
    RTXRenderer();
    virtual ~RTXRenderer();

    virtual int						                    Initialize(GLFWwindow* pWindow) override;
    virtual void					                    Update(float dt) override;
    virtual void					                    Render() override;
    virtual void					                    Cleanup() override;
                                                        
    virtual void					                    RecordCommands(uint32_t currentImage) override;    
                                                        
    virtual void					                    HandleWindowResize() override;
    virtual void					                    CleanupOnWindowResize() override;
                                                        
    // RTX                                              
    void							                    InitRayTracing();
    void							                    CreateTopLevelAS(bool bUpdate);
    void							                    CreateRayTracingDescriptorSet();
    void							                    CreateRayTracingGraphicsPipeline();
    void							                    CreateRayTracingBindingTable();

private:
    PFN_vkCreateAccelerationStructureKHR                vkCreateAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR         vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR      vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR             vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR                vkBuildAccelerationStructuresKHR;
    PFN_vkCmdTraceRaysKHR                               vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR            vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR                  vkCreateRayTracingPipelinesKHR;
                                                        
private:                                                

    // RTX
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR     m_vkRayTracingPipelineProperties;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR    m_vkRayTracingAccelerationStructureFeatures;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR    m_vkRayTracingAccelerationStructureFeaturesEnabled;

    Vulkan::RTAccelerationStructure                     m_BottomLevelAS;
    Vulkan::RTAccelerationStructure                     m_TopLevelAS;
    Vulkan::RTStorageImage                              m_StorageImage;

    RTShaderUniforms*                                   m_pShaderUniformsRT;

private:
    
    Vulkan::RTScratchBuffer                             CreateScratchBuffer(VkDeviceSize size);
    void                                                CreateStorageImage();

private:
   
    //RTXCube*                                            m_pCube;
    //TriangleMesh*                                       m_pMesh;

    Scene*                                              m_pScene;

    // RayGen shader binding table
    Vulkan::Buffer                                      m_RaygenShaderBindingTable;

    // Miss shader binding table
    Vulkan::Buffer                                      m_MissShaderBindingTable;

    // Hit shader binding table
    Vulkan::Buffer                                      m_HitShaderBindingTable;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR>   m_vecRayTracingShaderGroupsCreateInfos;
    std::vector<VkShaderModule>                         m_vecShaderModules;

    VkPipeline                                          m_vkPipelineRayTracing;
    VkPipelineLayout                                    m_vkPipelineLayoutRayTracing;
    VkDescriptorSet                                     m_vkDescriptorSetRayTracing;
    VkDescriptorSetLayout                               m_vkDescriptorSetLayoutRayTracing;
    VkDescriptorPool                                    m_vkDescriptorPoolRayTracing;
};

