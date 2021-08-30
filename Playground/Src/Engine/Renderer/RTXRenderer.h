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

    //void                                                SetupDepthStencil();
    //void                                                SetupRenderPass();
    //void                                                SetupFramebuffer();
                                                        
    // RTX                                              
    void							                    InitRayTracing();
    void							                    CreateBottomLevelAS();
    void							                    CreateTopLevelAS();
    void							                    CreateRayTracingDescriptorSet();
    void							                    CreateRayTracingGraphicsPipeline();
    void							                    CreateRayTracingBindingTable();

public:
    PFN_vkGetBufferDeviceAddressKHR                     vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR                vkCreateAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR         vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR      vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR             vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR                vkBuildAccelerationStructuresKHR;
    PFN_vkCmdTraceRaysKHR                               vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR            vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR                  vkCreateRayTracingPipelinesKHR;
                                                        
private:                                                
                                                        
    VkDeviceSize                                        m_vkBufferSize;
    VkBuffer                                            m_vkBufferImage;
    VkDeviceMemory                                      m_vkDeviceMemoryImage;

    // RTX
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR     m_vkRayTracingPipelineProperties;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR    m_vkRayTracingAccelerationStructureFeatures;
    VkPhysicalDeviceBufferDeviceAddressFeatures         m_vkBufferDeviceAddressFeaturesEnabled;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR       m_vkRayTracingPipelineFeaturesEnabled;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR    m_vkRayTracingAccelerationStructureFeaturesEnabled;

    Vulkan::RTAccelerationStructure                     m_BottomLevelAS;
    Vulkan::RTAccelerationStructure                     m_TopLevelAS;
    Vulkan::RTStorageImage                              m_StorageImage;

    RTShaderUniforms*                                   m_pShaderUniformsRT;


private:
    
    Vulkan::RTScratchBuffer                             CreateScratchBuffer(VkDeviceSize size);
    void                                                CreateStorageImage();

private:
    // Vertetx Buffer
    Vulkan::Buffer                                      m_VertexBuffer;

    // Index Buffer
    Vulkan::Buffer                                      m_IndexBuffer;
    uint32_t                                            m_uiIndexCount;

    // Transform Buffer
    Vulkan::Buffer                                      m_TransformBuffer;

    // RayGen shader binding table
    Vulkan::Buffer                                      m_RaygenShaderBindingTable;

    // Miss shader binding table
    Vulkan::Buffer                                      m_MissShaderBindingTable;

    // Hit shader binding table
    Vulkan::Buffer                                      m_HitShaderBindingTable;
    
    // Uniform data buffer
    Vulkan::Buffer                                      m_UniformDataBuffer;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR>   m_vecRayTracingShaderGroupsCreateInfos;
    std::vector<VkShaderModule>                         m_vecShaderModules;

    VkPipeline                                          m_vkPipelineRayTracing;
    VkPipelineLayout                                    m_vkPipelineLayoutRayTracing;
    VkDescriptorSet                                     m_vkDescriptorSetRayTracing;
    VkDescriptorSetLayout                               m_vkDescriptorSetLayoutRayTracing;
    VkDescriptorPool                                    m_vkDescriptorPoolRayTracing;
};

