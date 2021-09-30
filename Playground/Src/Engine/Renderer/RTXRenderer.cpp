#include "PlaygroundPCH.h"
#include "RTXRenderer.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanTexture2D.h"
#include "VulkanGraphicsPipeline.h"
#include "Engine/RenderObjects/HDRISkydome.h"
#include "Engine/Scene.h"      
#include "Engine/RenderObjects/RTXCube.h"
#include "Engine/Helpers/Camera.h"
#include "Engine/ImGui/UIManager.h"
#include "Engine/RenderObjects/TriangleMesh.h"
#include "Engine/RenderObjects/SceneObject.h"

//---------------------------------------------------------------------------------------------------------------------
RTXRenderer::RTXRenderer()
{
    m_pScene = nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
RTXRenderer::~RTXRenderer()
{
    SAFE_DELETE(m_pScene);
}

//---------------------------------------------------------------------------------------------------------------------
int RTXRenderer::Initialize(GLFWwindow* pWindow)
{
    VulkanRenderer::Initialize(pWindow);

    // Get the ray tracing & AS related function ptrs
    vkCreateAccelerationStructureKHR                = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_pDevice->m_vkLogicalDevice, "vkCreateAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizesKHR         = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(m_pDevice->m_vkLogicalDevice, "vkGetAccelerationStructureBuildSizesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR      = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(m_pDevice->m_vkLogicalDevice, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCmdBuildAccelerationStructuresKHR             = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_pDevice->m_vkLogicalDevice, "vkCmdBuildAccelerationStructuresKHR"));
    vkBuildAccelerationStructuresKHR                = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_pDevice->m_vkLogicalDevice, "vkBuildAccelerationStructuresKHR"));
    vkCmdTraceRaysKHR                               = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_pDevice->m_vkLogicalDevice, "vkCmdTraceRaysKHR"));
    vkGetRayTracingShaderGroupHandlesKHR            = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(m_pDevice->m_vkLogicalDevice, "vkGetRayTracingShaderGroupHandlesKHR"));
    vkCreateRayTracingPipelinesKHR                  = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(m_pDevice->m_vkLogicalDevice, "vkCreateRayTracingPipelinesKHR"));

    try
    {
        //SetupDepthStencil();
        //SetupRenderPass();
        InitRayTracing();

        m_pScene = new Scene();
        m_pScene->LoadScene(m_pDevice, m_pSwapChain);

        //m_pCube = new RTXCube();
        //m_pCube->Initialize(m_pDevice);

        //m_pMesh = new TriangleMesh("Assets/Models/SteamPunk.fbx");
        //m_pMesh->Initialize(m_pDevice);

        m_pShaderUniformsRT = new RTShaderUniforms();
        m_pShaderUniformsRT->CreateBuffer(m_pDevice, m_pSwapChain);

        m_vecShaderModules.clear();
        
        CreateTopLevelAS(false);
        CreateStorageImage();
        CreateRayTracingDescriptorSet();
        CreateRayTracingGraphicsPipeline();
        CreateRayTracingBindingTable();

        // Initialize UI Manager!
        UIManager::getInstance().Initialize(m_pWindow, m_vkInstance, m_pDevice, m_pSwapChain);
    }
    catch (const std::runtime_error& e)
    {
        std::cout << "\nERROR: " << e.what();
        return EXIT_FAILURE;
    }

    return 0;
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::Update(float dt)
{
    VulkanRenderer::Update(dt);

    // Update View & Projection matrix data!
    m_pShaderUniformsRT->uniformData.view        = glm::inverse(Camera::getInstance().m_matView);
    m_pShaderUniformsRT->uniformData.projection  = glm::inverse(Camera::getInstance().m_matProjection);

    //m_pCube->Update(dt);
    //m_pMesh->Update(dt);

    m_pScene->Update(m_pDevice, m_pSwapChain, dt);
    m_pShaderUniformsRT->UpdateUniforms(m_pDevice);

    CreateTopLevelAS(true);

    //VkAccelerationStructureInstanceKHR& tInst = m_TopLevelAS.handle;
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::Render()
{
    VulkanRenderer::BeginFrame();
    RecordCommands(m_uiSwapchainImageIndex);

    UIManager::getInstance().BeginRender();
    //UIManager::getInstance().RenderSceneUI(m_pScene);
    UIManager::getInstance().RenderDebugStats();
    UIManager::getInstance().EndRender(m_pSwapChain, m_uiSwapchainImageIndex);

    VulkanRenderer::SubmitAndPresentFrame();   
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::Cleanup()
{
    // Wait until no action being run on device before destroying! 
    vkDeviceWaitIdle(m_pDevice->m_vkLogicalDevice);

    UIManager::getInstance().Cleanup(m_pDevice);

    vkDestroyDescriptorPool(m_pDevice->m_vkLogicalDevice, m_vkDescriptorPoolRayTracing, nullptr);
    vkDestroyDescriptorSetLayout(m_pDevice->m_vkLogicalDevice, m_vkDescriptorSetLayoutRayTracing, nullptr);

    vkDestroyPipeline(m_pDevice->m_vkLogicalDevice, m_vkPipelineRayTracing, nullptr);
    vkDestroyPipelineLayout(m_pDevice->m_vkLogicalDevice, m_vkPipelineLayoutRayTracing, nullptr);
    
    for (uint16_t i = 0; i < m_vecShaderModules.size(); ++i)
    {
        vkDestroyShaderModule(m_pDevice->m_vkLogicalDevice, m_vecShaderModules[i], nullptr);
    }

    m_StorageImage.Cleanup(m_pDevice);

    //m_pCube->Cleanup(m_pDevice);
    //m_pMesh->Cleanup(m_pDevice);
    m_pScene->Cleanup(m_pDevice);
    m_TopLevelAS.Cleanup(m_pDevice);

    m_RaygenShaderBindingTable.Cleanup(m_pDevice);
    m_MissShaderBindingTable.Cleanup(m_pDevice);
    m_HitShaderBindingTable.Cleanup(m_pDevice);

    m_pShaderUniformsRT->Cleanup(m_pDevice);
    SAFE_DELETE(m_pShaderUniformsRT);

    VulkanRenderer::Cleanup();
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::RecordCommands(uint32_t currentImage)
{
    //VulkanRenderer::RecordCommands(currentImage);

    // Information about how to begin each command buffer
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferBeginInfo.flags = 0;

    VKRESULT_CHECK(vkBeginCommandBuffer(m_pDevice->m_vecCommandBufferGraphics[currentImage], &bufferBeginInfo));

    VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    const uint32_t handleSizeAligned = Vulkan::alignedSize(m_vkRayTracingPipelineProperties.shaderGroupHandleSize, m_vkRayTracingPipelineProperties.shaderGroupHandleAlignment);

    VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    raygenShaderSbtEntry.deviceAddress = Vulkan::GetBufferDeviceAddress(m_pDevice, m_RaygenShaderBindingTable.buffer);
    raygenShaderSbtEntry.stride = handleSizeAligned;
    raygenShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress = Vulkan::GetBufferDeviceAddress(m_pDevice, m_MissShaderBindingTable.buffer);
    missShaderSbtEntry.stride = handleSizeAligned;
    missShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    hitShaderSbtEntry.deviceAddress = Vulkan::GetBufferDeviceAddress(m_pDevice, m_HitShaderBindingTable.buffer);
    hitShaderSbtEntry.stride = handleSizeAligned;
    hitShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

    /*
        Dispatch the ray tracing commands
    */
    vkCmdBindPipeline(m_pDevice->m_vecCommandBufferGraphics[currentImage], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_vkPipelineRayTracing);
    vkCmdBindDescriptorSets(m_pDevice->m_vecCommandBufferGraphics[currentImage], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_vkPipelineLayoutRayTracing, 0, 1, &m_vkDescriptorSetRayTracing, 0, 0);

    vkCmdTraceRaysKHR(m_pDevice->m_vecCommandBufferGraphics[currentImage],
                      &raygenShaderSbtEntry,
                      &missShaderSbtEntry,
                      &hitShaderSbtEntry,
                      &callableShaderSbtEntry,
                      m_pSwapChain->m_vkSwapchainExtent.width, m_pSwapChain->m_vkSwapchainExtent.height, 1);

    /*
        Copy ray tracing output to swap chain image
    */

    // Prepare current swap chain image as transfer destination

    Vulkan::TransitionImageLayout(m_pDevice,
                                          m_pDevice->m_vecCommandBufferGraphics[currentImage],
                                          m_pSwapChain->m_vecSwapchainImages[currentImage],
                                          VK_IMAGE_LAYOUT_UNDEFINED,
                                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                          subresourceRange);
   
   Vulkan::TransitionImageLayout(m_pDevice,
                                         m_pDevice->m_vecCommandBufferGraphics[currentImage],
                                         m_StorageImage.image,
                                         VK_IMAGE_LAYOUT_GENERAL,
                                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                         subresourceRange);

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copyRegion.srcOffset = { 0, 0, 0 };
    copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copyRegion.dstOffset = { 0, 0, 0 };
    copyRegion.extent = { m_pSwapChain->m_vkSwapchainExtent.width, m_pSwapChain->m_vkSwapchainExtent.height, 1 };

    vkCmdCopyImage(m_pDevice->m_vecCommandBufferGraphics[currentImage], 
                   m_StorageImage.image, 
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_pSwapChain->m_vecSwapchainImages[currentImage], 
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                   1, 
                   &copyRegion);

    // Transition swap chain image back for presentation
    Vulkan::TransitionImageLayout(m_pDevice,
                                          m_pDevice->m_vecCommandBufferGraphics[currentImage],
                                          m_pSwapChain->m_vecSwapchainImages[currentImage],
                                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                          subresourceRange);

    Vulkan::TransitionImageLayout(m_pDevice,
                                          m_pDevice->m_vecCommandBufferGraphics[currentImage],
                                          m_StorageImage.image,
                                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                          VK_IMAGE_LAYOUT_GENERAL,
                                          subresourceRange);

    VKRESULT_CHECK(vkEndCommandBuffer(m_pDevice->m_vecCommandBufferGraphics[currentImage]));
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::HandleWindowResize()
{
    VulkanRenderer::HandleWindowResize();

    // Recreate Storage image!
    CreateStorageImage();

    // Update Descriptor!
    VkDescriptorImageInfo storageImageDescriptor = {};
    storageImageDescriptor.imageView = m_StorageImage.imageView;
    storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet resultImageWrite = {};
    resultImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    resultImageWrite.dstSet = m_vkDescriptorSetRayTracing;
    resultImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    resultImageWrite.dstBinding = 1;
    resultImageWrite.descriptorCount = 1;
    resultImageWrite.pImageInfo = &storageImageDescriptor;

    vkUpdateDescriptorSets(m_pDevice->m_vkLogicalDevice, 1, &resultImageWrite, 0, VK_NULL_HANDLE);

    UIManager::getInstance().HandleWindowResize(m_pWindow, m_vkInstance, m_pDevice, m_pSwapChain);

    LOG_DEBUG("Recreating SwapChain End");
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::CleanupOnWindowResize()
{
    VulkanRenderer::CleanupOnWindowResize();

    UIManager::getInstance().CleanupOnWindowResize(m_pDevice);
    
    m_StorageImage.CleanupOnWindowResize(m_pDevice);
}
//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::InitRayTracing()
{
    // Get Physical Device Ray Tracing properties
    m_vkRayTracingPipelineProperties = {};
    m_vkRayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    
    // Requesting ray tracing properties
    VkPhysicalDeviceProperties2 prop2 = {};
    prop2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    prop2.pNext = &m_vkRayTracingPipelineProperties;
    
    vkGetPhysicalDeviceProperties2(m_pDevice->m_vkPhysicalDevice, &prop2);
    
    // Get Acceleration Structure properties
    m_vkRayTracingAccelerationStructureFeatures = {};
    m_vkRayTracingAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    
    VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &m_vkRayTracingAccelerationStructureFeatures;
    
    vkGetPhysicalDeviceFeatures2(m_pDevice->m_vkPhysicalDevice, &deviceFeatures2);
}

//---------------------------------------------------------------------------------------------------------------------
// TLAS holds scene's object instances
//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::CreateTopLevelAS(bool bUpdate)
{ 
    // 1. Fetch transform matrix data
    // glm::mat4 matrix1 = glm::transpose(m_pScene->m_vecSceneObjects[0]->m_pMeshInstanceData->transformMatrix);
    // VkTransformMatrixKHR out_matrix1;
    // memcpy(&out_matrix1, &matrix1, sizeof(VkTransformMatrixKHR));
    // 
    // glm::mat4 matrix2 = glm::transpose(m_pScene->m_vecSceneObjects[1]->m_pMeshInstanceData->transformMatrix);
    // VkTransformMatrixKHR out_matrix2;
    // memcpy(&out_matrix2, &matrix2, sizeof(VkTransformMatrixKHR));

    std::vector<VkTransformMatrixKHR>               instMatrices(m_pScene->m_vecSceneObjects.size());
    std::vector<VkAccelerationStructureInstanceKHR> instAccelStruct(m_pScene->m_vecSceneObjects.size());

    for (uint32_t i = 0; i < m_pScene->m_vecSceneObjects.size(); i++)
    {
        // 1. Fetch transform matrix data
        glm::mat4 matrix = glm::transpose(m_pScene->m_vecSceneObjects[i]->m_pMeshInstanceData->transformMatrix);
        memcpy(&instMatrices[i], &matrix, sizeof(VkTransformMatrixKHR));

        VkAccelerationStructureInstanceKHR accelStructInstance = {};
        accelStructInstance.transform = instMatrices[i];
        accelStructInstance.instanceCustomIndex = i;
        accelStructInstance.mask = 0xFF;
        accelStructInstance.instanceShaderBindingTableRecordOffset = 0;
        accelStructInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        accelStructInstance.accelerationStructureReference = m_pScene->m_vecSceneObjects[i]->m_BottomLevelAS.deviceAddress;

        instAccelStruct[i] = accelStructInstance;
    }

    // 2. Create buffer for Instance data!
    
    // Create a buffer holding the actual instance data for use by the AS builder!
    VkDeviceSize instanceDescSizeInBytes = m_pScene->m_vecSceneObjects.size() * sizeof(VkAccelerationStructureInstanceKHR);

    Vulkan::Buffer instanceBuffer;
    m_pDevice->CreateBufferAndCopyData(instanceDescSizeInBytes,
                                       VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &instanceBuffer.buffer, &instanceBuffer.memory, instAccelStruct.data(), "TLAS_Instances");

    // 3. Get Device address of Buffer just created!
    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress = {};
    instanceDataDeviceAddress.deviceAddress = Vulkan::GetBufferDeviceAddress(m_pDevice, instanceBuffer.buffer);
    

    // We have just one Cube for now!
    //VkAccelerationStructureInstanceKHR accelStructInstance1 = {};
    //accelStructInstance1.transform = out_matrix1;
    //accelStructInstance1.instanceCustomIndex = 0;
    //accelStructInstance1.mask = 0xFF;
    //accelStructInstance1.instanceShaderBindingTableRecordOffset = 0;
    //accelStructInstance1.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    //accelStructInstance1.accelerationStructureReference = m_pScene->m_vecSceneObjects[0]->m_BottomLevelAS.deviceAddress; 
    //
    //VkAccelerationStructureInstanceKHR accelStructInstance2 = {};
    //accelStructInstance2.transform = out_matrix2;
    //accelStructInstance2.instanceCustomIndex = 0;
    //accelStructInstance2.mask = 0xFF;
    //accelStructInstance2.instanceShaderBindingTableRecordOffset = 0;
    //accelStructInstance2.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    //accelStructInstance2.accelerationStructureReference = m_pScene->m_vecSceneObjects[1]->m_BottomLevelAS.deviceAddress;

    // Buffer for instance data
    // Vulkan::Buffer instanceBuffer1;
    // Vulkan::Buffer instanceBuffer2;
    // 
    // // 2. Create buffer for Instance data!
    // m_pDevice->CreateBufferAndCopyData(sizeof(VkAccelerationStructureInstanceKHR),
    //                                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
    //                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //                                    &instanceBuffer1.buffer, &instanceBuffer1.memory, &accelStructInstance1, "TLAS_Instance1");
    // 
    // m_pDevice->CreateBufferAndCopyData(sizeof(VkAccelerationStructureInstanceKHR),
    //                                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
    //                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //                                    &instanceBuffer2.buffer, &instanceBuffer2.memory, &accelStructInstance2, "TLAS_Instance2");

    // 3. Get Device address of Buffer just created!
    // VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress1 = {};
    // instanceDataDeviceAddress1.deviceAddress = Vulkan::GetBufferDeviceAddress(m_pDevice, instanceBuffer1.buffer);
    // 
    // VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress2 = {};
    // instanceDataDeviceAddress2.deviceAddress = Vulkan::GetBufferDeviceAddress(m_pDevice, instanceBuffer2.buffer);

    // 4. Define AS data by providing buffer's device address
    VkAccelerationStructureGeometryKHR topASGeometry = {};
    topASGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    topASGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    topASGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    topASGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    topASGeometry.geometry.instances.data = instanceDataDeviceAddress;

    // VkAccelerationStructureGeometryKHR accelStructureGeom2 = {};
    // accelStructureGeom2.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    // accelStructureGeom2.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    // accelStructureGeom2.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    // accelStructureGeom2.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    // accelStructureGeom2.geometry.instances.arrayOfPointers = VK_FALSE;
    // accelStructureGeom2.geometry.instances.data = instanceDataDeviceAddress2;

    // std::array<VkAccelerationStructureGeometryKHR, 2> arrAccelStructGeoms = 
    // {
    //     accelStructureGeom1,
    //     accelStructureGeom2
    // };

    // 5. Get AS build size estimate
    VkAccelerationStructureBuildGeometryInfoKHR accelStructBuildGeomInfo = {};
    accelStructBuildGeomInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelStructBuildGeomInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelStructBuildGeomInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    accelStructBuildGeomInfo.geometryCount = 1;
    accelStructBuildGeomInfo.pGeometries = &topASGeometry;

    uint32_t primitiveCount = m_pScene->m_vecSceneObjects.size();

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(m_pDevice->m_vkLogicalDevice,
                                            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &accelStructBuildGeomInfo,
                                            &primitiveCount,
                                            &accelerationStructureBuildSizesInfo);

    // 6. Create buffer for holding AS and Create AS handle!
    if (!bUpdate)
    {
        m_pDevice->CreateBuffer(accelerationStructureBuildSizesInfo.accelerationStructureSize,
                                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
                                &m_TopLevelAS.buffer,
                                &m_TopLevelAS.memory,
                                "TLAS_AS");

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = m_TopLevelAS.buffer;
        accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        vkCreateAccelerationStructureKHR(m_pDevice->m_vkLogicalDevice, &accelerationStructureCreateInfo, nullptr, &m_TopLevelAS.handle);
    }
    

    // 7. Create a small scratch buffer used during building of the TLAS
    Vulkan::RTScratchBuffer scratchBuffer = CreateScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

    VkAccelerationStructureBuildGeometryInfoKHR accelBuildGeometryInfo2 = {};
    accelBuildGeometryInfo2.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelBuildGeometryInfo2.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelBuildGeometryInfo2.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    accelBuildGeometryInfo2.mode = bUpdate ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelBuildGeometryInfo2.dstAccelerationStructure = m_TopLevelAS.handle;
    accelBuildGeometryInfo2.geometryCount = 1;
    accelBuildGeometryInfo2.pGeometries = &topASGeometry;
    accelBuildGeometryInfo2.scratchData.deviceAddress = scratchBuffer.deviceAddress;
    accelBuildGeometryInfo2.srcAccelerationStructure = bUpdate ? m_TopLevelAS.handle : VK_NULL_HANDLE;
    accelBuildGeometryInfo2.dstAccelerationStructure = m_TopLevelAS.handle;

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo = {};
    accelerationStructureBuildRangeInfo.primitiveCount = static_cast<uint32_t>(m_pScene->m_vecSceneObjects.size());
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> vecAccelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

    // 8. Create AS either on CPU or GPU depending on the feature availability!
    if (m_vkRayTracingAccelerationStructureFeaturesEnabled.accelerationStructureHostCommands)
    {
        // Implementation supports building Accel Struct on Host!
        VKRESULT_CHECK_INFO(vkBuildAccelerationStructuresKHR(m_pDevice->m_vkLogicalDevice,
                                                             VK_NULL_HANDLE,
                                                             static_cast<uint32_t>(vecAccelerationBuildStructureRangeInfos.size()),
                                                             &accelBuildGeometryInfo2,
                                                             vecAccelerationBuildStructureRangeInfos.data()),
                           "On-Host BLAS failed to build",
                           "On-Host BLAS built successfully!");
    }
    else
    {
        VkCommandBuffer commandBuffer = m_pDevice->BeginCommandBuffer();

        // Accel Struct needs to be built on Device!
        vkCmdBuildAccelerationStructuresKHR(commandBuffer,
                                            static_cast<uint32_t>(vecAccelerationBuildStructureRangeInfos.size()),
                                            &accelBuildGeometryInfo2,
                                            vecAccelerationBuildStructureRangeInfos.data());

        m_pDevice->EndAndSubmitCommandBuffer(commandBuffer);
    }

    // 9. Finally, get hold of device address of TLAS!
    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = m_TopLevelAS.handle;
    m_TopLevelAS.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(m_pDevice->m_vkLogicalDevice, &accelerationDeviceAddressInfo);

    // 10. Scratch buffer & Instance buffer no longer needed!
    scratchBuffer.Cleanup(m_pDevice);
    instanceBuffer.Cleanup(m_pDevice);
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::CreateRayTracingDescriptorSet()
{
    //--- Create Descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = 
    {
        {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
    };

    VkDescriptorPoolCreateInfo descPoolCreateInfo = {};
    descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descPoolCreateInfo.maxSets = 1;
    descPoolCreateInfo.pPoolSizes = poolSizes.data();

    VKRESULT_CHECK_INFO(vkCreateDescriptorPool(m_pDevice->m_vkLogicalDevice,
                                               &descPoolCreateInfo,
                                               nullptr,
                                               &m_vkDescriptorPoolRayTracing),
                        "Failed to create Ray Tracing Descriptor Pool",
                        "Succesfully created Ray Tracing Descriptor Pool");

    //--- Create Descriptor Set Layout 
    VkDescriptorSetLayoutBinding accelStructLayoutBinding = {};
    accelStructLayoutBinding.binding = 0;
    accelStructLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    accelStructLayoutBinding.descriptorCount = 1;
    accelStructLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding resultImageLayoutBinding = {};
    resultImageLayoutBinding.binding = 1;
    resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    resultImageLayoutBinding.descriptorCount = 1;
    resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding ubLayoutBinding = {};
    ubLayoutBinding.binding = 2;
    ubLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubLayoutBinding.descriptorCount = 1;
    ubLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    std::vector<VkDescriptorSetLayoutBinding> bindings = 
    {
        accelStructLayoutBinding,
        resultImageLayoutBinding,
        ubLayoutBinding
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    descriptorSetLayoutCreateInfo.pBindings = bindings.data();
    
    VKRESULT_CHECK(vkCreateDescriptorSetLayout(m_pDevice->m_vkLogicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &m_vkDescriptorSetLayoutRayTracing));

    //--- Create Descriptor Sets!
    VkDescriptorSetAllocateInfo descSetAllocInfo = {};
    descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descSetAllocInfo.descriptorPool = m_vkDescriptorPoolRayTracing;
    descSetAllocInfo.descriptorSetCount =  1;
    descSetAllocInfo.pSetLayouts = &m_vkDescriptorSetLayoutRayTracing;

    VKRESULT_CHECK(vkAllocateDescriptorSets(m_pDevice->m_vkLogicalDevice, &descSetAllocInfo, &m_vkDescriptorSetRayTracing));

    // Descriptor for AS
    VkWriteDescriptorSetAccelerationStructureKHR descASInfo = {};
    descASInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    descASInfo.accelerationStructureCount = 1;
    descASInfo.pAccelerationStructures = &m_TopLevelAS.handle;

    VkWriteDescriptorSet accelStructWrite = {};
    accelStructWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    accelStructWrite.pNext = &descASInfo;
    accelStructWrite.dstSet = m_vkDescriptorSetRayTracing;
    accelStructWrite.dstBinding = 0;
    accelStructWrite.descriptorCount = 1;
    accelStructWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    // Descriptor for Storage/Result Image
    VkDescriptorImageInfo storageImageDescriptor = {};
    storageImageDescriptor.imageView = m_StorageImage.imageView;
    storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet resultImageWrite = {};
    resultImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    resultImageWrite.dstSet = m_vkDescriptorSetRayTracing;
    resultImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    resultImageWrite.dstBinding = 1;
    resultImageWrite.descriptorCount = 1;            
    resultImageWrite.pImageInfo = &storageImageDescriptor;

    // Descriptor for Uniform buffer
    VkDescriptorBufferInfo ubBufferInfo = {};
    ubBufferInfo.buffer = m_pShaderUniformsRT->uniformDataBuffer.buffer;    // buffer to get data from
    ubBufferInfo.offset = 0;								                // position of start of data
    ubBufferInfo.range = sizeof(RTShaderUniforms);			                // size of data

    VkWriteDescriptorSet uboWrite = {};
    uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uboWrite.dstSet = m_vkDescriptorSetRayTracing;
    uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboWrite.dstBinding = 2;
    uboWrite.descriptorCount = 1;
    uboWrite.pBufferInfo = &ubBufferInfo;

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = 
    {
        accelStructWrite,
        resultImageWrite,
        uboWrite
    };

    vkUpdateDescriptorSets(m_pDevice->m_vkLogicalDevice,
                           static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(),
                           0,
                           VK_NULL_HANDLE);
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::CreateRayTracingGraphicsPipeline()
{
    //--- Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_vkDescriptorSetLayoutRayTracing;

    VKRESULT_CHECK(vkCreatePipelineLayout(m_pDevice->m_vkLogicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_vkPipelineLayoutRayTracing));

    //--- Ray Tracing Shader Groups!
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    // Raygen
    VkShaderModule raygenShaderModule = Vulkan::CreateShaderModule(m_pDevice, "Assets/Shaders/raygenBasic.rgen.spv");
    m_vecShaderModules.push_back(raygenShaderModule);

    VkPipelineShaderStageCreateInfo raygenShaderStageCreateInfo = {};
    raygenShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    raygenShaderStageCreateInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    raygenShaderStageCreateInfo.module = raygenShaderModule;
    raygenShaderStageCreateInfo.pName = "main";

    shaderStages.push_back(raygenShaderStageCreateInfo);

    VkRayTracingShaderGroupCreateInfoKHR rayGenShaderGroupCreateInfo = {};
    rayGenShaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    rayGenShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    rayGenShaderGroupCreateInfo.generalShader = static_cast<uint32_t>(shaderStages.size()-1);
    rayGenShaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    rayGenShaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    rayGenShaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

    m_vecRayTracingShaderGroupsCreateInfos.push_back(rayGenShaderGroupCreateInfo);

    // Miss
    VkShaderModule missShaderModule = Vulkan::CreateShaderModule(m_pDevice, "Assets/Shaders/missBasic.rmiss.spv");
    m_vecShaderModules.push_back(missShaderModule);

    VkPipelineShaderStageCreateInfo missShaderStageCreateInfo = {};
    missShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    missShaderStageCreateInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    missShaderStageCreateInfo.module = missShaderModule;
    missShaderStageCreateInfo.pName = "main";

    shaderStages.push_back(missShaderStageCreateInfo);

    VkRayTracingShaderGroupCreateInfoKHR missShaderGroupCreateInfo = {};
    missShaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    missShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    missShaderGroupCreateInfo.generalShader = static_cast<uint32_t>(shaderStages.size() - 1);
    missShaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    missShaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    missShaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

    m_vecRayTracingShaderGroupsCreateInfos.push_back(missShaderGroupCreateInfo);

    // Closest Hit
    VkShaderModule closestHitShaderModule = Vulkan::CreateShaderModule(m_pDevice, "Assets/Shaders/closestHitBasic.rchit.spv");
    m_vecShaderModules.push_back(closestHitShaderModule);

    VkPipelineShaderStageCreateInfo closestHitShaderStageCreateInfo = {};
    closestHitShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    closestHitShaderStageCreateInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    closestHitShaderStageCreateInfo.module = closestHitShaderModule;
    closestHitShaderStageCreateInfo.pName = "main";

    shaderStages.push_back(closestHitShaderStageCreateInfo);

    VkRayTracingShaderGroupCreateInfoKHR closestHitShaderGroupCreateInfo = {};
    closestHitShaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    closestHitShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    closestHitShaderGroupCreateInfo.generalShader = VK_SHADER_UNUSED_KHR;
    closestHitShaderGroupCreateInfo.closestHitShader = static_cast<uint32_t>(shaderStages.size() - 1);
    closestHitShaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    closestHitShaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

    m_vecRayTracingShaderGroupsCreateInfos.push_back(closestHitShaderGroupCreateInfo);
    
    // Finally, Create RT Pipeline!
    VkRayTracingPipelineCreateInfoKHR rayTracingPipelineInfo = {};
    rayTracingPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    rayTracingPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    rayTracingPipelineInfo.pStages = shaderStages.data();
    rayTracingPipelineInfo.groupCount = static_cast<uint32_t>(m_vecRayTracingShaderGroupsCreateInfos.size());
    rayTracingPipelineInfo.pGroups = m_vecRayTracingShaderGroupsCreateInfos.data();
    rayTracingPipelineInfo.maxPipelineRayRecursionDepth = 1;
    rayTracingPipelineInfo.layout = m_vkPipelineLayoutRayTracing;

    VKRESULT_CHECK_INFO(vkCreateRayTracingPipelinesKHR(m_pDevice->m_vkLogicalDevice,
                                                       VK_NULL_HANDLE,
                                                       VK_NULL_HANDLE,
                                                       1,
                                                       &rayTracingPipelineInfo,
                                                       nullptr,
                                                       &m_vkPipelineRayTracing),
                        "Failed to create Ray Tracing Pipeline",
                        "Successfully created Ray Tracing Pipeline!");
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::CreateRayTracingBindingTable()
{
    const uint32_t handleSize = m_vkRayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAligned = Vulkan::alignedSize(m_vkRayTracingPipelineProperties.shaderGroupHandleSize,
                                                                   m_vkRayTracingPipelineProperties.shaderGroupHandleAlignment);

    const uint32_t groupCount = static_cast<uint32_t>(m_vecRayTracingShaderGroupsCreateInfos.size());
    const uint32_t sbtSize = groupCount * handleSizeAligned;

    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    
    VKRESULT_CHECK(vkGetRayTracingShaderGroupHandlesKHR(m_pDevice->m_vkLogicalDevice, m_vkPipelineRayTracing, 0, 
                                                        groupCount, sbtSize, shaderHandleStorage.data()));

    const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    const VkMemoryPropertyFlags memoryUsageFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    m_pDevice->CreateBuffer(handleSize, bufferUsageFlags, memoryUsageFlags, &m_RaygenShaderBindingTable.buffer, &m_RaygenShaderBindingTable.memory);
    m_pDevice->CreateBuffer(handleSize, bufferUsageFlags, memoryUsageFlags, &m_MissShaderBindingTable.buffer, &m_MissShaderBindingTable.memory);
    m_pDevice->CreateBuffer(handleSize, bufferUsageFlags, memoryUsageFlags, &m_HitShaderBindingTable.buffer, &m_HitShaderBindingTable.memory);

    void* data;
    VKRESULT_CHECK(vkMapMemory(m_pDevice->m_vkLogicalDevice, m_RaygenShaderBindingTable.memory, 0, handleSize, 0, &data));
    memcpy(data, shaderHandleStorage.data(), handleSize);
    
    VKRESULT_CHECK(vkMapMemory(m_pDevice->m_vkLogicalDevice, m_MissShaderBindingTable.memory, 0, handleSize, 0, &data));
    memcpy(data, shaderHandleStorage.data() + handleSizeAligned, handleSize);

    VKRESULT_CHECK(vkMapMemory(m_pDevice->m_vkLogicalDevice, m_HitShaderBindingTable.memory, 0, handleSize, 0, &data));
    memcpy(data, shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);
}

//---------------------------------------------------------------------------------------------------------------------
Vulkan::RTScratchBuffer RTXRenderer::CreateScratchBuffer(VkDeviceSize size)
{
    Vulkan::RTScratchBuffer scratchBuffer = {};

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    VKRESULT_CHECK(vkCreateBuffer(m_pDevice->m_vkLogicalDevice, &bufferCreateInfo, nullptr, &scratchBuffer.handle));

    VkMemoryRequirements memReq = {};
    vkGetBufferMemoryRequirements(m_pDevice->m_vkLogicalDevice, scratchBuffer.handle, &memReq);

    VkMemoryAllocateFlagsInfo memAllocFlagsInfo = {};
    memAllocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    memAllocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.pNext = &memAllocFlagsInfo;
    memAllocInfo.allocationSize = memReq.size;
    memAllocInfo.memoryTypeIndex = m_pDevice->FindMemoryTypeIndex(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VKRESULT_CHECK(vkAllocateMemory(m_pDevice->m_vkLogicalDevice, &memAllocInfo, nullptr, &scratchBuffer.memory));
    VKRESULT_CHECK(vkBindBufferMemory(m_pDevice->m_vkLogicalDevice, scratchBuffer.handle, scratchBuffer.memory, 0));
    
    scratchBuffer.deviceAddress = Vulkan::GetBufferDeviceAddress(m_pDevice, scratchBuffer.handle);

    return scratchBuffer;
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::CreateStorageImage()
{
    // Create storage image
    m_StorageImage.image = Vulkan::CreateImage(m_pDevice,
                                               m_pSwapChain->m_vkSwapchainExtent.width,
                                               m_pSwapChain->m_vkSwapchainExtent.height,
                                               m_pSwapChain->m_vkSwapchainImageFormat,
                                               VK_IMAGE_TILING_OPTIMAL,
                                               VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                               &m_StorageImage.memory);

    // Create storage image view
    m_StorageImage.imageView = Vulkan::CreateImageView(m_pDevice, 
                                                       m_StorageImage.image,
                                                       m_pSwapChain->m_vkSwapchainImageFormat,
                                                       VK_IMAGE_ASPECT_COLOR_BIT);

    VkImageSubresourceRange subrResourceRange = {};
    subrResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subrResourceRange.baseArrayLayer = 0;
    subrResourceRange.baseMipLevel = 0;
    subrResourceRange.layerCount = 1;
    subrResourceRange.levelCount = 1;

    Vulkan::TransitionImageLayout(m_pDevice, 
                                  m_StorageImage.image, 
                                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                  subrResourceRange);
    
}

//---------------------------------------------------------------------------------------------------------------------
void RTShaderUniforms::CreateBuffer(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
    pDevice->CreateBuffer(sizeof(RTShaderUniforms),
                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          &uniformDataBuffer.buffer, &uniformDataBuffer.memory, "RTShaderUniforms");
}

//---------------------------------------------------------------------------------------------------------------------
void RTShaderUniforms::UpdateUniforms(VulkanDevice* pDevice)
{
    void* data;
    vkMapMemory(pDevice->m_vkLogicalDevice, uniformDataBuffer.memory, 0, sizeof(RTUniformData), 0, &data);
    memcpy(data, &uniformData, sizeof(RTUniformData));
    vkUnmapMemory(pDevice->m_vkLogicalDevice, uniformDataBuffer.memory);
}

//---------------------------------------------------------------------------------------------------------------------
void RTShaderUniforms::Cleanup(VulkanDevice* pDevice)
{
    uniformDataBuffer.Cleanup(pDevice);
}

//---------------------------------------------------------------------------------------------------------------------
void RTShaderUniforms::CleanupOnWindowResize(VulkanDevice* pDevice)
{
}
