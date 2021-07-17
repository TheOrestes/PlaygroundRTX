#include "PlaygroundPCH.h"
#include "VulkanTextureCUBE.h"
#include "VulkanTexture2D.h"
#include "Engine/Renderer/VulkanDevice.h"
#include "Engine/Renderer/VulkanSwapChain.h"
#include "Engine/Renderer/VulkanGraphicsPipeline.h"
#include "Engine/RenderObjects/DummySkybox.h"
#include "Engine/Helpers/Utility.h"
#include "Engine/Helpers/Log.h"

#include "stb_image.h"

//---------------------------------------------------------------------------------------------------------------------
VulkanTextureCUBE::VulkanTextureCUBE()
{
	m_vkImageCUBE					= VK_NULL_HANDLE;
	m_vkImageViewCUBE				= VK_NULL_HANDLE;
	m_vkImageMemoryCUBE				= VK_NULL_HANDLE;
	m_vkSamplerCUBE					= VK_NULL_HANDLE;

	m_pTextureHDRI					= nullptr;
	m_pGraphicsPipelineHDRI2Cube	= nullptr;
	m_pGraphicsPipelineIrradiance	= nullptr;
	m_pGraphicsPipelinePrefilterSpec= nullptr;
	m_pDummySkybox					= nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
VulkanTextureCUBE::~VulkanTextureCUBE()
{
	SAFE_DELETE(m_pTextureHDRI);
	SAFE_DELETE(m_pGraphicsPipelineHDRI2Cube);
	SAFE_DELETE(m_pGraphicsPipelineIrradiance);
	SAFE_DELETE(m_pGraphicsPipelinePrefilterSpec);
	SAFE_DELETE(m_pDummySkybox);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateTextureCUBE(VulkanDevice* pDevice, std::string fileName)
{
	// Create Texture image!
	CreateTextureImage(pDevice, fileName);

	// Create Image View!
	m_vkImageViewCUBE = Helper::Vulkan::CreateImageViewCUBE(	pDevice,
																m_vkImageCUBE,
																VK_FORMAT_R8G8B8A8_UNORM,
																1,
																VK_IMAGE_ASPECT_COLOR_BIT);

	// Create Sampler!
	m_vkSamplerCUBE = CreateTextureSampler(pDevice, 1);
	
	LOG_DEBUG("Created Vulkan Cubemap Texture for {0}", fileName);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateTextureCubeFromHDRI(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, std::string fileName)
{
	// Load HDRI Map! 
	m_pTextureHDRI = new VulkanTexture2D();
	m_pTextureHDRI->CreateTexture(pDevice, fileName, TextureType::TEXTURE_HDRI);

	const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

	//*** Create VkImage with 6 array Layers!
	m_vkImageCUBE = Helper::Vulkan::CreateImageCUBE(pDevice,
													512,
													512,
													format,
													1,
													VK_IMAGE_TILING_OPTIMAL,
													VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_vkImageMemoryCUBE);

	// Create Image View!
	m_vkImageViewCUBE = Helper::Vulkan::CreateImageViewCUBE(pDevice,
															m_vkImageCUBE,
															format,
															1,
															VK_IMAGE_ASPECT_COLOR_BIT);

	// Create Sampler!
	m_vkSamplerCUBE = CreateTextureSampler(pDevice, 1);

	// Dummy skybox used for rendering HDRI into a cubemap!
	m_pDummySkybox = new DummySkybox();
	m_pDummySkybox->Init(pDevice);

	// Create RenderPass
	VkRenderPass hdr2CubeRenderPass = CreateOffscreenRenderPass(pDevice, format);

	// Create Off-screen framebuffer! 
	std::tuple<VkImage, VkImageView, VkDeviceMemory, VkFramebuffer> tupleOffscreen = CreateOffscreenFramebuffer(pDevice, hdr2CubeRenderPass, format, 512);

	// Create Descriptor Set layout & Descriptor Set!
	std::tuple<VkDescriptorPool, VkDescriptorSetLayout, VkDescriptorSet> tupleDescriptor = CreateHDRI2CubeDescriptorSet(pDevice);

	// Create Graphics Pipeline!
	CreateHDRI2CubePipeline(pDevice, pSwapchain, std::get<1>(tupleDescriptor), hdr2CubeRenderPass);

	// Render
	RenderHDRI2CUBE(pDevice, hdr2CubeRenderPass, std::get<3>(tupleOffscreen), m_vkImageCUBE, std::get<0>(tupleOffscreen), std::get<2>(tupleDescriptor), 512);

	// Cleanup!
	vkDestroyRenderPass(pDevice->m_vkLogicalDevice, hdr2CubeRenderPass, nullptr);
	vkDestroyFramebuffer(pDevice->m_vkLogicalDevice, std::get<3>(tupleOffscreen), nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, std::get<2>(tupleOffscreen), nullptr);
	vkDestroyImageView(pDevice->m_vkLogicalDevice, std::get<1>(tupleOffscreen), nullptr);
	vkDestroyImage(pDevice->m_vkLogicalDevice, std::get<0>(tupleOffscreen), nullptr);
	vkDestroyDescriptorPool(pDevice->m_vkLogicalDevice, std::get<0>(tupleDescriptor), nullptr);
	vkDestroyDescriptorSetLayout(pDevice->m_vkLogicalDevice, std::get<1>(tupleDescriptor), nullptr);

	m_pGraphicsPipelineHDRI2Cube->Cleanup(pDevice);
	m_pDummySkybox->Cleanup(pDevice);

	SAFE_DELETE(m_pGraphicsPipelineHDRI2Cube);
	SAFE_DELETE(m_pDummySkybox);
}

//---------------------------------------------------------------------------------------------------------------------
VkRenderPass VulkanTextureCUBE::CreateOffscreenRenderPass(VulkanDevice* pDevice, VkFormat format)
{
	VkRenderPass renderPass;

	VkAttachmentDescription attachDesc = {};

	// Color attachment
	attachDesc.format						= format;
	attachDesc.samples						= VK_SAMPLE_COUNT_1_BIT;
	attachDesc.loadOp						= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachDesc.storeOp						= VK_ATTACHMENT_STORE_OP_STORE;
	attachDesc.stencilLoadOp				= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachDesc.stencilStoreOp				= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachDesc.initialLayout				= VK_IMAGE_LAYOUT_UNDEFINED;
	attachDesc.finalLayout					= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkAttachmentReference	colorAttachRef	= {};
	colorAttachRef.attachment				= 0;
	colorAttachRef.layout					= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription	subpassDesc		= {};
	subpassDesc.pipelineBindPoint			= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount		= 1;
	subpassDesc.pColorAttachments			= &colorAttachRef;

	// Subpass dependencies
	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass				= VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass				= 0;
	dependencies[0].srcStageMask			= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask			= VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask			= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags			= VK_DEPENDENCY_BY_REGION_BIT;
	dependencies[1].srcSubpass				= 0;
	dependencies[1].dstSubpass				= VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask			= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask			= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask			= VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags			= VK_DEPENDENCY_BY_REGION_BIT;

	// Renderpass
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount	= 1;
	renderPassCreateInfo.pAttachments		= &attachDesc;
	renderPassCreateInfo.subpassCount		= 1;
	renderPassCreateInfo.pSubpasses			= &subpassDesc;
	renderPassCreateInfo.dependencyCount	= 2;
	renderPassCreateInfo.pDependencies		= dependencies.data();

	VkResult result = vkCreateRenderPass(pDevice->m_vkLogicalDevice, &renderPassCreateInfo, nullptr, &renderPass);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Renderpass for Irradiance Cubemap creation!");
		return VK_NULL_HANDLE;
	}

	return renderPass;
}

//---------------------------------------------------------------------------------------------------------------------
std::tuple<VkImage, VkImageView, VkDeviceMemory, VkFramebuffer> VulkanTextureCUBE::CreateOffscreenFramebuffer(VulkanDevice* pDevice, 
														VkRenderPass renderPass, VkFormat format, uint32_t dimension)
{
	VkImage			offscreenImage;
	VkImageView		offscreenImageView;
	VkDeviceMemory	offscreenImageMemory;
	VkFramebuffer	offscreenFramebuffer;

	// Color attachment
	offscreenImage = Helper::Vulkan::CreateImage(pDevice, dimension, dimension, format, VK_IMAGE_TILING_OPTIMAL,
												 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
												 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &offscreenImageMemory);

	offscreenImageView = Helper::Vulkan::CreateImageView(pDevice, offscreenImage, format, VK_IMAGE_ASPECT_COLOR_BIT);

	// Create Framebuffer
	VkFramebufferCreateInfo fbCreateInfo = {};
	fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbCreateInfo.renderPass = renderPass;
	fbCreateInfo.attachmentCount = 1;
	fbCreateInfo.pAttachments = &offscreenImageView;
	fbCreateInfo.width = dimension;
	fbCreateInfo.height = dimension;
	fbCreateInfo.layers = 1;

	VkResult result = vkCreateFramebuffer(pDevice->m_vkLogicalDevice, &fbCreateInfo, nullptr, &offscreenFramebuffer);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create offscreen framebuffer for Irradiance map!");
	}

	Helper::Vulkan::TransitionImageLayout(pDevice, offscreenImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	return std::make_tuple(offscreenImage, offscreenImageView, offscreenImageMemory, offscreenFramebuffer);
}

//---------------------------------------------------------------------------------------------------------------------
std::tuple<VkDescriptorPool, VkDescriptorSetLayout, VkDescriptorSet> VulkanTextureCUBE::CreateHDRI2CubeDescriptorSet(VulkanDevice* pDevice)
{
	VkResult result;
	VkDescriptorPool		descPool;
	VkDescriptorSetLayout	descSetLayout;
	VkDescriptorSet			descSet;

	// *** Create Descriptor pool
	std::array<VkDescriptorPoolSize, 1> arrDescriptorPoolSize = {};
	arrDescriptorPoolSize[0].descriptorCount = 1;
	arrDescriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.poolSizeCount = arrDescriptorPoolSize.size();
	poolCreateInfo.pPoolSizes = arrDescriptorPoolSize.data();
	poolCreateInfo.maxSets = 2;

	result = vkCreateDescriptorPool(pDevice->m_vkLogicalDevice, &poolCreateInfo, nullptr, &descPool);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Pool for HDRI2Cube!");
	}

	// *** Create Descriptor Set Layout
	std::array<VkDescriptorSetLayoutBinding, 1> arrDescriptorSetLayoutBindings = {};
	arrDescriptorSetLayoutBindings[0].binding = 0;
	arrDescriptorSetLayoutBindings[0].descriptorCount = 1;
	arrDescriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo descSetlayoutCreateInfo = {};
	descSetlayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetlayoutCreateInfo.bindingCount = arrDescriptorSetLayoutBindings.size();
	descSetlayoutCreateInfo.pBindings = arrDescriptorSetLayoutBindings.data();

	result = vkCreateDescriptorSetLayout(pDevice->m_vkLogicalDevice, &descSetlayoutCreateInfo, nullptr, &descSetLayout);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Set Layout for HDRI2Cube map!");
	}

	// *** Create Descriptor Set
	VkDescriptorSetAllocateInfo	setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = descPool;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &descSetLayout;

	result = vkAllocateDescriptorSets(pDevice->m_vkLogicalDevice, &setAllocInfo, &descSet);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Set for HDRI2Cube map!");
	}

	// *** Write Descriptor Sets (HDRI Image goes in as parameter to shader which then gets converted into CubeMap!)
	VkDescriptorImageInfo hdriImageInfo = {};
	hdriImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	hdriImageInfo.imageView = m_pTextureHDRI->m_vkTextureImageView;
	hdriImageInfo.sampler = m_pTextureHDRI->m_vkTextureSampler;

	// Descriptor write info
	VkWriteDescriptorSet hdriSetWrite = {};
	hdriSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	hdriSetWrite.dstSet = descSet;
	hdriSetWrite.dstBinding = 0;
	hdriSetWrite.dstArrayElement = 0;
	hdriSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	hdriSetWrite.descriptorCount = 1;
	hdriSetWrite.pImageInfo = &hdriImageInfo;

	// Update the descriptor sets with new buffer/binding info
	vkUpdateDescriptorSets(pDevice->m_vkLogicalDevice, 1, &hdriSetWrite, 0, nullptr);

	return std::make_tuple(descPool, descSetLayout, descSet);
}

//---------------------------------------------------------------------------------------------------------------------
std::tuple<VkDescriptorPool, VkDescriptorSetLayout, VkDescriptorSet> VulkanTextureCUBE::CreateIrradianceDescriptorSet(VulkanDevice* pDevice)
{
	VkResult result;
	VkDescriptorPool		descPool;
	VkDescriptorSetLayout	descSetLayout;
	VkDescriptorSet			descSet;

	// *** Create Descriptor pool
	std::array<VkDescriptorPoolSize, 1> arrDescriptorPoolSize = {};
	arrDescriptorPoolSize[0].descriptorCount = 1;
	arrDescriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.poolSizeCount = arrDescriptorPoolSize.size();
	poolCreateInfo.pPoolSizes = arrDescriptorPoolSize.data();
	poolCreateInfo.maxSets = 2;

	result = vkCreateDescriptorPool(pDevice->m_vkLogicalDevice, &poolCreateInfo, nullptr, &descPool);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Pool for Irradiance map!");
	}

	// *** Create Descriptor Set Layout
	std::array<VkDescriptorSetLayoutBinding, 1> arrDescriptorSetLayoutBindings = {};
	arrDescriptorSetLayoutBindings[0].binding = 0;
	arrDescriptorSetLayoutBindings[0].descriptorCount = 1;
	arrDescriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo descSetlayoutCreateInfo = {};
	descSetlayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetlayoutCreateInfo.bindingCount = arrDescriptorSetLayoutBindings.size();
	descSetlayoutCreateInfo.pBindings = arrDescriptorSetLayoutBindings.data();
	
	result = vkCreateDescriptorSetLayout(pDevice->m_vkLogicalDevice, &descSetlayoutCreateInfo, nullptr, &descSetLayout);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Set Layout for Irradiance map!");
	}

	// *** Create Descriptor Set
	VkDescriptorSetAllocateInfo	setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = descPool;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &descSetLayout;

	result = vkAllocateDescriptorSets(pDevice->m_vkLogicalDevice, &setAllocInfo, &descSet);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Set for Irradiance map!");
	}

	// *** Write Descriptor Sets (Cubemap Image goes in as parameter to shader which then gets converted into Irrad Map!)
	VkDescriptorImageInfo cubemapImageInfo = {};
	cubemapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;											
	cubemapImageInfo.imageView = m_vkImageViewCUBE;																	
	cubemapImageInfo.sampler = m_vkSamplerCUBE;			

	// Descriptor write info
	VkWriteDescriptorSet cubemapSetWrite = {};
	cubemapSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	cubemapSetWrite.dstSet = descSet;
	cubemapSetWrite.dstBinding = 0;
	cubemapSetWrite.dstArrayElement = 0;
	cubemapSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	cubemapSetWrite.descriptorCount = 1;
	cubemapSetWrite.pImageInfo = &cubemapImageInfo;

	// Update the descriptor sets with new buffer/binding info
	vkUpdateDescriptorSets(pDevice->m_vkLogicalDevice, 1, &cubemapSetWrite, 0, nullptr);

	return std::make_tuple(descPool, descSetLayout, descSet);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateHDRI2CubePipeline(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, VkDescriptorSetLayout descSetLayout, VkRenderPass renderPass)
{
	m_pGraphicsPipelineHDRI2Cube = new VulkanGraphicsPipeline(PipelineType::HDRI_CUBE, pSwapchain);

	// Descriptor Set layouts!
	std::vector<VkDescriptorSetLayout> setLayouts = { descSetLayout };

	// Push constants!
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(HDRIShaderPushData);

	std::vector<VkPushConstantRange> pushConstantRanges = { pushConstantRange };

	m_pGraphicsPipelineHDRI2Cube->CreatePipelineLayout(pDevice, setLayouts, pushConstantRanges);
	m_pGraphicsPipelineHDRI2Cube->CreateGraphicsPipeline(pDevice, pSwapchain, renderPass, 0, 1);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateIrradiancePipeline(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, VkDescriptorSetLayout descSetLayout, VkRenderPass renderPass)
{
	m_pGraphicsPipelineIrradiance = new VulkanGraphicsPipeline(PipelineType::IRRADIANCE_CUBE, pSwapchain);

	// Descriptor Set layouts!
	std::vector<VkDescriptorSetLayout> setLayouts = { descSetLayout };

	// Push constants!
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(IrradShaderPushData);

	std::vector<VkPushConstantRange> pushConstantRanges = { pushConstantRange };

	m_pGraphicsPipelineIrradiance->CreatePipelineLayout(pDevice, setLayouts, pushConstantRanges);
	m_pGraphicsPipelineIrradiance->CreateGraphicsPipeline(pDevice, pSwapchain, renderPass, 0, 1);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::RenderIrrandianceCUBE(VulkanDevice* pDevice, VkRenderPass renderPass, VkFramebuffer framebuffer, VkImage irradImage, VkImage offscreenImage, VkDescriptorSet irradDescSet, uint32_t dimension)
{
	std::array<VkClearValue, 1> arrClearValues;
	arrClearValues[0].color = { {0.8f, 0.8f, 0.8f, 0.0f} };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = framebuffer;
	renderPassBeginInfo.renderArea.extent.width = dimension;
	renderPassBeginInfo.renderArea.extent.height = dimension;
	renderPassBeginInfo.clearValueCount = arrClearValues.size();
	renderPassBeginInfo.pClearValues = arrClearValues.data();

	glm::mat4 captureViews[] =
	{
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	VkViewport vp = {};
	vp.width = (float)dimension;
	vp.height = (float)dimension;
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;

	VkRect2D scissorRect = {};
	scissorRect.extent.width = dimension;
	scissorRect.extent.height = dimension;
	scissorRect.offset.x = 0.0f;
	scissorRect.offset.y = 0.0f;

	IrradShaderPushData irradPushData;

	// Start recording commands to command buffer!
	VkCommandBuffer cmdBuffer = pDevice->BeginCommandBuffer();

	vkCmdSetViewport(cmdBuffer, 0, 1, &vp);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissorRect);

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 6;

	// Change image layout for all cubemap faces to transfer destination
	Helper::Vulkan::TransitionImageLayout(pDevice, cmdBuffer, irradImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

	// 6 faces of irradiance map!
	for (uint32_t f = 0; f < 6; f++)
	{
		// Render scene from cube face's point of view
		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		irradPushData.mvp = glm::perspective((float)(M_PI_OVER_TWO), 1.0f, 0.1f, 512.0f) * captureViews[f];
		vkCmdPushConstants(	cmdBuffer,
							m_pGraphicsPipelineIrradiance->m_vkPipelineLayout,
							VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
							0, sizeof(IrradShaderPushData),
							&irradPushData);	

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipelineIrradiance->m_vkGraphicsPipeline);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipelineIrradiance->m_vkPipelineLayout, 0, 1, &irradDescSet, 0, NULL);

		m_pDummySkybox->Render(pDevice, cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);

		Helper::Vulkan::TransitionImageLayout(pDevice, cmdBuffer, offscreenImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Copy region for transfer from framebuffer to cube face
		VkImageCopy copyRegion = {};

		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.mipLevel = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.srcOffset = { 0, 0, 0 };

		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.baseArrayLayer = f;
		copyRegion.dstSubresource.mipLevel = 0;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.dstOffset = { 0, 0, 0 };

		copyRegion.extent.width = static_cast<uint32_t>(vp.width);
		copyRegion.extent.height = static_cast<uint32_t>(vp.height);
		copyRegion.extent.depth = 1;

		vkCmdCopyImage(cmdBuffer, offscreenImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, irradImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,&copyRegion);

		// Transform framebuffer color attachment back
		Helper::Vulkan::TransitionImageLayout(pDevice, cmdBuffer, offscreenImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	// Transform Irradiance map to Shader readable optimal format!
	Helper::Vulkan::TransitionImageLayout(pDevice, cmdBuffer, irradImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

	pDevice->EndAndSubmitCommandBuffer(cmdBuffer);
}

//---------------------------------------------------------------------------------------------------------------------
std::tuple<VkDescriptorPool, VkDescriptorSetLayout, VkDescriptorSet> VulkanTextureCUBE::CreatePrefilteredSpecDescriptorSet(VulkanDevice* pDevice)
{
	VkResult result;
	VkDescriptorPool		descPool;
	VkDescriptorSetLayout	descSetLayout;
	VkDescriptorSet			descSet;

	// *** Create Descriptor pool
	std::array<VkDescriptorPoolSize, 1> arrDescriptorPoolSize = {};
	arrDescriptorPoolSize[0].descriptorCount = 1;
	arrDescriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.poolSizeCount = arrDescriptorPoolSize.size();
	poolCreateInfo.pPoolSizes = arrDescriptorPoolSize.data();
	poolCreateInfo.maxSets = 2;

	result = vkCreateDescriptorPool(pDevice->m_vkLogicalDevice, &poolCreateInfo, nullptr, &descPool);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Pool for Irradiance map!");
	}

	// *** Create Descriptor Set Layout
	std::array<VkDescriptorSetLayoutBinding, 1> arrDescriptorSetLayoutBindings = {};
	arrDescriptorSetLayoutBindings[0].binding = 0;
	arrDescriptorSetLayoutBindings[0].descriptorCount = 1;
	arrDescriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo descSetlayoutCreateInfo = {};
	descSetlayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetlayoutCreateInfo.bindingCount = arrDescriptorSetLayoutBindings.size();
	descSetlayoutCreateInfo.pBindings = arrDescriptorSetLayoutBindings.data();

	result = vkCreateDescriptorSetLayout(pDevice->m_vkLogicalDevice, &descSetlayoutCreateInfo, nullptr, &descSetLayout);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Set Layout for Irradiance map!");
	}

	// *** Create Descriptor Set
	VkDescriptorSetAllocateInfo	setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = descPool;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &descSetLayout;

	result = vkAllocateDescriptorSets(pDevice->m_vkLogicalDevice, &setAllocInfo, &descSet);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Set for Irradiance map!");
	}

	// *** Write Descriptor Sets (Cubemap Image goes in as parameter to shader which then gets converted into Prefiltered SpecMap!)
	VkDescriptorImageInfo cubemapImageInfo = {};
	cubemapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	cubemapImageInfo.imageView = m_vkImageViewCUBE;
	cubemapImageInfo.sampler = m_vkSamplerCUBE;

	// Descriptor write info
	VkWriteDescriptorSet cubemapSetWrite = {};
	cubemapSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	cubemapSetWrite.dstSet = descSet;
	cubemapSetWrite.dstBinding = 0;
	cubemapSetWrite.dstArrayElement = 0;
	cubemapSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	cubemapSetWrite.descriptorCount = 1;
	cubemapSetWrite.pImageInfo = &cubemapImageInfo;

	// Update the descriptor sets with new buffer/binding info
	vkUpdateDescriptorSets(pDevice->m_vkLogicalDevice, 1, &cubemapSetWrite, 0, nullptr);

	return std::make_tuple(descPool, descSetLayout, descSet);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreatePrefilteredSpecPipeline(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, VkDescriptorSetLayout descSetLayout, VkRenderPass renderPass)
{
	m_pGraphicsPipelinePrefilterSpec = new VulkanGraphicsPipeline(PipelineType::PREFILTER_SPEC, pSwapchain);

	// Descriptor Set layouts!
	std::vector<VkDescriptorSetLayout> setLayouts = { descSetLayout };

	// Push constants!
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PrefilterShaderPushData);

	std::vector<VkPushConstantRange> pushConstantRanges = { pushConstantRange };

	m_pGraphicsPipelinePrefilterSpec->CreatePipelineLayout(pDevice, setLayouts, pushConstantRanges);
	m_pGraphicsPipelinePrefilterSpec->CreateGraphicsPipeline(pDevice, pSwapchain, renderPass, 0, 1);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::RenderPrefilteredSpec(VulkanDevice* pDevice, VkRenderPass renderPass, VkFramebuffer framebuffer, VkImage prefilterImage, VkImage offscreenImage, VkDescriptorSet prefilterDescSet, uint32_t dimension, uint32_t nMipmaps)
{
	std::array<VkClearValue, 1> arrClearValues;
	arrClearValues[0].color = { {0.8f, 0.8f, 0.8f, 0.0f} };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = framebuffer;
	renderPassBeginInfo.renderArea.extent.width = dimension;
	renderPassBeginInfo.renderArea.extent.height = dimension;
	renderPassBeginInfo.clearValueCount = arrClearValues.size();
	renderPassBeginInfo.pClearValues = arrClearValues.data();

	glm::mat4 captureViews[] =
	{
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f,  1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f,  1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f,  1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f,  1.0f,  0.0f))
	};

	VkViewport vp = {};
	vp.width = (float)dimension;
	vp.height = (float)dimension;
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;

	VkRect2D scissorRect = {};
	scissorRect.extent.width = dimension;
	scissorRect.extent.height = dimension;
	scissorRect.offset.x = 0.0f;
	scissorRect.offset.y = 0.0f;

	PrefilterShaderPushData prefilterShaderData;

	// Start recording commands to command buffer!
	VkCommandBuffer cmdBuffer = pDevice->BeginCommandBuffer();

	vkCmdSetViewport(cmdBuffer, 0, 1, &vp);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissorRect);

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = nMipmaps;
	subresourceRange.layerCount = 6;

	// Change image layout for all cubemap faces to transfer destination
	Helper::Vulkan::TransitionImageLayoutCUBE(pDevice, cmdBuffer, prefilterImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

	for (uint32_t m = 0; m < nMipmaps; m++) 
	{
		prefilterShaderData.roughness = (float)m / (float)(nMipmaps - 1);

		for (uint32_t f = 0; f < 6; f++) 
		{
			vp.width = static_cast<float>(dimension * std::pow(0.5f, m));
			vp.height = static_cast<float>(dimension * std::pow(0.5f, m));
			vkCmdSetViewport(cmdBuffer, 0, 1, &vp);

			// Render scene from cube face's point of view
			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Update shader push constant block
			prefilterShaderData.mvp = glm::perspective((float)(M_PI / 2.0), 1.0f, 0.1f, 512.0f) * captureViews[f];

			vkCmdPushConstants(	cmdBuffer, 
								m_pGraphicsPipelinePrefilterSpec->m_vkPipelineLayout, 
								VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
								0, 
								sizeof(PrefilterShaderPushData), &prefilterShaderData);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipelinePrefilterSpec->m_vkGraphicsPipeline);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipelinePrefilterSpec->m_vkPipelineLayout, 0, 1, &prefilterDescSet, 0, NULL);

			m_pDummySkybox->Render(pDevice, cmdBuffer);

			vkCmdEndRenderPass(cmdBuffer);

			Helper::Vulkan::TransitionImageLayout(pDevice, cmdBuffer, offscreenImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			// Copy region for transfer from framebuffer to cube face
			VkImageCopy copyRegion = {};

			copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.srcSubresource.baseArrayLayer = 0;
			copyRegion.srcSubresource.mipLevel = 0;
			copyRegion.srcSubresource.layerCount = 1;
			copyRegion.srcOffset = { 0, 0, 0 };

			copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.dstSubresource.baseArrayLayer = f;
			copyRegion.dstSubresource.mipLevel = m;
			copyRegion.dstSubresource.layerCount = 1;
			copyRegion.dstOffset = { 0, 0, 0 };

			copyRegion.extent.width = static_cast<uint32_t>(vp.width);
			copyRegion.extent.height = static_cast<uint32_t>(vp.height);
			copyRegion.extent.depth = 1;

			vkCmdCopyImage(cmdBuffer, offscreenImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, prefilterImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			// Transform framebuffer color attachment back
			Helper::Vulkan::TransitionImageLayout(pDevice, cmdBuffer, offscreenImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		}
	}

	// Transform Irradiance map to Shader readable optimal format!
	Helper::Vulkan::TransitionImageLayoutCUBE(pDevice, cmdBuffer, prefilterImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

	pDevice->EndAndSubmitCommandBuffer(cmdBuffer);
}

//---------------------------------------------------------------------------------------------------------------------
std::tuple<VkDescriptorPool, VkDescriptorSetLayout, VkDescriptorSet> VulkanTextureCUBE::CreateBrdfLUTDescriptorSet(VulkanDevice* pDevice)
{
	VkResult result;
	VkDescriptorPool		descPool;
	VkDescriptorSetLayout	descSetLayout;
	VkDescriptorSet			descSet;

	// *** Create Descriptor pool
	std::array<VkDescriptorPoolSize, 1> arrDescriptorPoolSize = {};
	arrDescriptorPoolSize[0].descriptorCount = 1;
	arrDescriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.poolSizeCount = arrDescriptorPoolSize.size();
	poolCreateInfo.pPoolSizes = arrDescriptorPoolSize.data();
	poolCreateInfo.maxSets = 2;

	result = vkCreateDescriptorPool(pDevice->m_vkLogicalDevice, &poolCreateInfo, nullptr, &descPool);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Pool for Irradiance map!");
	}

	// *** Create Descriptor Set Layout
	std::array<VkDescriptorSetLayoutBinding, 1> arrDescriptorSetLayoutBindings = {};
	
	VkDescriptorSetLayoutCreateInfo descSetlayoutCreateInfo = {};
	descSetlayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetlayoutCreateInfo.bindingCount = arrDescriptorSetLayoutBindings.size();
	descSetlayoutCreateInfo.pBindings = arrDescriptorSetLayoutBindings.data();

	result = vkCreateDescriptorSetLayout(pDevice->m_vkLogicalDevice, &descSetlayoutCreateInfo, nullptr, &descSetLayout);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Set Layout for Irradiance map!");
	}

	// *** Create Descriptor Set
	VkDescriptorSetAllocateInfo	setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = descPool;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &descSetLayout;

	result = vkAllocateDescriptorSets(pDevice->m_vkLogicalDevice, &setAllocInfo, &descSet);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Set for Irradiance map!");
	}

	return std::make_tuple(descPool, descSetLayout, descSet);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateBrdfLUTPipeline(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, VkDescriptorSetLayout descSetLayout, VkRenderPass renderPass)
{
	m_pGraphicsPipelineBrdfLUT = new VulkanGraphicsPipeline(PipelineType::BRDF_LUT, pSwapchain);

	// Descriptor Set layouts!
	std::vector<VkDescriptorSetLayout> setLayouts = { descSetLayout };
	std::vector<VkPushConstantRange> pushConstantRanges = { };

	m_pGraphicsPipelineBrdfLUT->CreatePipelineLayout(pDevice, setLayouts, pushConstantRanges);
	m_pGraphicsPipelineBrdfLUT->CreateGraphicsPipeline(pDevice, pSwapchain, renderPass, 0, 1);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::RenderBrdfLUT(VulkanDevice* pDevice, VkRenderPass renderPass, VkFramebuffer framebuffer, VkDescriptorSet irradDescSet, uint32_t dimension)
{
	std::array<VkClearValue, 1> arrClearValues;
	arrClearValues[0].color = { {0.8f, 0.8f, 0.8f, 0.0f} };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = framebuffer;
	renderPassBeginInfo.renderArea.extent.width = dimension;
	renderPassBeginInfo.renderArea.extent.height = dimension;
	renderPassBeginInfo.clearValueCount = arrClearValues.size();
	renderPassBeginInfo.pClearValues = arrClearValues.data();

	VkViewport vp = {};
	vp.width = (float)dimension;
	vp.height = (float)dimension;
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;

	VkRect2D scissorRect = {};
	scissorRect.extent.width = dimension;
	scissorRect.extent.height = dimension;
	scissorRect.offset.x = 0.0f;
	scissorRect.offset.y = 0.0f;

	// Start recording commands to command buffer!
	VkCommandBuffer cmdBuffer = pDevice->BeginCommandBuffer();

	// Render scene from cube face's point of view
	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(cmdBuffer, 0, 1, &vp);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissorRect);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipelineBrdfLUT->m_vkGraphicsPipeline);
	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(cmdBuffer);

	pDevice->EndAndSubmitCommandBuffer(cmdBuffer);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::RenderHDRI2CUBE(VulkanDevice* pDevice, VkRenderPass renderPass, VkFramebuffer framebuffer, VkImage cubeImage, VkImage offscreenImage, VkDescriptorSet cubeDescSet, uint32_t dimension)
{
	std::array<VkClearValue, 1> arrClearValues;
	arrClearValues[0].color = { {0.8f, 0.8f, 0.8f, 0.0f} };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = framebuffer;
	renderPassBeginInfo.renderArea.extent.width = dimension;
	renderPassBeginInfo.renderArea.extent.height = dimension;
	renderPassBeginInfo.clearValueCount = arrClearValues.size();
	renderPassBeginInfo.pClearValues = arrClearValues.data();

	glm::mat4 captureViews[] =
	{
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, 1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, 1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  -1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, 1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, 1.0f,  0.0f))
	};

	VkViewport vp = {};
	vp.width = (float)dimension;
	vp.height = (float)dimension;
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;

	VkRect2D scissorRect = {};
	scissorRect.extent.width = dimension;
	scissorRect.extent.height = dimension;
	scissorRect.offset.x = 0.0f;
	scissorRect.offset.y = 0.0f;

	HDRIShaderPushData hdriShaderData;

	// Start recording commands to command buffer!
	VkCommandBuffer cmdBuffer = pDevice->BeginCommandBuffer();

	vkCmdSetViewport(cmdBuffer, 0, 1, &vp);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissorRect);

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 6;

	// Change image layout for all cubemap faces to transfer destination
	Helper::Vulkan::TransitionImageLayoutCUBE(pDevice, cmdBuffer, cubeImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

	// 6 faces of irradiance map!
	for (uint32_t f = 0; f < 6; f++)
	{
		// Render scene from cube face's point of view
		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		//projection[1][1] *= -1.0f;
		hdriShaderData.mvp =  projection * captureViews[f];

		vkCmdPushConstants(cmdBuffer,
			m_pGraphicsPipelineHDRI2Cube->m_vkPipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0, sizeof(HDRIShaderPushData),
			&hdriShaderData);

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipelineHDRI2Cube->m_vkGraphicsPipeline);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipelineHDRI2Cube->m_vkPipelineLayout, 0, 1, &cubeDescSet, 0, NULL);

		m_pDummySkybox->Render(pDevice, cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);

		Helper::Vulkan::TransitionImageLayout(pDevice, cmdBuffer, offscreenImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Copy region for transfer from framebuffer to cube face
		VkImageCopy copyRegion = {};

		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.mipLevel = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.srcOffset = { 0, 0, 0 };

		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.baseArrayLayer = f;
		copyRegion.dstSubresource.mipLevel = 0;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.dstOffset = { 0, 0, 0 };

		copyRegion.extent.width =  static_cast<uint32_t>(vp.width);
		copyRegion.extent.height = static_cast<uint32_t>(vp.height);
		copyRegion.extent.depth = 1;

		vkCmdCopyImage(cmdBuffer, offscreenImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cubeImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		// Transform framebuffer color attachment back
		Helper::Vulkan::TransitionImageLayout(pDevice, cmdBuffer, offscreenImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	// Transform Irradiance map to Shader readable optimal format!
	Helper::Vulkan::TransitionImageLayoutCUBE(pDevice, cmdBuffer, cubeImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

	pDevice->EndAndSubmitCommandBuffer(cmdBuffer);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateIrradianceCUBE(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, uint32_t dimension)
{
	m_pDummySkybox = new DummySkybox();
	m_pDummySkybox->Init(pDevice);

	const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

	//*** Create VkImage with 6 array Layers!
	m_vkImageIRRAD = Helper::Vulkan::CreateImageCUBE(pDevice,
													dimension,
													dimension,
													format, 
													1,
													VK_IMAGE_TILING_OPTIMAL,
													VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_vkImageMemoryIRRAD);

	// Create Image View!
	m_vkImageViewIRRAD = Helper::Vulkan::CreateImageViewCUBE(pDevice,
															m_vkImageIRRAD,
															format,
															1,
															VK_IMAGE_ASPECT_COLOR_BIT);

	// Create Sampler!
	m_vkSamplerIRRAD = CreateTextureSampler(pDevice, 1);

	// Create RenderPass
	VkRenderPass irradRenderPass = CreateOffscreenRenderPass(pDevice, format);

	// Create Off-screen framebuffer! 
	std::tuple<VkImage, VkImageView, VkDeviceMemory, VkFramebuffer> tupleOffscreen = CreateOffscreenFramebuffer(pDevice, irradRenderPass, format, dimension);

	// Create Descriptor Set layout & Descriptor Set!
	std::tuple<VkDescriptorPool, VkDescriptorSetLayout, VkDescriptorSet> tupleDescriptor = CreateIrradianceDescriptorSet(pDevice);

	// Create Graphics Pipeline!
	CreateIrradiancePipeline(pDevice, pSwapchain, std::get<1>(tupleDescriptor), irradRenderPass);

	// Render
	RenderIrrandianceCUBE(pDevice, irradRenderPass, std::get<3>(tupleOffscreen), m_vkImageIRRAD, std::get<0>(tupleOffscreen), std::get<2>(tupleDescriptor), dimension);

	// Cleanup!
	vkDestroyRenderPass(pDevice->m_vkLogicalDevice, irradRenderPass, nullptr);
	vkDestroyFramebuffer(pDevice->m_vkLogicalDevice, std::get<3>(tupleOffscreen), nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, std::get<2>(tupleOffscreen), nullptr);
	vkDestroyImageView(pDevice->m_vkLogicalDevice, std::get<1>(tupleOffscreen), nullptr);
	vkDestroyImage(pDevice->m_vkLogicalDevice, std::get<0>(tupleOffscreen), nullptr);
	vkDestroyDescriptorPool(pDevice->m_vkLogicalDevice, std::get<0>(tupleDescriptor), nullptr);
	vkDestroyDescriptorSetLayout(pDevice->m_vkLogicalDevice, std::get<1>(tupleDescriptor), nullptr);

	m_pGraphicsPipelineIrradiance->Cleanup(pDevice);
	m_pDummySkybox->Cleanup(pDevice);

	SAFE_DELETE(m_pGraphicsPipelineIrradiance);
	SAFE_DELETE(m_pDummySkybox);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreatePrefilteredSpecMap(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, uint32_t dimension)
{
	const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
	const uint32_t dim = dimension;
	const uint32_t numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;

	// Create 6 faced cubemap image
	m_vkImagePrefilterSpec = Helper::Vulkan::CreateImageCUBE(pDevice, 
															 dim, dim, format, numMips,
															 VK_IMAGE_TILING_OPTIMAL, 
															 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
															 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
															 &m_vkImageMemoryPrefilterSpec);

	// Create ImageView
	m_vkImageViewPrefilterSpec = Helper::Vulkan::CreateImageViewCUBE(pDevice, m_vkImagePrefilterSpec, format, numMips, VK_IMAGE_ASPECT_COLOR_BIT);

	// Create Sampler
	m_vkSamplerPrefilterSpec = CreateTextureSampler(pDevice, numMips);

	// Dummy skybox used for rendering HDRI into a cubemap!
	m_pDummySkybox = new DummySkybox();
	m_pDummySkybox->Init(pDevice);

	// Create offscreen Renderpass
	VkRenderPass prefilterRenderPass = CreateOffscreenRenderPass(pDevice, format);

	// Create offscreen framebuffer
	std::tuple<VkImage, VkImageView, VkDeviceMemory, VkFramebuffer> tupleOffscreen = CreateOffscreenFramebuffer(pDevice, prefilterRenderPass, format, dimension);

	// Create Descriptor Set layout & Descriptor Set!
	std::tuple<VkDescriptorPool, VkDescriptorSetLayout, VkDescriptorSet> tupleDescriptor = CreatePrefilteredSpecDescriptorSet(pDevice);

	// Create Graphics Pipeline!
	CreatePrefilteredSpecPipeline(pDevice, pSwapchain, std::get<1>(tupleDescriptor), prefilterRenderPass);

	// Render
	RenderPrefilteredSpec(pDevice, prefilterRenderPass, std::get<3>(tupleOffscreen), m_vkImagePrefilterSpec, std::get<0>(tupleOffscreen), std::get<2>(tupleDescriptor), dim, numMips);

	// Cleanup!
	vkDestroyRenderPass(pDevice->m_vkLogicalDevice, prefilterRenderPass, nullptr);
	vkDestroyFramebuffer(pDevice->m_vkLogicalDevice, std::get<3>(tupleOffscreen), nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, std::get<2>(tupleOffscreen), nullptr);
	vkDestroyImageView(pDevice->m_vkLogicalDevice, std::get<1>(tupleOffscreen), nullptr);
	vkDestroyImage(pDevice->m_vkLogicalDevice, std::get<0>(tupleOffscreen), nullptr);
	vkDestroyDescriptorPool(pDevice->m_vkLogicalDevice, std::get<0>(tupleDescriptor), nullptr);
	vkDestroyDescriptorSetLayout(pDevice->m_vkLogicalDevice, std::get<1>(tupleDescriptor), nullptr);

	m_pGraphicsPipelinePrefilterSpec->Cleanup(pDevice);
	m_pDummySkybox->Cleanup(pDevice);

	SAFE_DELETE(m_pGraphicsPipelinePrefilterSpec);
	SAFE_DELETE(m_pDummySkybox);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateBrdfLUTMap(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, uint32_t dimension)
{
	const VkFormat format = VK_FORMAT_R32G32_SFLOAT;
	const uint32_t dim = dimension;

	// Create LUT image
	m_vkImageBRDF = Helper::Vulkan::CreateImage(pDevice, dim, dim, format, 
												VK_IMAGE_TILING_OPTIMAL, 
												VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
												VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
												&m_vkImageMemoryBRDF);

	// Create LUT Image View
	m_vkImageViewBRDF = Helper::Vulkan::CreateImageView(pDevice, m_vkImageBRDF, format, VK_IMAGE_ASPECT_COLOR_BIT);

	// Create Sampler
	m_vkSamplerBRDF = CreateTextureSampler(pDevice, 1);

	// Create Renderpass
	VkRenderPass brdfLUTRenderPass;

	VkAttachmentDescription attachDesc = {};

	// Color attachment
	attachDesc.format = format;
	attachDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	attachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;		// Notice the main difference is here, instead of DST_OPTIMAL, it's SHADER_OPTIMAL

	VkAttachmentReference	colorAttachRef = {};
	colorAttachRef.attachment = 0;
	colorAttachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription	subpassDesc = {};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &colorAttachRef;

	// Subpass dependencies
	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Renderpass
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachDesc;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDesc;
	renderPassCreateInfo.dependencyCount = 2;
	renderPassCreateInfo.pDependencies = dependencies.data();

	VkResult result = vkCreateRenderPass(pDevice->m_vkLogicalDevice, &renderPassCreateInfo, nullptr, &brdfLUTRenderPass);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Renderpass for BRDF LUT creation!");
		return;
	}

	// Create framebuffer
	VkFramebuffer brdfLUTFramebuffer;

	VkFramebufferCreateInfo fbCreateInfo = {};
	fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbCreateInfo.renderPass = brdfLUTRenderPass;
	fbCreateInfo.attachmentCount = 1;
	fbCreateInfo.pAttachments = &m_vkImageViewBRDF;
	fbCreateInfo.width = dimension;
	fbCreateInfo.height = dimension;
	fbCreateInfo.layers = 1;

	result = vkCreateFramebuffer(pDevice->m_vkLogicalDevice, &fbCreateInfo, nullptr, &brdfLUTFramebuffer);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create offscreen framebuffer for BRDF LUT!");
	}

	// Create Descriptor Set layout & Descriptor Set!
	std::tuple<VkDescriptorPool, VkDescriptorSetLayout, VkDescriptorSet> tupleDescriptor = CreateBrdfLUTDescriptorSet(pDevice);

	// Create Graphics Pipeline!
	CreateBrdfLUTPipeline(pDevice, pSwapchain, std::get<1>(tupleDescriptor), brdfLUTRenderPass);

	// Render
	RenderBrdfLUT(pDevice, brdfLUTRenderPass, brdfLUTFramebuffer, std::get<2>(tupleDescriptor), dim);

	// Cleanup!
	vkDestroyRenderPass(pDevice->m_vkLogicalDevice, brdfLUTRenderPass, nullptr);
	vkDestroyFramebuffer(pDevice->m_vkLogicalDevice, brdfLUTFramebuffer, nullptr);
	//vkDestroyImageView(pDevice->m_vkLogicalDevice, std::get<1>(tupleOffscreen), nullptr);
	//vkDestroyImage(pDevice->m_vkLogicalDevice, std::get<0>(tupleOffscreen), nullptr);
	vkDestroyDescriptorPool(pDevice->m_vkLogicalDevice, std::get<0>(tupleDescriptor), nullptr);
	vkDestroyDescriptorSetLayout(pDevice->m_vkLogicalDevice, std::get<1>(tupleDescriptor), nullptr);

	m_pGraphicsPipelineBrdfLUT->Cleanup(pDevice);
	SAFE_DELETE(m_pGraphicsPipelineBrdfLUT);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::Cleanup(VulkanDevice* pDevice)
{
	// Cubemap
	vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vkImageViewCUBE, nullptr);
	vkDestroyImage(pDevice->m_vkLogicalDevice, m_vkImageCUBE, nullptr);
	vkDestroySampler(pDevice->m_vkLogicalDevice, m_vkSamplerCUBE, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkImageMemoryCUBE, nullptr);

	// Irradiance map
	vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vkImageViewIRRAD, nullptr);
	vkDestroyImage(pDevice->m_vkLogicalDevice, m_vkImageIRRAD, nullptr);
	vkDestroySampler(pDevice->m_vkLogicalDevice, m_vkSamplerIRRAD, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkImageMemoryIRRAD, nullptr);

	// Prefiltered Spec map
	vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vkImageViewPrefilterSpec, nullptr);
	vkDestroyImage(pDevice->m_vkLogicalDevice, m_vkImagePrefilterSpec, nullptr);
	vkDestroySampler(pDevice->m_vkLogicalDevice, m_vkSamplerPrefilterSpec, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkImageMemoryPrefilterSpec, nullptr);

	// BRDF LUT map
	vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vkImageViewBRDF, nullptr);
	vkDestroyImage(pDevice->m_vkLogicalDevice, m_vkImageBRDF, nullptr);
	vkDestroySampler(pDevice->m_vkLogicalDevice, m_vkSamplerBRDF, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkImageMemoryBRDF, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CleanupOnWindowResize(VulkanDevice* pDevice)
{
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateTextureImage(VulkanDevice* pDevice, std::string fileName)
{
	// *** Load pixel data
	int width = 0, height = 0, channels = 0;

	std::array<std::string, 6> arrFileNames;

	arrFileNames[0] = "Textures/Cubemaps/" + fileName + '/' + "posx.jpg";
	arrFileNames[1] = "Textures/Cubemaps/" + fileName + '/' + "negx.jpg";
	arrFileNames[2] = "Textures/Cubemaps/" + fileName + '/' + "posy.jpg";
	arrFileNames[3] = "Textures/Cubemaps/" + fileName + '/' + "negy.jpg";
	arrFileNames[4] = "Textures/Cubemaps/" + fileName + '/' + "posz.jpg";
	arrFileNames[5] = "Textures/Cubemaps/" + fileName + '/' + "negz.jpg";

	// Data for 6 faces of cubemap!
	std::array<unsigned char*, 6> arrImageData = {};

	for (int i = 0; i < arrImageData.size(); ++i)
	{
		arrImageData[i] = stbi_load(arrFileNames[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if (!arrImageData[i])
		{
			LOG_ERROR(("Failed to load a Texture file! (" + arrFileNames[i] + ")").c_str());
			return;
		}
	}

	// Calculate image size using given data for 6 faces!
	const VkDeviceSize imageSize = width * height * 4 * 6;
	const VkDeviceSize layerSize = imageSize / 6;

	//*** Create staging buffer to hold loaded data, ready to copy to device
	VkBuffer imageStagingBuffer;
	VkDeviceMemory imageStagingBufferMemory;

	// create staging buffer to hold the loaded data, ready to copy to device!
	pDevice->CreateBuffer(imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&imageStagingBuffer,
		&imageStagingBufferMemory);

	//*** copy image data to staging buffer
	void* data;
	vkMapMemory(pDevice->m_vkLogicalDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);

	VkDeviceSize currOffset = 0;
	for (int i = 0; i < arrImageData.size(); ++i)
	{
		memcpy(static_cast<unsigned char*>(data) + currOffset, arrImageData[i], layerSize);
		currOffset += layerSize;
	}

	vkUnmapMemory(pDevice->m_vkLogicalDevice, imageStagingBufferMemory);

	// Free original image data
	for (int i = 0; i < arrImageData.size(); ++i)
	{
		stbi_image_free(arrImageData[i]);
	}

	//*** Create VkImage with 6 array Layers!
	m_vkImageCUBE = Helper::Vulkan::CreateImageCUBE(	pDevice,
														width,
														height,
														VK_FORMAT_R8G8B8A8_UNORM,
														1,
														VK_IMAGE_TILING_OPTIMAL,
														VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_vkImageMemoryCUBE);


	//*** Transition image to be DST for copy operation
	Helper::Vulkan::TransitionImageLayoutCUBE(	pDevice,
												m_vkImageCUBE,
												VK_IMAGE_LAYOUT_UNDEFINED,
												VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// COPY DATA TO IMAGE
	Helper::Vulkan::CopyImageBufferCUBE(pDevice, imageStagingBuffer, m_vkImageCUBE, width, height);

	// Transition image to be shader readable for shader usage
	Helper::Vulkan::TransitionImageLayoutCUBE(	pDevice,
												m_vkImageCUBE,
												VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
												VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	//*** Destroy staging buffers
	vkDestroyBuffer(pDevice->m_vkLogicalDevice, imageStagingBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, imageStagingBufferMemory, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
VkSampler VulkanTextureCUBE::CreateTextureSampler(VulkanDevice* pDevice, uint32_t nMipmaps)
{
	VkSampler sampler;

	//-- Sampler creation Info
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;								// how to render when image is magnified on screen
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;								// how to render when image is minified on screen			
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;		// how to handle texture wrap in U (x) direction
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;		// how to handle texture wrap in V (y) direction
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;		// how to handle texture wrap in W (z) direction
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;			// border beyond texture (only works for border clamp)
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;						// whether values of texture coords between [0,1] i.e. normalized
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;				// Mipmap interpolation mode
	samplerCreateInfo.mipLodBias = 0.0f;										// Level of detail bias for mip level
	samplerCreateInfo.minLod = 0.0f;											// minimum level of detail to pick mip level
	samplerCreateInfo.maxLod = static_cast<float>(nMipmaps);					// maximum level of detail to pick mip level
	samplerCreateInfo.anisotropyEnable = VK_FALSE;								// Enable Anisotropy or not? Check physical device features to see if anisotropy is supported or not!
	samplerCreateInfo.maxAnisotropy = 16;										// Anisotropy sample level

	if (vkCreateSampler(pDevice->m_vkLogicalDevice, &samplerCreateInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Texture sampler!");
	}

	return sampler;
}



