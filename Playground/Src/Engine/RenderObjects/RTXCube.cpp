#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"
#include "RTXCube.h"

//---------------------------------------------------------------------------------------------------------------------
RTXCube::RTXCube()
{
    m_vecVertices.clear();
    m_vecIndices.clear();
}

//---------------------------------------------------------------------------------------------------------------------
RTXCube::~RTXCube()
{
    
}

//---------------------------------------------------------------------------------------------------------------------
void RTXCube::Initialize(VulkanDevice* pDevice)
{
    // Get the ray tracing & AS related function ptrs
    vkCreateAccelerationStructureKHR            = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(pDevice->m_vkLogicalDevice, "vkCreateAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizesKHR     = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(pDevice->m_vkLogicalDevice, "vkGetAccelerationStructureBuildSizesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR  = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(pDevice->m_vkLogicalDevice, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCmdBuildAccelerationStructuresKHR         = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(pDevice->m_vkLogicalDevice, "vkCmdBuildAccelerationStructuresKHR"));

    // Vertex data
    m_vecVertices.reserve(8);
    m_vecVertices.emplace_back(glm::vec3(-1, -1,  1));
    m_vecVertices.emplace_back(glm::vec3( 1, -1,  1));
    m_vecVertices.emplace_back(glm::vec3( 1,  1,  1));
    m_vecVertices.emplace_back(glm::vec3(-1,  1,  1));
    m_vecVertices.emplace_back(glm::vec3(-1, -1, -1));
    m_vecVertices.emplace_back(glm::vec3( 1, -1, -1));
    m_vecVertices.emplace_back(glm::vec3( 1,  1, -1));
    m_vecVertices.emplace_back(glm::vec3(-1,  1, -1));

    // Index data
    m_vecIndices.resize(36);
    m_vecIndices[0] = 0;        m_vecIndices[1] = 1;        m_vecIndices[2] = 2;
    m_vecIndices[3] = 2;        m_vecIndices[4] = 3;        m_vecIndices[5] = 0;

    m_vecIndices[6] = 3;        m_vecIndices[7] = 2;        m_vecIndices[8] = 6;
    m_vecIndices[9] = 6;        m_vecIndices[10] = 7;        m_vecIndices[11] = 3;

    m_vecIndices[12] = 7;        m_vecIndices[13] = 6;        m_vecIndices[14] = 5;
    m_vecIndices[15] = 5;        m_vecIndices[16] = 4;        m_vecIndices[17] = 7;

    m_vecIndices[18] = 4;        m_vecIndices[19] = 5;        m_vecIndices[20] = 1;
    m_vecIndices[21] = 1;        m_vecIndices[22] = 0;        m_vecIndices[23] = 4;

    m_vecIndices[24] = 4;        m_vecIndices[25] = 0;        m_vecIndices[26] = 3;
    m_vecIndices[27] = 3;        m_vecIndices[28] = 7;        m_vecIndices[29] = 4;

    m_vecIndices[30] = 1;        m_vecIndices[31] = 5;        m_vecIndices[32] = 6;
    m_vecIndices[33] = 6;        m_vecIndices[34] = 2;        m_vecIndices[35] = 1;

    // Set defaults
   m_pMeshData = new Vulkan::MeshData();
   m_pMeshInstanceData = new Vulkan::MeshInstance();
   
   CreateBottomLevelAS(pDevice);
}

//---------------------------------------------------------------------------------------------------------------------
void RTXCube::Update(float dt)
{
    static float angle = 0.0f;
    angle += dt;

    m_pMeshInstanceData->Update(dt);
}

//---------------------------------------------------------------------------------------------------------------------
void RTXCube::Render()
{
}

//---------------------------------------------------------------------------------------------------------------------
void RTXCube::Cleanup(VulkanDevice* pDevice)
{
    m_pMeshData->Cleanup(pDevice);
    m_BottomLevelAS.Cleanup(pDevice);

    m_vecVertices.clear();
    m_vecIndices.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void RTXCube::CreateBottomLevelAS(VulkanDevice* pDevice)
{
    // 2. Create buffers for Mesh Data
    // Create VB
    pDevice->CreateBufferAndCopyData(m_vecVertices.size() * sizeof(App::VertexP),
                                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                     &(m_pMeshData->vertexBuffer.buffer),
                                     &(m_pMeshData->vertexBuffer.memory),
                                     m_vecVertices.data(),
                                     "BLAS_CUBE_VB");

    // Create IB
    pDevice->CreateBufferAndCopyData(m_vecIndices.size() * sizeof(uint32_t),
                                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                     &(m_pMeshData->indexBuffer.buffer),
                                     &(m_pMeshData->indexBuffer.memory),
                                     m_vecIndices.data(),
                                     "BLAS_CUBE_IB");


    // 3. Get Device addresses of buffers just created
    VkDeviceOrHostAddressConstKHR vbAddress = {};
    VkDeviceOrHostAddressConstKHR ibAddress = {};
    VkDeviceOrHostAddressConstKHR trAddress = {};

    vbAddress.deviceAddress = Vulkan::GetBufferDeviceAddress(pDevice, m_pMeshData->vertexBuffer.buffer);
    ibAddress.deviceAddress = Vulkan::GetBufferDeviceAddress(pDevice, m_pMeshData->indexBuffer.buffer);
    //trAddress.deviceAddress = Vulkan::GetBufferDeviceAddress(pDevice, m_TransformBuffer.buffer);

    m_pMeshInstanceData->verticesAddress  = vbAddress.deviceAddress;
    m_pMeshInstanceData->indicesAddress = ibAddress.deviceAddress;
  

    // 4. Define AS Geometry by providing vb, ib & tb addresses 
    VkAccelerationStructureGeometryKHR accelStructureGeometry = {};
    accelStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelStructureGeometry.geometry.triangles.vertexData = vbAddress;
    accelStructureGeometry.geometry.triangles.maxVertex = static_cast<uint32_t>(m_vecVertices.size());
    accelStructureGeometry.geometry.triangles.vertexStride = sizeof(App::VertexP);
    accelStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    accelStructureGeometry.geometry.triangles.indexData = ibAddress;
    accelStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
    accelStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
    accelStructureGeometry.geometry.triangles.transformData = trAddress;

    // 5. Get AS Build size estimate
    VkAccelerationStructureBuildGeometryInfoKHR accelStructBuildGeomInfo = {};
    accelStructBuildGeomInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelStructBuildGeomInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelStructBuildGeomInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | 
                                     VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | 
                                     VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    accelStructBuildGeomInfo.geometryCount = 1;
    accelStructBuildGeomInfo.pGeometries = &accelStructureGeometry;

    const uint32_t nTriangles = static_cast<uint32_t>(m_vecIndices.size()) / 3;
    VkAccelerationStructureBuildSizesInfoKHR accelStructBuildSizesInfo = {};
    accelStructBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    vkGetAccelerationStructureBuildSizesKHR(pDevice->m_vkLogicalDevice,
                                            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &accelStructBuildGeomInfo,
                                            &nTriangles,
                                            &accelStructBuildSizesInfo);

    // 6. Create buffer for holding AS and Create AS handle!
    pDevice->CreateBuffer(accelStructBuildSizesInfo.accelerationStructureSize,
                          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                          VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
                          &m_BottomLevelAS.buffer,
                          &m_BottomLevelAS.memory,
                          "BLAS_CUBE");

    VkAccelerationStructureCreateInfoKHR accelStructCreateInfo = {};
    accelStructCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelStructCreateInfo.buffer = m_BottomLevelAS.buffer;
    accelStructCreateInfo.size = accelStructBuildSizesInfo.accelerationStructureSize;
    accelStructCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    VKRESULT_CHECK_INFO(vkCreateAccelerationStructureKHR(pDevice->m_vkLogicalDevice,
                                                         &accelStructCreateInfo,
                                                         nullptr,
                                                         &m_BottomLevelAS.handle),
                       "Failed to create Cube BLAS",
                       "Successfully created Cube BLAS!");  

    // 7. Create a small scratch buffer used during build of BLAS
    Vulkan::RTScratchBuffer scratchBuffer;

    pDevice->CreateBuffer(accelStructBuildSizesInfo.buildScratchSize,
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          &scratchBuffer.handle,
                          &scratchBuffer.memory,
                          "BLAS_SCRATCH_BUFFER");

    scratchBuffer.deviceAddress = Vulkan::GetBufferDeviceAddress(pDevice, scratchBuffer.handle);

    VkAccelerationStructureBuildGeometryInfoKHR accelStructBuildGeomInfo2 = {};
    accelStructBuildGeomInfo2.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelStructBuildGeomInfo2.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelStructBuildGeomInfo2.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelStructBuildGeomInfo2.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelStructBuildGeomInfo2.dstAccelerationStructure = m_BottomLevelAS.handle;
    accelStructBuildGeomInfo2.geometryCount = 1;
    accelStructBuildGeomInfo2.pGeometries = &accelStructureGeometry;
    accelStructBuildGeomInfo2.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR accelStructBuildRangeInfo = {};
    accelStructBuildRangeInfo.primitiveCount = nTriangles;
    accelStructBuildRangeInfo.primitiveOffset = 0;
    accelStructBuildRangeInfo.firstVertex = 0;
    accelStructBuildRangeInfo.transformOffset = 0;

    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> vecAccelStructRangeInfos;
    vecAccelStructRangeInfos.push_back(&accelStructBuildRangeInfo);

    VkCommandBuffer commandBuffer = pDevice->BeginCommandBuffer();

    // Accel Struct needs to be built on Device!
    vkCmdBuildAccelerationStructuresKHR(commandBuffer,
                                        static_cast<uint32_t>(vecAccelStructRangeInfos.size()),
                                        &accelStructBuildGeomInfo2,
                                        vecAccelStructRangeInfos.data()),

    pDevice->EndAndSubmitCommandBuffer(commandBuffer);

    // 9. Finally, get hold of device address of BLAS!
    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = m_BottomLevelAS.handle;
    m_BottomLevelAS.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(pDevice->m_vkLogicalDevice, &accelerationDeviceAddressInfo);

    // 10. Scratch buffer no lonoger needed!
    scratchBuffer.Cleanup(pDevice);
}
