#pragma once

#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "vulkan/vulkan.h"

#include "Engine/Helpers/Log.h"
#include "Engine/Renderer/VulkanDevice.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"

//-----------------------------------------------------------------------------------------------------------------------
namespace App
{
	const uint32_t	MAX_FRAME_DRAWS = 2;
	const float WINDOW_WIDTH = 960.0f;
	const float WINDOW_HEIGHT = 540.0f;

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Vertex data with Position
	struct VertexP
	{
		VertexP() { Position = glm::vec3(0); }
		VertexP(const glm::vec3& _pos) :
			Position(_pos) {}

		glm::vec3 Position;			// Vertex position X, Y, Z
	};

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Vertex data with Position, Color
	struct VertexPC
	{
		VertexPC() { Position = glm::vec3(0);  Color = glm::vec3(0); }
		VertexPC(const glm::vec3& _pos, const glm::vec3& _col) :
			Position(_pos),
			Color(_col) {}

		glm::vec3 Position;			// Vertex position X, Y, Z
		glm::vec3 Color;			// Vertex Color R, G, B
	};

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Vertex data with Position, UV
	struct VertexPT
	{
		VertexPT() { Position = glm::vec3(0);  UV = glm::vec2(0); }
		VertexPT(const glm::vec3& _pos, const glm::vec2& _uv) :
			Position(_pos),
			UV(_uv) {}

		glm::vec3 Position;			// Vertex position X, Y, Z
		glm::vec2 UV;				// Texture coordinates U,V
	};

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Vertex data with Position, Color, UV
	struct VertexPCT
	{
		VertexPCT() { Position = glm::vec3(0);  Color = glm::vec3(0);  UV = glm::vec2(0); }
		VertexPCT(const glm::vec3& _pos, const glm::vec3& _color, const glm::vec2& _uv) :
			Position(_pos),
			Color(_color),
			UV(_uv) {}

		glm::vec3 Position;			// Vertex position X, Y, Z
		glm::vec3 Color;			// Vertex Color R, G, B
		glm::vec2 UV;				// Texture coordinates U,V
	};

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Vertex data with Position, Normal, UV
	struct VertexPNT
	{
		VertexPNT() { Position = glm::vec3(0);  Normal = glm::vec3(1);  UV = glm::vec2(0); }
		VertexPNT(const glm::vec3& _pos, const glm::vec3& _normal, const glm::vec2& _uv) :
			Position(_pos),
			Normal(_normal),
			UV(_uv) {}

		glm::vec3 Position;			// Vertex position X, Y, Z
		glm::vec3 Normal;			// Vertex Normal X, Y, Z
		glm::vec2 UV;				// Texture coordinates U,V
	};

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Vertex data with Position, Normal, Tangent, BiNormal, Texcoords
	struct VertexPNTBT
	{
		VertexPNTBT() { Position = glm::vec3(0);  Normal = glm::vec3(0);  Tangent = glm::vec3(0); BiNormal = glm::vec3(0); UV = glm::vec2(0); }
		VertexPNTBT(const glm::vec3& _pos, const glm::vec3& _normal, const glm::vec3& _tangent, const glm::vec3& _binormal, const glm::vec2& _uv) :
			Position(_pos),
			Normal(_normal),
			Tangent(_tangent),
			BiNormal(_binormal),
			UV(_uv) {}

		glm::vec3 Position;			// Vertex position X, Y, Z
		glm::vec3 Normal;			// Normals
		glm::vec3 Tangent;			// Tangents
		glm::vec3 BiNormal;			// BiNormals
		glm::vec2 UV;				// Texture coordinates U,V
	};
}


//-----------------------------------------------------------------------------------------------------------------------
namespace Vulkan
{
	//-----------------------------------------------------------------------------------------------------------------------
	//--- list of validation layers...
	const std::vector<const char*> g_strValidationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

#ifdef _DEBUG
	const bool g_bEnableValidationLayer = true;
#elif
	const bool g_bEnableValidationLayer = false;
#endif

	//-----------------------------------------------------------------------------------------------------------------------
	//-- list of device extensions...
	const std::vector<const char*> g_strDeviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,

		// For Raytracing!
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,

		// Required by VK_KHR_acceleration_structure
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,

		// Required by VK_KHR_ray_tracing_pipeline
		VK_KHR_SPIRV_1_4_EXTENSION_NAME,

		// Required by VK_KHR_spirv_1_4
		VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
	};

	//-----------------------------------------------------------------------------------------------------------------------
	//-- RayTracing Scratch Buffer
	struct RTScratchBuffer
	{
		RTScratchBuffer()
		{
			deviceAddress	= 0;
			handle			= VK_NULL_HANDLE;
			memory			= VK_NULL_HANDLE;
		}

		void Cleanup(VulkanDevice* pDevice)
		{
			vkFreeMemory(pDevice->m_vkLogicalDevice, memory, nullptr);
			vkDestroyBuffer(pDevice->m_vkLogicalDevice, handle, nullptr);
			deviceAddress = 0;
		}

		uint64_t deviceAddress;
		VkBuffer handle;
		VkDeviceMemory memory;
	};


	//-----------------------------------------------------------------------------------------------------------------------
	//-- RayTracing Acceleration Structure
	struct RTAccelerationStructure
	{
		RTAccelerationStructure()
		{
			handle			= VK_NULL_HANDLE;
			deviceAddress	= 0;
			buffer			= VK_NULL_HANDLE;
			memory			= VK_NULL_HANDLE;
		}

		void Cleanup(VulkanDevice* pDevice)
		{
			PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>
			(vkGetDeviceProcAddr(pDevice->m_vkLogicalDevice, "vkDestroyAccelerationStructureKHR"));

			vkFreeMemory(pDevice->m_vkLogicalDevice, memory, nullptr);
			vkDestroyBuffer(pDevice->m_vkLogicalDevice, buffer, nullptr);
			vkDestroyAccelerationStructureKHR(pDevice->m_vkLogicalDevice, handle, nullptr);
			deviceAddress = 0;
		}

		VkAccelerationStructureKHR handle;
		uint64_t deviceAddress;
		VkBuffer buffer;
		VkDeviceMemory memory;
	};

	//-----------------------------------------------------------------------------------------------------------------------
	struct RTStorageImage
	{
		RTStorageImage()
		{
			memory = VK_NULL_HANDLE;
			image = VK_NULL_HANDLE;
			imageView = VK_NULL_HANDLE;
		}

		void Cleanup(VulkanDevice* pDevice)
		{
			vkDestroyImageView(pDevice->m_vkLogicalDevice, imageView, nullptr);
			vkDestroyImage(pDevice->m_vkLogicalDevice, image, nullptr);
			vkFreeMemory(pDevice->m_vkLogicalDevice, memory, nullptr);
		}

		void CleanupOnWindowResize(VulkanDevice* pDevice)
		{
			vkDestroyImageView(pDevice->m_vkLogicalDevice, imageView, nullptr);
			vkDestroyImage(pDevice->m_vkLogicalDevice, image, nullptr);
			vkFreeMemory(pDevice->m_vkLogicalDevice, memory, nullptr);
		}

		VkDeviceMemory  memory;
		VkImage         image;
		VkImageView     imageView;
	};

	//-----------------------------------------------------------------------------------------------------------------------
	// Generic Vulkan Buffer
	struct Buffer
	{
		Buffer()
		{
			buffer		= VK_NULL_HANDLE;
			memory		= VK_NULL_HANDLE;
			size		= 0;
			alignment	= 0;
		}

		void Cleanup(VulkanDevice*	pDevice)
		{
			vkFreeMemory(pDevice->m_vkLogicalDevice, memory, nullptr);
			vkDestroyBuffer(pDevice->m_vkLogicalDevice, buffer, nullptr);
		}

		VkBuffer				buffer;
		VkDeviceMemory			memory;
		VkDeviceSize			size;
		VkDeviceSize			alignment;
		VkBufferUsageFlags		usageFlags;
		VkMemoryPropertyFlags	memPropertyFlags;
	};


	//-----------------------------------------------------------------------------------------------------------------------
	//--- Create Shader Module
	inline VkShaderModule CreateShaderModule(VulkanDevice* pDevice, const std::string& fileName)
	{
		// start reading at the end & in binary mode.
		// Advantage of reading file from the end is we can use read position to determine
		// size of the file & allocate buffer accordingly!
		std::ifstream file(fileName, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			LOG_ERROR("Failed to open Shader file!");

		// get the file size & allocate buffer memory!
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		// now seek back to the beginning of the file & read all bytes at once!
		file.seekg(0);
		file.read(buffer.data(), fileSize);

		// close the file!
		file.close();

		// Create Shader Module
		VkShaderModuleCreateInfo shaderModuleInfo;
		shaderModuleInfo.codeSize = buffer.size();
		shaderModuleInfo.flags = 0;
		shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
		shaderModuleInfo.pNext = nullptr;
		shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		VkShaderModule shaderModule;
		std::string shaderModuleName = fileName;

		VKRESULT_CHECK_INFO(vkCreateShaderModule(pDevice->m_vkLogicalDevice, &shaderModuleInfo, nullptr, &shaderModule),
							"Failed to create shader module for " + shaderModuleName,
							"Successfully created shader module for " + shaderModuleName);

		return shaderModule;
	}

	//-----------------------------------------------------------------------------------------------------------------------
	inline void SetDebugUtilsObjectName(VkDevice pDevice, VkObjectType objType, uint64_t objHandle, const std::string& objName)
	{
		PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = 
		reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(pDevice, "vkSetDebugUtilsObjectNameEXT"));

		// This is for debugging purpose only, using debugutils extension allows us to have debug name
		// during validation layer error reporting!
		VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = objType;
		nameInfo.objectHandle = (uint64_t)objHandle;
		nameInfo.pObjectName = objName.c_str();

		vkSetDebugUtilsObjectNameEXT(pDevice, &nameInfo);
	}

	//-----------------------------------------------------------------------------------------------------------------------
	inline uint32_t alignedSize(uint32_t value, uint32_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Generic get buffer device address
	inline VkDeviceAddress GetBufferDeviceAddress(VulkanDevice* pDevice, VkBuffer buffer)
	{
		VkBufferDeviceAddressInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		info.buffer = buffer;

		return vkGetBufferDeviceAddress(pDevice->m_vkLogicalDevice, &info);
	}

	//-----------------------------------------------------------------------------------------------------------------------
	inline VkFormat ChooseSupportedFormats(VulkanDevice* pDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
	{
		// Loop through options & find the compatible one
		for (VkFormat format : formats)
		{
			// Get properties for given formats on this device
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(pDevice->m_vkPhysicalDevice, format, &properties);

			// depending on tiling choice, need to check for different bit flag
			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
			{
				return format;
			}

			LOG_ERROR("Failed to find matching format!");
		}
	}

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Generic Copy Image buffer from srcBuffer to VkImage using transferQueue & transferCommandPool of specific width-height!
	inline void CopyImageBuffer(VulkanDevice* pDevice, VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height)
		{
			//Create buffer
			VkCommandBuffer transferCommandBuffer = pDevice->BeginCommandBuffer();

			VkBufferImageCopy imageRegion = {};
			imageRegion.bufferOffset = 0;											// offset into data
			imageRegion.bufferRowLength = 0;										// row length of data to calculate data spacing
			imageRegion.bufferImageHeight = 0;										// image height to calculate data spacing
			imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// which aspect of image to copy
			imageRegion.imageSubresource.mipLevel = 0;								// mipmap level to copy
			imageRegion.imageSubresource.baseArrayLayer = 0;						// starting array layer (if array)
			imageRegion.imageSubresource.layerCount = 1;							// number of layers to copy starting at baseArrayLayer
			imageRegion.imageOffset = { 0,0,0 };									// offset into image (as a opposed to raw data in the buffer)
			imageRegion.imageExtent = { width, height, 1 };							// size of region to copy as (x,y,z) values

			// copy buffer to given image
			vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

			pDevice->EndAndSubmitCommandBuffer(transferCommandBuffer);
		}

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Function to transition image layout from old to new image layout using command pool for VkImage!
	inline void TransitionImageLayout(VulkanDevice* pDevice, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout)
		{
			VkCommandBuffer commandBuffer = pDevice->BeginCommandBuffer();

			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = oldImageLayout;								// Layout to transition from
			imageMemoryBarrier.newLayout = newImageLayout;								// Layout to transition to
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
			imageMemoryBarrier.image = image;											// Image being accessed & modified as a part of barrier
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Aspect of image being altered
			imageMemoryBarrier.subresourceRange.baseMipLevel = 0;						// First mip level to start alteration on
			imageMemoryBarrier.subresourceRange.levelCount = 1;							// Number of mip levels to alter starting from base mip level
			imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;						// First layer of start alterations on
			imageMemoryBarrier.subresourceRange.layerCount = 1;							// Number of layers to alter starting from base array layer

			VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

			// If transitioning from new image to image ready to receive data...
			//if (oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			//{
			//	imageMemoryBarrier.srcAccessMask = 0;									// memory access stage transition must happen after this stage
			//	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;		// memory access stage transition must happen before this stage
			//
			//	srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			//	dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			//}
			//// If transitioning from transfer destination to shader readable...
			//else if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			//{
			//	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			//	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			//
			//	srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			//	dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			//}

			// Credit = Sascha Willems : VulkanTools.cpp!
			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			// ? Added srcStage logic too, only where it's required! 
			switch (oldImageLayout)
			{
				case VK_IMAGE_LAYOUT_UNDEFINED:
				{
					// Image layout is undefined (or does not matter)
					// Only valid as initial layout
					// No flags required, listed only for completeness
					imageMemoryBarrier.srcAccessMask = 0;
					srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					break;
				}
					
				case VK_IMAGE_LAYOUT_PREINITIALIZED:
				{
					// Image is preinitialized
					// Only valid as initial layout for linear images, preserves memory contents
					// Make sure host writes have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
					break;
				}
					
				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				{
					// Image is a color attachment
					// Make sure any writes to the color buffer have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
					break;
				}
					
				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				{
					// Image is a depth/stencil attachment
					// Make sure any writes to the depth/stencil buffer have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					break;
				}

				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				{
					// Image is a transfer source
					// Make sure any reads from the image have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					break;
				}
					
				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				{
					// Image is a transfer destination
					// Make sure any writes to the image have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					break;
				}
					
				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				{
					// Image is read by a shader
					// Make sure any shader reads from the image have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;
				}
					
				default:
					// Other source layouts aren't handled (yet)
					break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				{
					// Image will be used as a transfer destination
					// Make sure any writes to the image have been finished
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					break;
				}
				
				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				{
					// Image will be used as a transfer source
					// Make sure any reads from the image have been finished
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					break;
				}
				
				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				{
					// Image will be used as a color attachment
					// Make sure any writes to the color buffer have been finished
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
					break;
				}
					
				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				{
					// Image layout will be used as a depth/stencil attachment
					// Make sure any writes to depth/stencil buffer have been finished
					imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					break;
				}
				
				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				{
					// Image will be read in a shader (sampler, input attachment)
					// Make sure any writes to the image have been finished
					if (imageMemoryBarrier.srcAccessMask == 0)
					{
						imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
					}
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
					
					break;
				}
					
				default:
					// Other source layouts aren't handled (yet)
					break;
			}

			vkCmdPipelineBarrier(commandBuffer,
				srcStage, dstStage,		// Pipeline stages (match to src & dest AcccessMask)
				0,						// Dependency flags
				0, nullptr,				// Memory barrier count + data
				0, nullptr,				// Buffer memory barrier count + data
				1, &imageMemoryBarrier);// image memmory barrier count + data

			pDevice->EndAndSubmitCommandBuffer(commandBuffer);
		}

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Function to transition image layout from old to new image layout using command pool for VkImage!
	inline void TransitionImageLayout(VulkanDevice* pDevice, VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout)
		{
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = oldImageLayout;								// Layout to transition from
			imageMemoryBarrier.newLayout = newImageLayout;								// Layout to transition to
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
			imageMemoryBarrier.image = image;											// Image being accessed & modified as a part of barrier
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Aspect of image being altered
			imageMemoryBarrier.subresourceRange.baseMipLevel = 0;						// First mip level to start alteration on
			imageMemoryBarrier.subresourceRange.levelCount = 1;							// Number of mip levels to alter starting from base mip level
			imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;						// First layer of start alterations on
			imageMemoryBarrier.subresourceRange.layerCount = 1;							// Number of layers to alter starting from base array layer

			VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

			// If transitioning from new image to image ready to receive data...
			//if (oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			//{
			//	imageMemoryBarrier.srcAccessMask = 0;									// memory access stage transition must happen after this stage
			//	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;		// memory access stage transition must happen before this stage
			//
			//	srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			//	dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			//}
			//// If transitioning from transfer destination to shader readable...
			//else if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			//{
			//	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			//	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			//
			//	srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			//	dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			//}

			// Credit = Sascha Willems : VulkanTools.cpp!
			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			// ? Added srcStage logic too, only where it's required! 
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
			{
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
			{
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			{
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			{
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			{
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			{
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			{
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			}

			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			{
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			{
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			{
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			{
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			{
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

				break;
			}

			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Put Barrier!
			vkCmdPipelineBarrier(
									cmdBuffer,
									srcStage, dstStage,		// Pipeline stages (match to src & dest AcccessMask)
									0,						// Dependency flags
									0, nullptr,				// Memory barrier count + data
									0, nullptr,				// Buffer memory barrier count + data
									1, &imageMemoryBarrier  // image memmory barrier count + data
								);
		}

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Function to transition image layout from old to new image layout using command pool for VkImage!
	inline void TransitionImageLayout(VulkanDevice* pDevice, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, 
										VkImageSubresourceRange subResRange)
		{
			VkCommandBuffer commandBuffer = pDevice->BeginCommandBuffer();

			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = oldImageLayout;								// Layout to transition from
			imageMemoryBarrier.newLayout = newImageLayout;								// Layout to transition to
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
			imageMemoryBarrier.image = image;											// Image being accessed & modified as a part of barrier
			imageMemoryBarrier.subresourceRange = subResRange;

			VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

			// If transitioning from new image to image ready to receive data...
			//if (oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			//{
			//	imageMemoryBarrier.srcAccessMask = 0;									// memory access stage transition must happen after this stage
			//	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;		// memory access stage transition must happen before this stage
			//
			//	srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			//	dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			//}
			//// If transitioning from transfer destination to shader readable...
			//else if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			//{
			//	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			//	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			//
			//	srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			//	dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			//}

			// Credit = Sascha Willems : VulkanTools.cpp!
			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			// ? Added srcStage logic too, only where it's required! 
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
			{
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;
			}

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
			{
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			{
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			{
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			{
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			{
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			{
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			}

			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			{
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			{
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			{
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			{
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			{
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				break;
			}

			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			vkCmdPipelineBarrier(commandBuffer,
				srcStage, dstStage,		// Pipeline stages (match to src & dest AcccessMask)
				0,						// Dependency flags
				0, nullptr,				// Memory barrier count + data
				0, nullptr,				// Buffer memory barrier count + data
				1, &imageMemoryBarrier);// image memmory barrier count + data

			pDevice->EndAndSubmitCommandBuffer(commandBuffer);
		}

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Function to transition image layout from old to new image layout using command pool for VkImage!
	inline void TransitionImageLayout(	VulkanDevice* pDevice, VkCommandBuffer cmdBuffer, VkImage image, 
										VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subResRange)
		{
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = oldImageLayout;								// Layout to transition from
			imageMemoryBarrier.newLayout = newImageLayout;								// Layout to transition to
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
			imageMemoryBarrier.image = image;											// Image being accessed & modified as a part of barrier
			imageMemoryBarrier.subresourceRange = subResRange;

			VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

			// Credit = Sascha Willems : VulkanTools.cpp!
			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			// ? Added srcStage logic too, only where it's required! 
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
			{
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;
			}

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
			{
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			{
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			{
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			{
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			{
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			{
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			}

			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			{
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			{
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			{
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			{
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			{
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				break;
			}

			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			vkCmdPipelineBarrier(
									cmdBuffer,
									srcStage, dstStage,		// Pipeline stages (match to src & dest AcccessMask)
									0,						// Dependency flags
									0, nullptr,				// Memory barrier count + data
									0, nullptr,				// Buffer memory barrier count + data
									1, &imageMemoryBarrier	// image memmory barrier count + data
								);
		}

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Create VkImage & VkDeviceMemory based on width-height-format-tiling-usageFlags-propertyFlags!
	inline VkImage CreateImage(VulkanDevice* pDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory)
	{
		// Image creation info
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;							// Type of image
		imageCreateInfo.extent.width = width;									// width of image extent
		imageCreateInfo.extent.height = height;									// height of image extent
		imageCreateInfo.extent.depth = 1;										// depth of image ( just 1, no 3D aspect) 
		imageCreateInfo.mipLevels = 1;											// number of mipmap levels
		imageCreateInfo.arrayLayers = 1;										// number of levels in image array
		imageCreateInfo.format = format;										// format type of image	
		imageCreateInfo.tiling = tiling;										// how image data should be tiled
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;				// layout of image data on creation
		imageCreateInfo.usage = usageFlags;										// bit flags defining what image will be used for 
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;						// number of samples for multi-sampling
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;				// whether image can be shared between queues

		// Create Image
		VkImage image;
		if (vkCreateImage(pDevice->m_vkLogicalDevice, &imageCreateInfo, nullptr, &image) != VK_SUCCESS)
			LOG_ERROR("Failed to create an image");

		//SetDebugUtilsObjectName(pDevice, VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(image), "ImageVulkan");

		// Get memory requirements for type of image
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(pDevice->m_vkLogicalDevice, image, &memoryRequirements);

		// Allocated memory using image requirements & user defined properties
		VkMemoryAllocateInfo memoryAllocInfo = {};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memoryRequirements.size;
		memoryAllocInfo.memoryTypeIndex = pDevice->FindMemoryTypeIndex(memoryRequirements.memoryTypeBits, propFlags);

		if (vkAllocateMemory(pDevice->m_vkLogicalDevice, &memoryAllocInfo, nullptr, imageMemory) != VK_SUCCESS)
			LOG_ERROR("Failed to allocated memory for image!");

		// Connect memory to image
		vkBindImageMemory(pDevice->m_vkLogicalDevice, image, *imageMemory, 0);

		return image;
	}

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Create VkImageView for a VkImage
	inline VkImageView CreateImageView(const VulkanDevice* pDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
		{
			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.image = image;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = format;
			imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;

			// Create image view & return it
			VkImageView imageView;
			if (vkCreateImageView(pDevice->m_vkLogicalDevice, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
				LOG_ERROR("Failed to create an Image View");

			return imageView;
		}

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Create Cubemap VkImage & VkDeviceMemory based on width-height-format-tiling-usageFlags-propertyFlags!
	inline VkImage CreateImageCUBE(VulkanDevice* pDevice, uint32_t width, uint32_t height, VkFormat format, uint32_t nMipmaps, VkImageTiling tiling,
		VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory)
		{
			// Image creation info
			VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;							// Type of image
			imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;			// For cubemaps!
			imageCreateInfo.extent.width = width;									// width of image extent
			imageCreateInfo.extent.height = height;									// height of image extent
			imageCreateInfo.extent.depth = 1;										// depth of image ( just 1, no 3D aspect) 
			imageCreateInfo.mipLevels = nMipmaps;									// number of mipmap levels
			imageCreateInfo.arrayLayers = 6;										// 6 levels in image array for cubemap!
			imageCreateInfo.format = format;										// format type of image	
			imageCreateInfo.tiling = tiling;										// how image data should be tiled
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;				// layout of image data on creation
			imageCreateInfo.usage = usageFlags;										// bit flags defining what image will be used for 
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;						// number of samples for multi-sampling
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;				// whether image can be shared between queues

			// Create Image
			VkImage image;
			if (vkCreateImage(pDevice->m_vkLogicalDevice, &imageCreateInfo, nullptr, &image) != VK_SUCCESS)
				LOG_ERROR("Failed to create an image");

			// Get memory requirements for type of image
			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(pDevice->m_vkLogicalDevice, image, &memoryRequirements);

			// Allocated memory using image requirements & user defined properties
			VkMemoryAllocateInfo memoryAllocInfo = {};
			memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memoryAllocInfo.allocationSize = memoryRequirements.size;
			memoryAllocInfo.memoryTypeIndex = pDevice->FindMemoryTypeIndex(memoryRequirements.memoryTypeBits, propFlags);

			if (vkAllocateMemory(pDevice->m_vkLogicalDevice, &memoryAllocInfo, nullptr, imageMemory) != VK_SUCCESS)
				LOG_ERROR("Failed to allocated memory for image!");

			// Connect memory to image
			vkBindImageMemory(pDevice->m_vkLogicalDevice, image, *imageMemory, 0);

			return image;
		}

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Create Cubemap VkImageView for a VkImage
	inline VkImageView CreateImageViewCUBE(const VulkanDevice* pDevice, VkImage image, VkFormat format, uint32_t nMipmaps, VkImageAspectFlags aspectFlags)
		{
			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.image = image;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			imageViewCreateInfo.format = format;
			imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = nMipmaps;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 6;

			// Create image view & return it
			VkImageView imageView;
			if (vkCreateImageView(pDevice->m_vkLogicalDevice, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
				LOG_ERROR("Failed to create an Image View");

			return imageView;
		}

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Function to transition Cubemap image layout from old to new image layout using command pool for VkImage!
	inline void TransitionImageLayoutCUBE(VulkanDevice* pDevice, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout)
		{
			VkCommandBuffer commandBuffer = pDevice->BeginCommandBuffer();

			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = oldImageLayout;								// Layout to transition from
			imageMemoryBarrier.newLayout = newImageLayout;								// Layout to transition to
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
			imageMemoryBarrier.image = image;											// Image being accessed & modified as a part of barrier
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Aspect of image being altered
			imageMemoryBarrier.subresourceRange.baseMipLevel = 0;						// First mip level to start alteration on
			imageMemoryBarrier.subresourceRange.levelCount = 1;							// Number of mip levels to alter starting from base mip level
			imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;						// First layer of start alterations on
			imageMemoryBarrier.subresourceRange.layerCount = 6;							// Number of layers to alter starting from base array layer

			VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

			// Credit = Sascha Willems : VulkanTools.cpp!
			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			// ? Added srcStage logic too, only where it's required! 
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
			{
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;
			}

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
			{
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			{
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			{
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			{
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			{
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			{
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			}

			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			{
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			{
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			{
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			{
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			{
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				break;
			}

			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			vkCmdPipelineBarrier(commandBuffer,
								 srcStage, dstStage,			// Pipeline stages (match to src & dest AcccessMask)
								 0,							// Dependency flags
								 0, nullptr,					// Memory barrier count + data
								 0, nullptr,					// Buffer memory barrier count + data
								 1, &imageMemoryBarrier);	// image memmory barrier count + data

			pDevice->EndAndSubmitCommandBuffer(commandBuffer);
		}

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Function to transition Cubemap image layout from old to new image layout using command pool for VkImage!
	inline void TransitionImageLayoutCUBE(VulkanDevice* pDevice, VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subResRange)
		{
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = oldImageLayout;								// Layout to transition from
			imageMemoryBarrier.newLayout = newImageLayout;								// Layout to transition to
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
			imageMemoryBarrier.image = image;											// Image being accessed & modified as a part of barrier
			imageMemoryBarrier.subresourceRange = subResRange;

			VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

			// Credit = Sascha Willems : VulkanTools.cpp!
			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			// ? Added srcStage logic too, only where it's required! 
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
			{
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;
			}

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
			{
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			{
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			{
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			{
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			{
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			{
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			}

			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			{
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			{
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			{
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			{
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			}

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			{
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				break;
			}

			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			vkCmdPipelineBarrier(cmdBuffer,
				srcStage, dstStage,			// Pipeline stages (match to src & dest AcccessMask)
				0,							// Dependency flags
				0, nullptr,					// Memory barrier count + data
				0, nullptr,					// Buffer memory barrier count + data
				1, &imageMemoryBarrier);	// image memmory barrier count + data
		}

	//-----------------------------------------------------------------------------------------------------------------------
	//--- Generic Copy Image buffer from srcBuffer to VkImage using transferQueue & transferCommandPool of specific width-height!
	inline void CopyImageBufferCUBE(VulkanDevice* pDevice, VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height)
		{
			//Create buffer
			VkCommandBuffer transferCommandBuffer = pDevice->BeginCommandBuffer();

			VkBufferImageCopy imageRegion = {};
			imageRegion.bufferOffset = 0;											// offset into data
			imageRegion.bufferRowLength = 0;										// row length of data to calculate data spacing
			imageRegion.bufferImageHeight = 0;										// image height to calculate data spacing
			imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// which aspect of image to copy
			imageRegion.imageSubresource.mipLevel = 0;								// mipmap level to copy
			imageRegion.imageSubresource.baseArrayLayer = 0;						// starting array layer (if array)
			imageRegion.imageSubresource.layerCount = 6;							// number of layers to copy starting at baseArrayLayer
			imageRegion.imageOffset = { 0,0,0 };									// offset into image (as a opposed to raw data in the buffer)
			imageRegion.imageExtent = { width, height, 1 };							// size of region to copy as (x,y,z) values

			// copy buffer to given image
			vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

			pDevice->EndAndSubmitCommandBuffer(transferCommandBuffer);
		}
}


//-----------------------------------------------------------------------------------------------------------------
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

//-----------------------------------------------------------------------------------------------------------------
static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

//-----------------------------------------------------------------------------------------------------------------
//static VkDeviceAddress vkGetBufferDeviceAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo* pInfo)
//{
//	auto func = (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR");
//
//	if (func != nullptr)
//	{
//		return func(device, pInfo);
//	}
//	else
//	{
//		return 0;
//	}
//}
//
////-----------------------------------------------------------------------------------------------------------------
//static VkResult vkCreateAccelerationStructureKHR(VkDevice device, const VkAccelerationStructureCreateInfoKHR* pCreateInfo, 
//												 const VkAllocationCallbacks* pAllocator, VkAccelerationStructureKHR* pAccelerationStructure)
//{
//	auto func = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
//
//	if (func != nullptr)
//	{
//		return func(device, pCreateInfo, pAllocator, pAccelerationStructure);
//	}
//	else
//	{
//		return VK_ERROR_EXTENSION_NOT_PRESENT;
//	}
//}
//
////-----------------------------------------------------------------------------------------------------------------
//static void vkGetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType, 
//													const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo, 
//													const uint32_t* pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo)
//{
//	auto func = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
//
//	if (func != nullptr)
//	{
//		func(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
//	}
//}
////-----------------------------------------------------------------------------------------------------------------
//static VkDeviceAddress vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)
//{
//	auto func = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
//
//	if (func != nullptr)
//	{
//		return func(device, pInfo);
//	}
//	else
//	{
//		return 0;
//	}
//}
////-----------------------------------------------------------------------------------------------------------------
//static void vkCmdBuildAccelerationStructuresKHR(VkInstance instance, VkCommandBuffer commandBuffer, uint32_t infoCount,
//												const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
//												const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
//{
//	auto func = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetInstanceProcAddr(instance, "vkCmdBuildAccelerationStructuresKHR");
//
//	if (func != nullptr)
//	{
//		func(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
//	}
//}

//-----------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------------
//PFN_vkCmdTraceRaysKHR                               vkCmdTraceRaysKHR;
//PFN_vkGetRayTracingShaderGroupHandlesKHR            vkGetRayTracingShaderGroupHandlesKHR;
//PFN_vkCreateRayTracingPipelinesKHR                  vkCreateRayTracingPipelinesKHR;
