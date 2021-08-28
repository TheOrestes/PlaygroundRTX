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
// Holds data for a ray tracing scratch buffer used as temp storage!
struct RayTracingScratchBuffer
{
    uint64_t deviceAddress = 0;
    VkBuffer handle = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

//-----------------------------------------------------------------------------------------------------------------------
// Ray tracing Acceleration Structure!
struct AccelerationStructure
{
    VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
    uint64_t deviceAddress = 0;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

//-----------------------------------------------------------------------------------------------------------------------
struct StorageImage
{
    VkDeviceMemory  memory;
    VkImage         image;
    VkImageView     imageView;
    VkFormat        format;
};

//-----------------------------------------------------------------------------------------------------------------------
struct UniformData
{
    glm::mat4       view;
    glm::mat4       projection;
};

//---------------------------------------------------------------------------------------------------------------------
struct ShaderUniformsRT
{
    ShaderUniformsRT() {}

    void								CreateBuffer(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);

    void                                UpdateUniforms(VulkanDevice* pDevice);

    void								Cleanup(VulkanDevice* pDevice);
    void								CleanupOnWindowResize(VulkanDevice* pDevice);

    UniformData							shaderData;

    // Vulkan Specific
    VkBuffer				            vkBuffer;
    VkDeviceMemory          			vkDeviceMemory;
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
    PFN_vkDestroyAccelerationStructureKHR               vkDestroyAccelerationStructureKHR;
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

    AccelerationStructure                               m_vkBottomLevelAS;
    AccelerationStructure                               m_vkTopLevelAS;
    StorageImage                                        m_vkStorageImage;

    ShaderUniformsRT*                                   m_pShaderUniformsRT;


private:
    
    RayTracingScratchBuffer                             CreateScratchBuffer(VkDeviceSize size);
    void                                                CreateStorageImage();
    void                                                DeleteScratchBuffer(RayTracingScratchBuffer& buffer);

private:
    // Vertetx Buffer
    VkBuffer                                            m_vkVertexBuffer;
    VkDeviceMemory                                      m_vkVertexBufferDeviceMemory;

    // Index Buffer
    uint32_t                                            m_uiIndexCount;
    VkBuffer                                            m_vkIndexBuffer;
    VkDeviceMemory                                      m_vkIndexBufferDeviceMemory;

    // Transform Buffer
    VkBuffer                                            m_vkTransformBuffer;
    VkDeviceMemory                                      m_vkTransformBufferDeviceMemory;

    // RayGen shader binding table
    VkBuffer                                            m_vkRaygenShaderBindingTable;
    VkDeviceMemory                                      m_vkRaygenShaderBindingTableDeviceMemory;

    // Miss shader binding table
    VkBuffer                                            m_vkMissShaderBindingTable;
    VkDeviceMemory                                      m_vkMissShaderBindingTableDeviceMemory;

    // Hit shader binding table
    VkBuffer                                            m_vkHitShaderBindingTable;
    VkDeviceMemory                                      m_vkHitShaderBindingTableDeviceMemory;
    
    // Uniform data buffer
    VkBuffer                                            m_vkUniformBuffer;
    VkDeviceMemory                                      m_vkUniformBufferDeviceMemory;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR>   m_vecRayTracingShaderGroupsCreateInfos;
    std::vector<VkShaderModule>                         m_vecShaderModules;

    VkPipeline                                          m_vkPipelineRayTracing;
    VkPipelineLayout                                    m_vkPipelineLayoutRayTracing;
    VkDescriptorSet                                     m_vkDescriptorSetRayTracing;
    VkDescriptorSetLayout                               m_vkDescriptorSetLayoutRayTracing;
    VkDescriptorPool                                    m_vkDescriptorPoolRayTracing;
};

