#include "PlaygroundPCH.h"
#include "RTXRenderer.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanTexture2D.h"
#include "VulkanGraphicsPipeline.h"
#include "Engine/RenderObjects/HDRISkydome.h"
#include "Engine/Scene.h"      
#include "Engine/Helpers/Utility.h"

//---------------------------------------------------------------------------------------------------------------------
RTXRenderer::RTXRenderer()
{
}

//---------------------------------------------------------------------------------------------------------------------
RTXRenderer::~RTXRenderer()
{
}

//---------------------------------------------------------------------------------------------------------------------
int RTXRenderer::Initialize(GLFWwindow* pWindow)
{
    VulkanRenderer::Initialize(pWindow);

    try
    {
        m_pDevice = new VulkanDevice(m_vkInstance, m_vkSurface);

        m_pDevice->PickPhysicalDevice();
        m_pDevice->CreateLogicalDevice();

        m_pSwapChain = new VulkanSwapChain();
        m_pSwapChain->CreateSwapChain(m_pDevice, m_vkSurface, m_pWindow);

        m_pDevice->CreateGraphicsCommandPool();
        m_pDevice->CreateGraphicsCommandBuffers(m_pSwapChain->m_vecSwapchainImages.size());

        // Create a buffer
        m_vkBufferSize = m_pSwapChain->m_vkSwapchainExtent.width * m_pSwapChain->m_vkSwapchainExtent.height * 3 * sizeof(float);
        m_pDevice->CreateBuffer(m_vkBufferSize, 
                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                                &m_vkBufferImage,
                                &m_vkDeviceMemoryImage);

        RecordCommands(0);

        // Get the image data back from the GPU
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        // copy image data to staging buffer
        void* data;
        vkMapMemory(m_pDevice->m_vkLogicalDevice, m_vkDeviceMemoryImage, 0, m_vkBufferSize, 0, (void**)&data);

        //float* fData = reinterpret_cast<float*>(data);
        stbi_write_hdr("out.hdr", 
                        m_pSwapChain->m_vkSwapchainExtent.width, 
                        m_pSwapChain->m_vkSwapchainExtent.height, 
                        3, reinterpret_cast<float*>(data));

        //memcpy(reinterpret_cast<float*>(data), m_vkBufferImage, m_vkBufferSize);
        vkUnmapMemory(m_pDevice->m_vkLogicalDevice, m_vkDeviceMemoryImage);
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
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::Render()
{
    VulkanRenderer::Render();

    
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::Cleanup()
{
    VulkanRenderer::Cleanup();
    vkDestroyCommandPool(m_pDevice->m_vkLogicalDevice, m_pDevice->m_vkCommandPoolGraphics, nullptr);
    
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::RecordCommands(uint32_t currentImage)
{
    VulkanRenderer::RecordCommands(currentImage);

    // Begin recording commands!
    VkCommandBuffer cmdBuffer = m_pDevice->BeginCommandBuffer();

    // Fill the buffer
    const float fillValue = 0.5f;
    const uint32_t& fillValueU32 = reinterpret_cast<const uint32_t&>(fillValue);
    vkCmdFillBuffer(cmdBuffer, m_vkBufferImage, 0, m_vkBufferSize, fillValueU32);

    VkMemoryBarrier memoryBarrier  = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    memoryBarrier.pNext = nullptr;

    vkCmdPipelineBarrier(cmdBuffer, 
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_HOST_BIT,
                         0,
                         1, &memoryBarrier,
                         0, nullptr, 0, nullptr);

    m_pDevice->EndAndSubmitCommandBuffer(cmdBuffer);
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::CreateSyncObjects()
{
    VulkanRenderer::CreateSyncObjects();
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::HandleWindowResize()
{
    VulkanRenderer::HandleWindowResize();
}

//---------------------------------------------------------------------------------------------------------------------
void RTXRenderer::CleanupOnWindowResize()
{
    VulkanRenderer::CleanupOnWindowResize();
}

