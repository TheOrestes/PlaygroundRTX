
#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"

#include "VulkanRenderer.h"
#include "VulkanSwapChain.h"

#include "Engine/ImGui/UIManager.h"

//---------------------------------------------------------------------------------------------------------------------
VulkanRenderer::VulkanRenderer()
{
	m_pWindow							= nullptr;

	m_pDevice							= nullptr;
	m_pSwapChain						= nullptr;
	
	m_uiCurrentFrame					= 0;
	m_uiSwapchainImageIndex				= 0;
	m_bFramebufferResized				= false;

	m_vkInstance						= VK_NULL_HANDLE;
	m_vkDebugMessenger					= VK_NULL_HANDLE;
	m_vkSurface							= VK_NULL_HANDLE;

	m_vecSemaphoreImageAvailable.clear();
	m_vecSemaphoreRenderFinished.clear();
	m_vecFencesRender.clear();
}

//---------------------------------------------------------------------------------------------------------------------
VulkanRenderer::~VulkanRenderer()
{
	m_vecSemaphoreImageAvailable.clear();
	m_vecSemaphoreRenderFinished.clear();
	m_vecFencesRender.clear();

	SAFE_DELETE(m_pSwapChain);
	SAFE_DELETE(m_pDevice);
}

//---------------------------------------------------------------------------------------------------------------------
int VulkanRenderer::Initialize(GLFWwindow* pWindow)
{
	m_pWindow = pWindow;

	// register a callback to detect window resize
	glfwSetFramebufferSizeCallback(m_pWindow, FramebufferResizeCallback);

	// Run shader compiler before everything else
	RunShaderCompiler("Assets/Shaders");

	try
	{
		LOG_DEBUG("sizeof glm::vec3 = {0}", sizeof(glm::vec3));
		LOG_DEBUG("sizeof glm::vec4 = {0}", sizeof(glm::vec4));
		LOG_DEBUG("sizeof glm::mat3 = {0}", sizeof(glm::mat3));
		LOG_DEBUG("sizeof glm::mat4 = {0}", sizeof(glm::mat4));
		LOG_DEBUG("sizeof uint32_t = {0}", sizeof(uint32_t));
		LOG_DEBUG("sizeof float = {0}", sizeof(float));
		LOG_DEBUG("sizeof bool = {0}", sizeof(bool));
		
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();

		m_pDevice = new VulkanDevice(m_vkInstance, m_vkSurface);

		m_pDevice->PickPhysicalDevice();
		m_pDevice->CreateLogicalDevice();

		m_pSwapChain = new VulkanSwapChain();
		m_pSwapChain->CreateSwapChain(m_pDevice, m_vkSurface, m_pWindow);

		m_pDevice->CreateGraphicsCommandPool();
		m_pDevice->CreateGraphicsCommandBuffers(m_pSwapChain->m_vecSwapchainImages.size());

		CreateSyncObjects();
	}
	catch (const std::runtime_error& e)
	{
		std::cout << "\nERROR: " << e.what();
		return EXIT_FAILURE;
	}

	return 0;
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::Update(float dt)
{

}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::RunShaderCompiler(const std::string& directoryPath)
{
	std::string shaderCompiler = "C:/VulkanSDK/1.2.182.0/Bin/glslc.exe";
	for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
	{
		if (entry.is_regular_file() && 
		   (entry.path().extension().string() == ".vert" || entry.path().extension().string() == ".frag") ||
		    entry.path().extension().string() == ".rchit" || entry.path().extension().string() == ".rmiss"  || entry.path().extension().string() == ".rgen")
		{
			std::string cmd = shaderCompiler + " --target-env=vulkan1.2" + " -c" + " " + entry.path().string() + " -o " + entry.path().string() + ".spv";
			LOG_DEBUG("Compiling shader " + entry.path().filename().string());
			std::system(cmd.c_str());
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::CreateInstance()
{
	// Check if requested validation layer is available with current vulkan instance!
	if (Vulkan::g_bEnableValidationLayer && !CheckValidationLayerSupport())
	{
		LOG_ERROR("Requested Validation Layer not supported!");
	}

	// Provide information about our application, this struct is optional!
	VkApplicationInfo appInfo = {};
	appInfo.apiVersion = VK_API_VERSION_1_2;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pApplicationName = "Hello Vulkan Triangle";
	appInfo.pEngineName = "Vulkan Engine";
	appInfo.pNext = nullptr;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

	// Non-optional struct. This tells vulkan driver which global extensions
	// and validation layers we want to use. 
	VkInstanceCreateInfo createInfo{};

	uint32_t glfwExtensionCount = 0;

	//This function returns an array of names of Vulkan instance extensions required
	// by GLFW for creating Vulkan surfaces for GLFW windows.
	const char** extensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// const char** to std::vector<const char*> conversion 
	// Add debug messenger extension conditionally!
	std::vector<const char*> vecExtensions(extensions, extensions + glfwExtensionCount);
	if (Vulkan::g_bEnableValidationLayer)
	{
		vecExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	// Check if all required Instance Extensions are supported!
	if (!CheckInstanceExtensionSupport(vecExtensions))
	{
		LOG_ERROR("VkInstance does not support required extensions!");
	}
	else
	{
		LOG_INFO("VkInstance supports required extensions!");
	}

	// Create additional debug messenger just for vkCreateInstance & vkDestroyInstance!
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (Vulkan::g_bEnableValidationLayer)
	{
		createInfo.enabledLayerCount = Vulkan::g_strValidationLayers.size();
		createInfo.ppEnabledLayerNames = Vulkan::g_strValidationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	createInfo.enabledExtensionCount = vecExtensions.size();
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.ppEnabledExtensionNames = vecExtensions.data();
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

	if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS)
		LOG_ERROR("Failed to create Vulkan Instance!");

	LOG_INFO("Vulkan Instance Created!");
}

//---------------------------------------------------------------------------------------------------------------------
bool VulkanRenderer::CheckInstanceExtensionSupport(const std::vector<const char*>& instanceExtensions)
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> vecExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, vecExtensions.data());

#if defined _DEBUG
	// Enumerate all the extensions supported by the vulkan instance.
	// Ideally, this list should contain extensions requested by GLFW and
	// few addional ones!
	LOG_DEBUG("--------- Available Vulkan Extensions ---------");
	for (int i = 0; i < extensionCount; ++i)
	{
		LOG_DEBUG(vecExtensions[i].extensionName);
	}
	LOG_DEBUG("-----------------------------------------------");
#endif

	// Check if given extensions are in the list of available extensions
	for (uint32_t i = 0; i < extensionCount; i++)
	{
		bool hasExtension = false;

		for (uint32_t j = 0; j < instanceExtensions.size(); j++)
		{
			if (strcmp(vecExtensions[i].extensionName, instanceExtensions[j]))
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
			return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
bool VulkanRenderer::CheckValidationLayerSupport()
{
	// Enumerate available validation layers for the vulkan instance!
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> vecAvailableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, vecAvailableLayers.data());

	// Try to see if requested enumeration layers [in Helper.h] is present in available 
	// validation layers. 
	bool layerFound = false;
	for (int i = 0; i < Vulkan::g_strValidationLayers.size(); ++i)
	{
		for (int j = 0; j < layerCount; ++j)
		{
			if (strcmp(Vulkan::g_strValidationLayers[i], vecAvailableLayers[j].layerName) == 0)
			{
				layerFound = true;

				std::string msg = std::string(Vulkan::g_strValidationLayers[i]) + " validation layer found!";
				LOG_DEBUG(msg.c_str());
			}
		}
	}

	return layerFound;
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.flags = 0;
	createInfo.messageSeverity = //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |	
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								 //VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	createInfo.messageType =	 VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr;
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::CreateSurface()
{
	if (glfwCreateWindowSurface(m_vkInstance, m_pWindow, nullptr, &m_vkSurface) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Windows surface!");
		return;
	}

	LOG_INFO("Windows surface created");
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::HandleWindowResize()
{
	// Handle window minimization => framebuffer size = 0
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_pWindow, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_pWindow, &width, &height);
		glfwWaitEvents();
	}

	// we shouldn't touch resources that are still in use!
	vkDeviceWaitIdle(m_pDevice->m_vkLogicalDevice);

	// perform cleanup on old versions
	CleanupOnWindowResize();

	// Recreate...!
	LOG_DEBUG("Recreating SwapChain Start");

	m_pSwapChain->CreateSwapChain(m_pDevice, m_vkSurface, m_pWindow);
	m_pDevice->CreateGraphicsCommandBuffers(m_pSwapChain->m_vecSwapchainImages.size());
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::RecordCommands(uint32_t currentImage)
{
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::CreateSyncObjects()
{
	m_vecSemaphoreImageAvailable.resize(App::MAX_FRAME_DRAWS);
	m_vecSemaphoreRenderFinished.resize(App::MAX_FRAME_DRAWS);
	m_vecFencesRender.resize(App::MAX_FRAME_DRAWS);

	// Semaphore create information
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.flags = 0;
	semaphoreCreateInfo.pNext = nullptr;

	// Fence create information
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	fenceCreateInfo.pNext = nullptr;

	for (uint32_t i = 0; i < App::MAX_FRAME_DRAWS; ++i)
	{
		// Semaphore Image Available
		VKRESULT_CHECK_INFO(vkCreateSemaphore(m_pDevice->m_vkLogicalDevice, &semaphoreCreateInfo, nullptr, &m_vecSemaphoreImageAvailable[i]),
							"Failed to create ImageAvailable Semaphore",
							"Created ImageAvailable Semaphore");

		// Semaphore Render Finished
		VKRESULT_CHECK_INFO(vkCreateSemaphore(m_pDevice->m_vkLogicalDevice, &semaphoreCreateInfo, nullptr, &m_vecSemaphoreRenderFinished[i]),
							"Failed to create RenderFinished Semaphore",
							"Created RenderFinished Semaphore");

		// Fence
		VKRESULT_CHECK_INFO(vkCreateFence(m_pDevice->m_vkLogicalDevice, &fenceCreateInfo, nullptr, &m_vecFencesRender[i]),
							"Failed to create Fence",
							"Successfully created Fence");
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::SetupDebugMessenger()
{
	if (!Vulkan::g_bEnableValidationLayer)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_vkDebugMessenger) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to setup debug messenger!");
		return;
	}
}

//---------------------------------------------------------------------------------------------------------------------
// 1. Acquire the next available image from the swap chain to draw. Set something to signal when we're finished with the image (semaphore)
// 2. Submit the command buffer to queue for execution, making sure it waits for the image to be signaled as available before drawing & signals when it has finished rendering!
// 3. Present image to the screen when it has signaled finished rendering!
void VulkanRenderer::Render()
{
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::CleanupOnWindowResize()
{
	m_pSwapChain->CleanupOnWindowResize(m_pDevice);
	m_pDevice->CleanupOnWindowResize();
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::Cleanup()
{
	// Destroy semaphores
	for (uint32_t i = 0; i < App::MAX_FRAME_DRAWS; ++i)
	{
		vkDestroySemaphore(m_pDevice->m_vkLogicalDevice, m_vecSemaphoreImageAvailable[i], nullptr);
		vkDestroySemaphore(m_pDevice->m_vkLogicalDevice, m_vecSemaphoreRenderFinished[i], nullptr);
		vkDestroyFence(m_pDevice->m_vkLogicalDevice, m_vecFencesRender[i], nullptr);
	}

	m_pSwapChain->Cleanup(m_pDevice);
	m_pDevice->Cleanup();

	if (Vulkan::g_bEnableValidationLayer)
	{
		DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
	vkDestroyInstance(m_vkInstance, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::BeginFrame()
{
	//--- 1. Acquire next image from the swap chain!
	// Wait for given fence to signal (open) from last draw call before continuing...
	vkWaitForFences(m_pDevice->m_vkLogicalDevice, 1, &m_vecFencesRender[m_uiCurrentFrame], VK_TRUE, UINT64_MAX);

	// Manually reset (close) fence!
	vkResetFences(m_pDevice->m_vkLogicalDevice, 1, &m_vecFencesRender[m_uiCurrentFrame]);

	// Get index of next image to be drawn to & signal semaphore when ready to be drawn to
	VkResult result = vkAcquireNextImageKHR(m_pDevice->m_vkLogicalDevice, m_pSwapChain->m_vkSwapchain, UINT64_MAX, m_vecSemaphoreImageAvailable[m_uiCurrentFrame], VK_NULL_HANDLE, &m_uiSwapchainImageIndex);

	// During any event such as window size change etc. we need to check if swap chain recreation is necessary
	// Vulkan tells us that swap chain in no longer adequate during presentation
	// VK_ERROR_OUT_OF_DATE_KHR = swap chain has become incompatible with the surface & can no longer be used for rendering. (window resize)
	// VK_SUBOPTIMAL_KHR = swap chain can be still used to present to the surface but the surface properties are no longer matching!

	// if swap chain is out of date while acquiring the image, then its not possible to present it!
	// We should recreate the swap chain & try again in the next draw call...
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		HandleWindowResize();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		LOG_ERROR("Failed to acquire swap chain image!");
		return;
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::SubmitAndPresentFrame()
{ 
	//--- 2. Execute the command buffer
	// Queue submission & synchronization is configured through VkSubmitInfo.

	VkSemaphore waitSemaphores[] = { m_vecSemaphoreImageAvailable[m_uiCurrentFrame] };
	VkSemaphore signalSemaphores[] = { m_vecSemaphoreRenderFinished[m_uiCurrentFrame] };

	std::array<VkCommandBuffer, 2> commandBuffers =
	{
		m_pDevice->m_vecCommandBufferGraphics[m_uiSwapchainImageIndex],
		UIManager::getInstance().m_vecCommandBuffers[m_uiSwapchainImageIndex]
	};

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;														// Number of semaphores to wait on
	submitInfo.pWaitSemaphores = waitSemaphores;											// List of semaphores to wait on

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = waitStages;												// stages to check semaphores at

	submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());			// number of command buffers to submit
	submitInfo.pCommandBuffers = commandBuffers.data();										// command buffers to submit
	submitInfo.signalSemaphoreCount = 1;													// number of semaphores to signal
	submitInfo.pSignalSemaphores = signalSemaphores;										// semaphores to signal when command buffer finishes
	submitInfo.pNext = nullptr;

	if (vkQueueSubmit(m_pDevice->m_vkQueueGraphics, 1, &submitInfo, m_vecFencesRender[m_uiCurrentFrame]) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to submit draw command buffer!");
	}

	//--- 3. Submit result back to the swap chain.
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;														// Number of semaphores to wait on
	presentInfo.pWaitSemaphores = signalSemaphores;											// semaphores to wait on
	presentInfo.swapchainCount = 1;															// number of swap chains to present to
	presentInfo.pSwapchains = &(m_pSwapChain->m_vkSwapchain);								// swapchain to present image to
	presentInfo.pImageIndices = &m_uiSwapchainImageIndex;									// index of images in swap chains to present
	presentInfo.pNext = nullptr;
	presentInfo.pResults = nullptr;

	// check if swap chain is optimal or not! else recreate & try in next draw call!
	VkResult result = vkQueuePresentKHR(m_pDevice->m_vkQueuePresent, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_bFramebufferResized)
	{
		m_bFramebufferResized = false;
		HandleWindowResize();
	}
	else if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to present swap chain image!");
	}

	// Get next frame 
	m_uiCurrentFrame = (m_uiCurrentFrame + 1) % App::MAX_FRAME_DRAWS;
}
