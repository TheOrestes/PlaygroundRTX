#include "PlaygroundPCH.h"
#include "VulkanDevice.h"

#include "PlaygroundHeaders.h"
#include "Engine/Helpers/Utility.h"

//---------------------------------------------------------------------------------------------------------------------
VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface)
{
	// Application Instance
	m_vkInstance = instance;

	// Window Surface
	m_vkSurface = surface;

	// Physical Device
	m_vkPhysicalDevice = nullptr;
	m_vkDeviceProperties = {};
	m_vkDeviceFeaturesAvailable = {};
	m_vkDeviceFeaturesEnabled = {};
	m_vkDeviceMemoryProps = {};
	m_vkQueueGraphics = nullptr;
	m_vkQueuePresent = nullptr;

	m_vkLogicalDevice = nullptr;
	m_vkCommandPoolGraphics = nullptr;
	m_pQueueFamilyIndices = nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
VulkanDevice::~VulkanDevice()
{
	SAFE_DELETE(m_pQueueFamilyIndices);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanDevice::PickPhysicalDevice()
{
	// List out all the physical devices
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);

	// if there are NO devices with Vulkan support, no point going forward!
	if (deviceCount == 0)
	{
		LOG_CRITICAL("Failed to find GPU with Vulkan support!");
		return;
	}

	// allocate an array to hold all physical devices handles...
	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, physicalDevices.data());

	// List out all the Physical devices & get their properties!
	for (int i = 0; i < deviceCount; ++i)
	{
		vkGetPhysicalDeviceProperties(physicalDevices[i], &m_vkDeviceProperties);
		LOG_DEBUG("{0} Detected!", m_vkDeviceProperties.deviceName);
	}

	// check if physical device has the Queue family required for needed operations
	for (int i = 0; i < deviceCount; ++i)
	{
		// Find Queue families, look for needed queue families...
		FindQueueFamilies(physicalDevices[i]);
		bool bExtensionsSupported = CheckDeviceExtensionSupport(physicalDevices[i]);

		if (m_pQueueFamilyIndices->isComplete() && bExtensionsSupported)
		{
			// Prefer Discrete GPU over Integrated one! 
			if (m_vkDeviceProperties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				m_vkPhysicalDevice = physicalDevices[i];

				LOG_DEBUG("{0} Selected!!", m_vkDeviceProperties.deviceName);
				LOG_INFO("---------- Device Limits ----------");
				LOG_INFO("Max Color Attachments: {0}", m_vkDeviceProperties.limits.maxColorAttachments);
				LOG_INFO("Max Descriptor Set Samplers: {0}", m_vkDeviceProperties.limits.maxDescriptorSetSamplers);
				LOG_INFO("Max Descriptor Set Uniform Buffers: {0}", m_vkDeviceProperties.limits.maxDescriptorSetUniformBuffers);
				LOG_INFO("Max Framebuffer Height: {0}", m_vkDeviceProperties.limits.maxFramebufferHeight);
				LOG_INFO("Max Framebuffer Width: {0}", m_vkDeviceProperties.limits.maxFramebufferWidth);
				LOG_INFO("Max Push Constant Size: {0}", m_vkDeviceProperties.limits.maxPushConstantsSize);
				LOG_INFO("Max Uniform Buffer Range: {0}", m_vkDeviceProperties.limits.maxUniformBufferRange);
				LOG_INFO("Max Vertex Input Attributes: {0}", m_vkDeviceProperties.limits.maxVertexInputAttributes);

				break;
			}
			else
			{
				LOG_ERROR("{0} Rejected!!", m_vkDeviceProperties.deviceName);
			}
		}
	}

	// If we don't find suitable device, return!
	if (m_vkPhysicalDevice == VK_NULL_HANDLE)
	{
		LOG_ERROR("Failed to find suitable GPU!");
		return;
	}
}

//---------------------------------------------------------------------------------------------------------------------
bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	// Get count of total number of extensions
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	// gather their information
	m_vecSupportedExtensions.resize(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, m_vecSupportedExtensions.data());

	// Compare Required extensions with supported extensions...
	for (int i = 0; i < Vulkan::g_strDeviceExtensions.size(); ++i)
	{
		bool bExtensionFound = false;

		for (int j = 0; j < extensionCount; ++j)
		{
			// If device supported extensions matches the one we want, good news ... Enumarate them!
			if (strcmp(Vulkan::g_strDeviceExtensions[i], m_vecSupportedExtensions[j].extensionName) == 0)
			{
				bExtensionFound = true;

				std::string msg = std::string(Vulkan::g_strDeviceExtensions[i]) + " device extension found!";
				LOG_DEBUG(msg.c_str());

				break;
			}
		}

		// No matching extension found ... bail out!
		if (!bExtensionFound)
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanDevice::FindQueueFamilies(VkPhysicalDevice device)
{
	// retrieve list of queue families 
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	m_pQueueFamilyIndices = new QueueFamilyIndices();

	// VkQueueFamilyProperties contains details about the queue family. We need to find at least one
	// queue family that supports VK_QUEUE_GRAPHICS_BIT
	for (int i = 0; i < queueFamilyCount; ++i)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_pQueueFamilyIndices->m_uiGraphicsFamily = i;
		}

		// check if this queue family has capability of presenting to our window surface!
		VkBool32 bPresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_vkSurface, &bPresentSupport);

		// if yes, store presentation family queue index!
		if (bPresentSupport)
		{
			m_pQueueFamilyIndices->m_uiPresentFamily = i;
		}

		if (m_pQueueFamilyIndices->isComplete())
			break;
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanDevice::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	// std::set allows only One unique value for input values, no duplicate is allowed, so if both Graphics Queue family
	// and Presentation Queue family index is same then it will avoid the duplicates and assign only one queue index!
	std::set<uint32_t> uniqueQueueFamilies =
	{
		m_pQueueFamilyIndices->m_uiGraphicsFamily.value(),
		m_pQueueFamilyIndices->m_uiPresentFamily.value()
	};

	float queuePriority = 1.0f;

	for (uint32_t queueFamily = 0; queueFamily < uniqueQueueFamilies.size(); ++queueFamily)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.flags = 0;
		queueCreateInfo.pNext = nullptr;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Specify used device features...
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;		// Enabling anisotropy!
	deviceFeatures.fillModeNonSolid = VK_TRUE;

	// Request GPU features related to ray tracing!
	VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {};
	bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
	bufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay = VK_FALSE;
	bufferDeviceAddressFeatures.bufferDeviceAddressMultiDevice = VK_FALSE;
	bufferDeviceAddressFeatures.pNext = nullptr;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {};
	rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
	rayTracingPipelineFeatures.pNext = &bufferDeviceAddressFeatures;
	
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelStrcutureFeatures = {};
	accelStrcutureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	accelStrcutureFeatures.accelerationStructure = VK_TRUE;
	accelStrcutureFeatures.accelerationStructureCaptureReplay = VK_TRUE;
	accelStrcutureFeatures.accelerationStructureHostCommands = VK_FALSE;
	accelStrcutureFeatures.accelerationStructureIndirectBuild = VK_FALSE;
	accelStrcutureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE;
	accelStrcutureFeatures.pNext = &rayTracingPipelineFeatures;

	// Create logical device...
	VkDeviceCreateInfo createInfo{};
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

	// These are part of Vulkan Instance from 1.1 & deprecated as part of logical device!
	createInfo.enabledExtensionCount = Vulkan::g_strDeviceExtensions.size();
	createInfo.ppEnabledExtensionNames = Vulkan::g_strDeviceExtensions.data();

	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	if (Vulkan::g_bEnableValidationLayer)
	{
		createInfo.enabledLayerCount = Vulkan::g_strValidationLayers.size();
		createInfo.ppEnabledLayerNames = Vulkan::g_strValidationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	createInfo.pNext = &accelStrcutureFeatures;


	if (vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkLogicalDevice) != VK_SUCCESS)
	{
		LOG_CRITICAL("Failed to create logical vulkan device!");
		return;
	}

	// The queues are automatically created along with the logical device, but we don't 
	// have a handle to interface with them yet. Since we are only creating a single queue
	// from this family, we will use index 0
	vkGetDeviceQueue(m_vkLogicalDevice, m_pQueueFamilyIndices->m_uiGraphicsFamily.value(), 0, &m_vkQueueGraphics);
	vkGetDeviceQueue(m_vkLogicalDevice, m_pQueueFamilyIndices->m_uiPresentFamily.value(), 0, &m_vkQueuePresent);

	LOG_INFO("Logical Device Created!");
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanDevice::CreateGraphicsCommandPool()
{
	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = m_pQueueFamilyIndices->m_uiGraphicsFamily.value();
	commandPoolCreateInfo.pNext = nullptr;

	// Create a Graphics queue family Command Pool
	if (vkCreateCommandPool(m_vkLogicalDevice, &commandPoolCreateInfo, nullptr, &m_vkCommandPoolGraphics) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Graphics Command Pool!");
	}
	else
		LOG_DEBUG("Created Graphics Command Pool!");
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanDevice::CreateGraphicsCommandBuffers(uint32_t size)
{
	m_vecCommandBufferGraphics.resize(size);

	VkCommandBufferAllocateInfo commandBufferAllocInfo{};
	commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocInfo.commandPool = m_vkCommandPoolGraphics;
	commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;						// Buffer you submit directly to the queue. can't be called by other buffers!
																						// BUFFER_LEVEL_SECONDARY can't be called directly but can be called from other buffers via "vkCmdExecuteCommands"
	commandBufferAllocInfo.commandBufferCount = (uint32_t)m_vecCommandBufferGraphics.size();
	commandBufferAllocInfo.pNext = nullptr;

	// Allocate command buffers & place handles in array of buffers!
	if (vkAllocateCommandBuffers(m_vkLogicalDevice, &commandBufferAllocInfo, m_vecCommandBufferGraphics.data()) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Graphics Command buffer!");
	}
	else
		LOG_INFO("Created Graphics Command buffers!");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//--- Find suitable memory type based on allowed type & property flags
uint32_t VulkanDevice::FindMemoryTypeIndex(uint32_t allowedTypeIndex, VkMemoryPropertyFlags props)
{
	// Get properties of physical device memory
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &m_vkDeviceMemoryProps);

	for (uint32_t i = 0; i < m_vkDeviceMemoryProps.memoryTypeCount; i++)
	{
		if ((allowedTypeIndex & (1 << i))												// Index of memory type must match corresponding bit in allowed types!
			&& (m_vkDeviceMemoryProps.memoryTypes[i].propertyFlags & props) == props)	// Desired property bit flags are part of the memory type's property flags!
		{
			// This memory type is valid, so return index!
			return i;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//--- Create VkBuffer & VkDeviceMemory of specific size, based on usage flags & property flags. 
void VulkanDevice::CreateBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags bufferProperties, 
								VkBuffer* outBuffer, VkDeviceMemory* outBufferMemory, const std::string& debugName)
{

	// Information to create a buffer (doesn't include assigning memory)
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize;													// total size of buffer
	bufferInfo.usage = bufferUsageFlags;											// type of buffer
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;								// similar to swap chain images, can share vertex buffers

	if (vkCreateBuffer(m_vkLogicalDevice, &bufferInfo, nullptr, outBuffer) != VK_SUCCESS)
		LOG_ERROR("Failed to create Vertex Buffer");

	Vulkan::SetDebugUtilsObjectName(m_vkLogicalDevice, VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(*outBuffer), (debugName + "_buffer"));

	// GET BUFFER MEMORY REQUIREMENTS
	VkMemoryRequirements	memRequirements;
	vkGetBufferMemoryRequirements(m_vkLogicalDevice, *outBuffer, &memRequirements);

	// ALLOCATE MEMORY TO BUFFER
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memRequirements.size;
	memoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, bufferProperties); // Index of memory type on Physical Device that has required bit flags

	// If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
	VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
	if (bufferUsageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
		allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
		memoryAllocInfo.pNext = &allocFlagsInfo;
	}

	// Allocated memory to VkDeviceMemory
	if (vkAllocateMemory(m_vkLogicalDevice, &memoryAllocInfo, nullptr, outBufferMemory) != VK_SUCCESS)
		LOG_ERROR("Failed to allocated Vertex Buffer Memory!");

	Vulkan::SetDebugUtilsObjectName(m_vkLogicalDevice, VK_OBJECT_TYPE_DEVICE_MEMORY, reinterpret_cast<uint64_t>(*outBufferMemory), (debugName + "_deviceMemory"));

	// Allocate memory to given Vertex buffer
	vkBindBufferMemory(m_vkLogicalDevice, *outBuffer, *outBufferMemory, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//--- Create VkBuffer & VkDeviceMemory of specific size, based on usage flags & property flags. 
void VulkanDevice::CreateBufferAndCopyData(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags bufferProperties,
										   VkBuffer* outBuffer, VkDeviceMemory* outBufferMemory, void* inData, const std::string& debugName)
{

	// Information to create a buffer (doesn't include assigning memory)
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize;													// total size of buffer
	bufferInfo.usage = bufferUsageFlags;											// type of buffer
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;								// similar to swap chain images, can share vertex buffers

	if (vkCreateBuffer(m_vkLogicalDevice, &bufferInfo, nullptr, outBuffer) != VK_SUCCESS)
		LOG_ERROR("Failed to create Vertex Buffer");

	Vulkan::SetDebugUtilsObjectName(m_vkLogicalDevice, VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(*outBuffer), (debugName + "_buffer"));

	// GET BUFFER MEMORY REQUIREMENTS
	VkMemoryRequirements	memRequirements;
	vkGetBufferMemoryRequirements(m_vkLogicalDevice, *outBuffer, &memRequirements);

	// ALLOCATE MEMORY TO BUFFER
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memRequirements.size;
	memoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, bufferProperties); // Index of memory type on Physical Device that has required bit flags

	// If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
	VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
	if (bufferUsageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) 
	{
		allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
		allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
		memoryAllocInfo.pNext = &allocFlagsInfo;
	}

	// Allocated memory to VkDeviceMemory
	if (vkAllocateMemory(m_vkLogicalDevice, &memoryAllocInfo, nullptr, outBufferMemory) != VK_SUCCESS)
		LOG_ERROR("Failed to allocated Vertex Buffer Memory!");

	Vulkan::SetDebugUtilsObjectName(m_vkLogicalDevice, VK_OBJECT_TYPE_DEVICE_MEMORY, reinterpret_cast<uint64_t>(*outBufferMemory), (debugName + "_deviceMemory"));

	// If a pointer to the buffer data has been passed, map the buffer & copy over the data!
	if (inData != nullptr)
	{
		void* data;
		VKRESULT_CHECK(vkMapMemory(m_vkLogicalDevice, *outBufferMemory, 0, bufferSize, 0, &data));
		memcpy(data, inData, bufferSize);											
		vkUnmapMemory(m_vkLogicalDevice, *outBufferMemory);								
	}

	// Allocate memory to given Vertex buffer
	vkBindBufferMemory(m_vkLogicalDevice, *outBuffer, *outBufferMemory, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//--- Begin Command buffer for recording commands! 
VkCommandBuffer VulkanDevice::BeginCommandBuffer()
{
	// Command buffer to hold transfer command
	VkCommandBuffer commandBuffer;

	// Command buffer details
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_vkCommandPoolGraphics;
	allocInfo.commandBufferCount = 1;

	// Allocate command buffer from pool
	vkAllocateCommandBuffers(m_vkLogicalDevice, &allocInfo, &commandBuffer);

	// Information to begin the command buffer record!
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;	// We are only using the command buffer once, so set for one time submit!

	// Begin recording transfer commands
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//--- End recording Commands & submit them to the Queue!
void VulkanDevice::EndAndSubmitCommandBuffer(VkCommandBuffer commandBuffer)
{
	// End Commands!
	vkEndCommandBuffer(commandBuffer);

	// Queue submission information
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Submit transfer command to transfer queue (which is same as Graphics Queue) & wait until it finishes!
	vkQueueSubmit(m_vkQueueGraphics, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_vkQueueGraphics);

	// Free temporary command buffer back to pool!
	vkFreeCommandBuffers(m_vkLogicalDevice, m_vkCommandPoolGraphics, 1, &commandBuffer);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//--- Generic Copy buffer from srcBuffer to dstBuffer using transferQueue & transferCommandPool of specific size
void VulkanDevice::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
	// Create buffer
	VkCommandBuffer transferCommandBuffer = BeginCommandBuffer();

	// Region of data to copy from and to 
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = 0;
	bufferCopyRegion.size = bufferSize;

	// Command to copy src buffer to dst buffer
	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	EndAndSubmitCommandBuffer(transferCommandBuffer);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanDevice::Cleanup()
{
	// clean-up existing command buffer & reuse existing pool to allocate new command buffers instead of recreating it!
	vkFreeCommandBuffers(m_vkLogicalDevice, m_vkCommandPoolGraphics, static_cast<uint32_t>(m_vecCommandBufferGraphics.size()), m_vecCommandBufferGraphics.data());

	// Destroy command pool
	vkDestroyCommandPool(m_vkLogicalDevice, m_vkCommandPoolGraphics, nullptr);

	vkDestroyDevice(m_vkLogicalDevice, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanDevice::CleanupOnWindowResize()
{
	// clean-up existing command buffer & reuse existing pool to allocate new command buffers instead of recreating it!
	vkFreeCommandBuffers(m_vkLogicalDevice, m_vkCommandPoolGraphics, static_cast<uint32_t>(m_vecCommandBufferGraphics.size()), m_vecCommandBufferGraphics.data());
}
