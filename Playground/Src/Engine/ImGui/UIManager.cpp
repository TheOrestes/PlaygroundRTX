#include "PlaygroundPCH.h"
#include "UIManager.h"

#include "Engine/Renderer/VulkanDevice.h"
#include "Engine/Renderer/VulkanSwapChain.h"
#include "Engine/Renderer/VulkanFrameBuffer.h"
#include "Engine/RenderObjects/TriangleMesh.h"
#include "PlaygroundHeaders.h"
#include "Engine/Helpers/Log.h"
#include "Engine/Helpers/Utility.h"
#include "Engine/Scene.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

//---------------------------------------------------------------------------------------------------------------------
UIManager::UIManager()
{
	m_iPassID = 0;
}

//---------------------------------------------------------------------------------------------------------------------
UIManager::~UIManager()
{
	
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::Initialize(GLFWwindow* pWindow, VkInstance instance, VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	// Initialize Render Pass
	InitRenderPass(pDevice, pSwapchain);
	
	// Initialize Framebuffers
	InitFramebuffers(pDevice, pSwapchain);

	// Initialize Command Pool & Command Buffers
	InitCommandBuffers(pDevice, pSwapchain);
	
	// Initialize Descriptor Pool
	InitDescriptorPool(pDevice);

	// **** Initialize ImGui
	// Setup ImGui context!
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	io.Fonts->AddFontFromFileTTF("Assets/Fonts/SFMono-Regular.otf", 13.0f);

	// setup ImGui style
	ImGui::StyleColorsDark();
	
	ImGui_ImplGlfw_InitForVulkan(pWindow, true);
	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = instance;
	initInfo.PhysicalDevice = pDevice->m_vkPhysicalDevice;
	initInfo.Device = pDevice->m_vkLogicalDevice;
	initInfo.QueueFamily = pDevice->m_pQueueFamilyIndices->m_uiGraphicsFamily.value();
	initInfo.Queue = pDevice->m_vkQueueGraphics;
	initInfo.PipelineCache = VK_NULL_HANDLE;
	initInfo.DescriptorPool = m_vkDescriptorPool;
	initInfo.Allocator = nullptr;
	initInfo.MinImageCount = pSwapchain->m_uiMinImageCount;
	initInfo.ImageCount = pSwapchain->m_uiImageCount;
	initInfo.CheckVkResultFn = nullptr;
	ImGui_ImplVulkan_Init(&initInfo, m_vkRenderPass);

	// Upload fonts to the GPU 
	VkCommandBuffer commandBuffer = pDevice->BeginCommandBuffer();
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	pDevice->EndAndSubmitCommandBuffer(commandBuffer);
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::InitDescriptorPool(VulkanDevice* pDevice)
{
	// Create Descriptor Pool
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	if (vkCreateDescriptorPool(pDevice->m_vkLogicalDevice, &pool_info, nullptr, &m_vkDescriptorPool) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create ImGui Descriptor Pool!");
	}
	else
		LOG_DEBUG("ImGui Descriptor Pool created!");
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::InitCommandBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	// Create Command Pool
	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = pDevice->m_pQueueFamilyIndices->m_uiGraphicsFamily.value();
	commandPoolCreateInfo.pNext = nullptr;

	// Create a Graphics queue family Command Pool
	if (vkCreateCommandPool(pDevice->m_vkLogicalDevice, &commandPoolCreateInfo, nullptr, &m_vkCommandPool) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create GUI Command Pool!");
	}
	else
		LOG_DEBUG("Created GUI Command Pool!");

	// Create Command Buffers!
	m_vecCommandBuffers.resize(pSwapchain->m_vecSwapchainImages.size());

	for (uint32_t i = 0 ; i < m_vecCommandBuffers.size() ; i++)
	{
		VkCommandBuffer commandBuffer;
		
		VkCommandBufferAllocateInfo commandBufferAllocInfo{};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandPool = m_vkCommandPool;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;						// Buffer you submit directly to the queue. can't be called by other buffers!
																							// BUFFER_LEVEL_SECONDARY can't be called directly but can be called from other buffers via "vkCmdExecuteCommands"
		commandBufferAllocInfo.commandBufferCount = 1;
		commandBufferAllocInfo.pNext = nullptr;

		// Allocate command buffers & place handles in array of buffers!
		if (vkAllocateCommandBuffers(pDevice->m_vkLogicalDevice, &commandBufferAllocInfo, &m_vecCommandBuffers[i]) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create GUI Command buffer!");
		}
		else
			LOG_INFO("Created GUI Command buffers!");
	}
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::InitFramebuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	// resize framebuffer count to equal swap chain image views count
	m_vecFramebuffers.resize(pSwapchain->m_vecSwapchainImages.size());

	// create framebuffer for each swap chain image view
	for (uint32_t i = 0; i < pSwapchain->m_vecSwapchainImages.size(); ++i)
	{
		VkImageView attachments[] = { pSwapchain->m_vecSwapchainImageViews[i] };
		
		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = m_vkRenderPass;									// Render pass layout the framebuffer will be used with					 
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = attachments;									// List of attachments
		framebufferCreateInfo.width = pSwapchain->m_vkSwapchainExtent.width;				// framebuffer width
		framebufferCreateInfo.height = pSwapchain->m_vkSwapchainExtent.height;				// framebuffer height
		framebufferCreateInfo.layers = 1;													// framebuffer layers
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.pNext = nullptr;

		if (vkCreateFramebuffer(pDevice->m_vkLogicalDevice, &framebufferCreateInfo, nullptr, &m_vecFramebuffers[i]) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create GUI Framebuffer");
		}
		else
			LOG_INFO("GUI Framebuffer created!");
	}
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::InitRenderPass(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	// **** First create attachment description
	VkAttachmentDescription attachment = {};
	attachment.format = pSwapchain->m_vkSwapchainImageFormat;					// format to use for attachment : same as framebuffer
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;									// number of samples for multi-sampling
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;								// we want to draw GUI over main rendering, hence we don't clear it. 
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;							// describes what to do with attachment after rendering
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// describes what to do with stencil before rendering
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// describes what to do with stencil after rendering
	attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;		// image data layout before render pass starts
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;					// image data layout after render pass

	// Actual reference to the attachment
	VkAttachmentReference attachRef = {};
	attachRef.attachment = 0;
	attachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// **** Create subpass for our Render pass
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachRef;

	// Create subpass dependency 
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// **** Create Render Pass! 
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(pDevice->m_vkLogicalDevice, &renderPassCreateInfo, nullptr, &m_vkRenderPass) != VK_SUCCESS)
	{
		LOG_ERROR("ImGui Render pass creation failed!");
	}
	else
		LOG_DEBUG("ImGui Render pass created!");
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::BeginRender()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::EndRender(VulkanSwapChain* pSwapchain, uint32_t imageIndex)
{
	ImGui::Render();

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(m_vecCommandBuffers[imageIndex], &commandBufferBeginInfo);

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_vkRenderPass;
	renderPassBeginInfo.framebuffer = m_vecFramebuffers[imageIndex];
	renderPassBeginInfo.renderArea.extent.width = pSwapchain->m_vkSwapchainExtent.width;
	renderPassBeginInfo.renderArea.extent.height = pSwapchain->m_vkSwapchainExtent.height;
	renderPassBeginInfo.clearValueCount = 1;

	VkClearValue clearValue;
	clearValue.color = { 0.2f, 0.2f, 0.2f, 1.0f };
	clearValue.depthStencil = { 1.0f, 1 };
	
	renderPassBeginInfo.pClearValues = &clearValue;
	vkCmdBeginRenderPass(m_vecCommandBuffers[imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_vecCommandBuffers[imageIndex]);
	vkCmdEndRenderPass(m_vecCommandBuffers[imageIndex]);
	vkEndCommandBuffer(m_vecCommandBuffers[imageIndex]);
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::RenderSceneUI(Scene* pScene)
{
	ImGui::Begin("Scene Properties");

	if(ImGui::CollapsingHeader("Scene Objects"))
	{
		// All 3D Models!
		for (uint32_t i = 0 ; i < pScene->m_vecSceneObjects.size() ; ++i)
		{
			ImGui::PushID(i);
			ImGui::AlignTextToFramePadding();

			TriangleMesh* pMesh = static_cast<TriangleMesh*>(pScene->m_vecSceneObjects.at(i));

			// Transforms
			std::string nodeName = "Model" + std::to_string(i);
			if (ImGui::TreeNode(nodeName.c_str()))
			{
				//**** TRANSORM UI
				if(ImGui::TreeNode("Transform"))
				{
					//-- Position
					//glm::vec3 pos = pMesh->m_pMeshInstanceData->position;
					//
					//if (ImGui::SliderFloat3("Position", glm::value_ptr(pos), -100, 100))
					//	pMesh->m_pMeshInstanceData->position = pos;
					//
					////-- Rotation Axis
					//	glm::vec3 rot = pMesh->m_pMeshInstanceData->rotationAxis;
					//if (ImGui::InputFloat3("Rotation Axis", glm::value_ptr(rot)))
					//	pMesh->m_pMeshInstanceData->rotationAxis = rot;
					//
					////-- Rotation Angle
					//float rotAngle = pMesh->m_pMeshInstanceData->angle;
					//if (ImGui::SliderAngle("Rotation Angle", &rotAngle))
					//	pMesh->m_pMeshInstanceData->angle = rotAngle;
					//
					////-- Scale
					//glm::vec3 scale = pMesh->m_pMeshInstanceData->scale;
					//if (ImGui::InputFloat3("Scale", glm::value_ptr(scale)))
					//	pMesh->m_pMeshInstanceData->scale = scale;

					ImGui::TreePop();
				}

				//**** MATERIAL UI
				if (ImGui::TreeNode("Material"))
				{
					//ShaderData* data = &(pModel->m_pShaderUniforms->shaderData);
					//
					////-- Albedo Color
					//float albedo[4] = { data->albedoColor.r, data->albedoColor.g, data->albedoColor.b, data->albedoColor.a };
					//if(ImGui::ColorEdit4("Albedo", albedo))
					//{
					//	data->albedoColor = glm::vec4(albedo[0], albedo[1], albedo[2], albedo[3]);
					//}
					//
					////-- Emission Color
					//float emission[4] = { data->emissiveColor.r, data->emissiveColor.g, data->emissiveColor.b, data->emissiveColor.a };
					//if (ImGui::ColorEdit4("Emission", emission))
					//{
					//	data->emissiveColor = glm::vec4(emission[0], emission[1], emission[2], emission[3]);
					//}
					//
					////-- Roughness
					//float roughness = data->roughness;
					//if(ImGui::SliderFloat("Roughness", &roughness, 0.001f, 1.0f))
					//{
					//	data->roughness = roughness;
					//}
					//
					////-- Metalness
					//float metalness = data->metalness;
					//if (ImGui::SliderFloat("Metalness", &metalness, 0.001f, 1.0f))
					//{
					//	data->metalness = metalness;
					//}
					//
					////-- Occlusion
					//float occlusion = data->ao;
					//if (ImGui::SliderFloat("AmbOcclusion", &occlusion, 0.001f, 1.0f))
					//{
					//	data->ao = occlusion;
					//}
					
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
			
			ImGui::PopID();
		}
	}

	//**** Light UI
	if (ImGui::CollapsingHeader("Sun Light Properties"))
	{
		//glm::vec3 angleXYZ = pScene->GetLightEulerAngles();
		//if (ImGui::DragFloat3("Rotation", glm::value_ptr(angleXYZ), 1, -180, 180, "%0.1f"))
		//{
		//	pScene->SetLightDirection(angleXYZ);
		//}

		float intensity = pScene->m_LightIntensity;
		if (ImGui::SliderFloat("Intensity", &intensity, 0.01f, 10.0f))
		{
			pScene->m_LightIntensity = intensity;
		}
	}

	//**** Debugging UI
	if (ImGui::CollapsingHeader("Debug G-Buffer"))	
	{
		const char* arr[] = { "FINAL", "ALBEDO", "DEPTH", "POSITION", "NORMAL", "METALNESS", "ROUGHNESS", "AO", "EMISSION", "BACKGROUND", "OBJECTID" };
		ImGui::Combo("Channel", &m_iPassID, arr, IM_ARRAYSIZE(arr));
	}
	
	ImGui::End();
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::RenderDebugStats()
{
	ImGui::Begin("Debug Statistics");
	ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
	ImGui::Text("ms Per Frame: %f", 1000.0f / ImGui::GetIO().Framerate);
	ImGui::End();
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::HandleWindowResize(GLFWwindow* pWindow, VkInstance instance, VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	int width, height;
	glfwGetFramebufferSize(pWindow, &width, &height);
	if (width > 0 && height > 0)
	{
		ImGui_ImplVulkan_SetMinImageCount(static_cast<uint32_t>(pSwapchain->m_vecSwapchainImages.size()));
	}

	// Re-Initialize Render Pass
	InitRenderPass(pDevice, pSwapchain);

	// Re-Initialize Framebuffers
	InitFramebuffers(pDevice, pSwapchain);

	// Re-Initialize Command Pool & Command Buffers
	InitCommandBuffers(pDevice, pSwapchain);
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::Cleanup(VulkanDevice* pDevice)
{
	// Cleanup Framebuffers
	for (int i = 0; i < m_vecFramebuffers.size(); ++i)
	{
		vkDestroyFramebuffer(pDevice->m_vkLogicalDevice, m_vecFramebuffers[i], nullptr);
	}
	
	// Cleanup Render Pass
	vkDestroyRenderPass(pDevice->m_vkLogicalDevice, m_vkRenderPass, nullptr);

	// Cleanup Command Buffers
	for (int j = 0; j < m_vecCommandBuffers.size(); ++j)
	{
		vkFreeCommandBuffers(pDevice->m_vkLogicalDevice, m_vkCommandPool, 1, &m_vecCommandBuffers[j]);
	}

	// Cleanup Command Pool
	vkDestroyCommandPool(pDevice->m_vkLogicalDevice, m_vkCommandPool, nullptr);

	// Cleanup ImGui stuff...
	vkDeviceWaitIdle(pDevice->m_vkLogicalDevice);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Cleanup Descriptor Pool
	vkDestroyDescriptorPool(pDevice->m_vkLogicalDevice, m_vkDescriptorPool, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::CleanupOnWindowResize(VulkanDevice* pDevice)
{
	// Cleanup Framebuffers
	for (int i = 0; i < m_vecFramebuffers.size(); ++i)
	{
		vkDestroyFramebuffer(pDevice->m_vkLogicalDevice, m_vecFramebuffers[i], nullptr);
	}

	// Cleanup Render Pass
	vkDestroyRenderPass(pDevice->m_vkLogicalDevice, m_vkRenderPass, nullptr);

	// Cleanup Command Buffers
	for (int j = 0; j < m_vecCommandBuffers.size(); ++j)
	{
		vkFreeCommandBuffers(pDevice->m_vkLogicalDevice, m_vkCommandPool, 1, &m_vecCommandBuffers[j]);
	}

	// Cleanup Command Pool
	vkDestroyCommandPool(pDevice->m_vkLogicalDevice, m_vkCommandPool, nullptr);
}
