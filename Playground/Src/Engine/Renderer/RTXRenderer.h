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

class RTXRenderer : public VulkanRenderer
{
public:
    RTXRenderer();
    virtual ~RTXRenderer();

    virtual int						Initialize(GLFWwindow* pWindow) override;
    virtual void					Update(float dt) override;
    virtual void					Render() override;
    virtual void					Cleanup() override;

    virtual void					RecordCommands(uint32_t currentImage) override;
    virtual void					CreateSyncObjects() override;

    virtual void					HandleWindowResize() override;
    virtual void					CleanupOnWindowResize() override;

private:

    VkDeviceSize                    m_vkBufferSize;
    VkBuffer                        m_vkBufferImage;
    VkDeviceMemory                  m_vkDeviceMemoryImage;
    
};

