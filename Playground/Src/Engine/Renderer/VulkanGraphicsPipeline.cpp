#include "PlaygroundPCH.h"
#include "VulkanGraphicsPipeline.h"

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

#include "Engine/Helpers/Utility.h"
#include "Engine/Helpers/Log.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

//#define WIREFRAME_MODE

//---------------------------------------------------------------------------------------------------------------------
VulkanGraphicsPipeline::VulkanGraphicsPipeline(PipelineType type, VulkanSwapChain* pSwapChain)
{
	m_vkPipelineLayout = VK_NULL_HANDLE;
	m_vkGraphicsPipeline = VK_NULL_HANDLE;

	m_strVertexShader.clear();
	m_strFragmentShader.clear();

	m_eType = type;
}

//---------------------------------------------------------------------------------------------------------------------
VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanGraphicsPipeline::CreatePipelineLayout(VulkanDevice* pDevice, const std::vector<VkDescriptorSetLayout>& layouts,
													const std::vector<VkPushConstantRange> pushConstantRanges)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
	pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();

	// create pipeline layout
	if (vkCreatePipelineLayout(pDevice->m_vkLogicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Pipeline layout!");
	}
	else
		LOG_INFO("Created Pipeline layout");
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanGraphicsPipeline::CreateDefaultPipelineConfigInfo(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, 
															 VkRenderPass renderPass, uint32_t subPass, VkPipelineLayout layout, 
															 PipelineConfigInfo& outInfo)
{
	// Input Assembly
	outInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	outInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	outInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	//--- Viewport rect
	outInfo.viewport.x = 0.0f;
	outInfo.viewport.y = 0.0f;
	outInfo.viewport.width = (float)pSwapchain->m_vkSwapchainExtent.width;
	outInfo.viewport.height = (float)pSwapchain->m_vkSwapchainExtent.height;
	outInfo.viewport.minDepth = 0.0f;
	outInfo.viewport.maxDepth = 1.0f;

	//--- Scissor rect 
	outInfo.scissorRect.offset = { 0, 0 };															// offset to use region from
	outInfo.scissorRect.extent = pSwapchain->m_vkSwapchainExtent;									// extent to describe region to use, starting at offset!

	//--- combine viewport & scissor rect info to create viewport info
	outInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	outInfo.viewportInfo.flags = 0;
	outInfo.viewportInfo.pNext = nullptr;
	outInfo.viewportInfo.pScissors = &(outInfo.scissorRect);
	outInfo.viewportInfo.pViewports = &(outInfo.viewport);
	outInfo.viewportInfo.scissorCount = 1;
	outInfo.viewportInfo.viewportCount = 1;

	//--- Rasterizer
	outInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	outInfo.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;									// which face of a triangle to cull
	outInfo.rasterizationInfo.depthClampEnable = VK_FALSE;										// change if fragments beyond near/far planes are clipped.
	outInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;						// treat clockwise as front side
	outInfo.rasterizationInfo.lineWidth = 1.0f;													// how thick the line should be drawn
	outInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;								// how to handle filling points between vertices
	outInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;								// whether to discard data & skip rasterizer. Never creates fragments, only suitable for pipelines without framebuffers!
	outInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;										// whether to add depth bias to fragments (good for stopping "shadow acne")
	outInfo.rasterizationInfo.depthBiasClamp = 0.0f;
	outInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;
	outInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;
	outInfo.rasterizationInfo.flags = 0;
	outInfo.rasterizationInfo.pNext = nullptr;

	//--- Multi-sampling
	outInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	outInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;									// Enable multisampling or not
	outInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;					// number of samples per fragment
	outInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
	outInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;
	outInfo.multisampleInfo.flags = 0;
	outInfo.multisampleInfo.minSampleShading = 1.0f;
	outInfo.multisampleInfo.pNext = nullptr;
	outInfo.multisampleInfo.pSampleMask = nullptr;

	//--- Color Blending
	outInfo.vecColorBlendAttachments.clear();
	
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	outInfo.vecColorBlendAttachments.push_back(colorBlendAttachment);

	outInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	outInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
	outInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
	outInfo.colorBlendInfo.attachmentCount = static_cast<uint32_t>(outInfo.vecColorBlendAttachments.size());
	outInfo.colorBlendInfo.pAttachments = outInfo.vecColorBlendAttachments.data();
	outInfo.colorBlendInfo.blendConstants[0] = 0.0f;
	outInfo.colorBlendInfo.blendConstants[1] = 0.0f;
	outInfo.colorBlendInfo.blendConstants[2] = 0.0f;
	outInfo.colorBlendInfo.blendConstants[3] = 0.0f;

	//--- Depth & Stencil testing 
	outInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	outInfo.depthStencilInfo.depthTestEnable = VK_TRUE;										// Enable depth check to determine fragment write
	outInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;									// Enable writing to depth buffer to replace old values
	outInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;					// Comparison opearation that allows an overwrite (is in front)
	outInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;								// Depth bounds test, does the depth value exist between two bounds!
	outInfo.depthStencilInfo.minDepthBounds = 0.0f;
	outInfo.depthStencilInfo.maxDepthBounds = 1.0f;
	outInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;									// Enable/Disable Stencil test
	outInfo.depthStencilInfo.front = {};
	outInfo.depthStencilInfo.back = {};

	// Assign incoming information!
	outInfo.renderPass = renderPass;
	outInfo.subpass = subPass;
	outInfo.pipelineLayout = layout;
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanGraphicsPipeline::CreateGraphicsPipeline(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain, 
													VkRenderPass renderPass, uint32_t subPass, uint32_t nOutputAttachments)
{
	VkShaderModule vertShaderModule = VK_NULL_HANDLE;
	VkShaderModule fragShaderModule = VK_NULL_HANDLE;

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};

	// Create default configuration for the pipeline!
	PipelineConfigInfo m_sPipelineConfigInfo = {};
	CreateDefaultPipelineConfigInfo(pDevice, pSwapChain, renderPass, subPass, m_vkPipelineLayout, m_sPipelineConfigInfo);

	switch (m_eType)
	{
		case PipelineType::GBUFFER_OPAQUE:
			{
				m_strVertexShader = "Assets/Shaders/GBuffer.vert.spv";
				m_strFragmentShader = "Assets/Shaders/GBuffer.frag.spv";

				vertShaderModule = Vulkan::CreateShaderModule(pDevice, m_strVertexShader);
				fragShaderModule = Vulkan::CreateShaderModule(pDevice, m_strFragmentShader);

				//--- How the data for the single vertex (including info such as Position, color, texcoords etc.) is as a whole
				VkVertexInputBindingDescription bindingDescription = {};
				bindingDescription.binding = 0; // can bind multiple stream of data, this defines which one?
				bindingDescription.stride = sizeof(App::VertexPNTBT); // size of single vertex object
				bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
				// How to move between data after each vertex
				// VK_VERTEX_INPUT_RATE_VERTEX : move on to the next vertex																							// VK_VERTEX_INPUT_RATE_INSTANCE: move on to a vertex of next instance.
				// How the data for an attribute is defined within a vertex
				std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions;

				// Position attribute
				attributeDescriptions[0].binding = 0; // which binding the data is at (should be same as above)
				attributeDescriptions[0].location = 0; // location in shader where data will be read from
				attributeDescriptions[0].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
				// format the data will take (also helps define size of the data)
				attributeDescriptions[0].offset = offsetof(App::VertexPNTBT, Position);
				// where this attribute is defined in the data for a single vertex

				// Normal attribute
				attributeDescriptions[1].binding = 0;
				attributeDescriptions[1].location = 1;
				attributeDescriptions[1].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[1].offset = offsetof(App::VertexPNTBT, Normal);

				// Tangent attribute
				attributeDescriptions[2].binding = 0;
				attributeDescriptions[2].location = 2;
				attributeDescriptions[2].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[2].offset = offsetof(App::VertexPNTBT, Tangent);

				// BiNormal attribute
				attributeDescriptions[3].binding = 0;
				attributeDescriptions[3].location = 3;
				attributeDescriptions[3].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[3].offset = offsetof(App::VertexPNTBT, BiNormal);

				// Texture attribute
				attributeDescriptions[4].binding = 0;
				attributeDescriptions[4].location = 4;
				attributeDescriptions[4].format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
				attributeDescriptions[4].offset = offsetof(App::VertexPNTBT, UV);

				// Vertex Input
				vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
				vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
				// List of vertex attribute descriptions (data format & where to bind to - from)
				vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
				vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
				// List of vertex binding descriptions (data spacing/strides info) 
				vertexInputStateCreateInfo.flags = 0;
				vertexInputStateCreateInfo.pNext = nullptr;

#ifdef WIREFRAME_MODE
				m_sPipelineConfigInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
				m_sPipelineConfigInfo.rasterizationInfo.lineWidth = 1.0f;
#endif

				//--- Color blending
				// We need to explicitly mention the blending setting between all output attachments else default colormask will be 0x0
				// and nothing will be rendered to the attachment! 
				m_sPipelineConfigInfo.vecColorBlendAttachments.clear();
				for (int i = 0; i < nOutputAttachments; ++i)
				{
					VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
					colorBlendAttachment.colorWriteMask = 0xf; // All channels
					colorBlendAttachment.blendEnable = VK_FALSE;

					m_sPipelineConfigInfo.vecColorBlendAttachments.push_back(colorBlendAttachment);
				}

				m_sPipelineConfigInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				m_sPipelineConfigInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
				// alternative to calculations is to use logical operations
				m_sPipelineConfigInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
				m_sPipelineConfigInfo.colorBlendInfo.attachmentCount = static_cast<uint32_t>(m_sPipelineConfigInfo.vecColorBlendAttachments.size());
				m_sPipelineConfigInfo.colorBlendInfo.pAttachments = m_sPipelineConfigInfo.vecColorBlendAttachments.data();
				m_sPipelineConfigInfo.colorBlendInfo.blendConstants[0] = 0.0f;
				m_sPipelineConfigInfo.colorBlendInfo.blendConstants[1] = 0.0f;
				m_sPipelineConfigInfo.colorBlendInfo.blendConstants[2] = 0.0f;
				m_sPipelineConfigInfo.colorBlendInfo.blendConstants[3] = 0.0f;
				m_sPipelineConfigInfo.colorBlendInfo.flags = 0;
				m_sPipelineConfigInfo.colorBlendInfo.pNext = nullptr;

				break;
			}

		case PipelineType::HDRI_SKYDOME:
		{
			m_strVertexShader = "Assets/Shaders/HDRISkydome.vert.spv";
			m_strFragmentShader = "Assets/Shaders/HDRISkydome.frag.spv";

			vertShaderModule = Vulkan::CreateShaderModule(pDevice, m_strVertexShader);
			fragShaderModule = Vulkan::CreateShaderModule(pDevice, m_strFragmentShader);

			//--- How the data for the single vertex (including info such as Position, color, texcoords etc.) is as a whole
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0; // can bind multiple stream of data, this defines which one?
			bindingDescription.stride = sizeof(App::VertexPNT); // size of single vertex object
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;

			// Position attribute
			attributeDescriptions[0].binding = 0; // which binding the data is at (should be same as above)
			attributeDescriptions[0].location = 0; // location in shader where data will be read from
			attributeDescriptions[0].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
			// format the data will take (also helps define size of the data)
			attributeDescriptions[0].offset = offsetof(App::VertexPNT, Position);
			// where this attribute is defined in the data for a single vertex

			// Normal attribute
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(App::VertexPNT, Normal);

			// UV attribute
			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(App::VertexPNT, UV);

			// Vertex Input
			vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
			vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
			// List of vertex attribute descriptions (data format & where to bind to - from)
			vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
			vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
			// List of vertex binding descriptions (data spacing/strides info) 
			vertexInputStateCreateInfo.flags = 0;
			vertexInputStateCreateInfo.pNext = nullptr;

#ifdef WIREFRAME_MODE
			m_vkRasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_LINE;
			m_vkRasterizerCreateInfo.lineWidth = 1.0f;
#endif

			//--- Color blending
			// We need to explicitly mention the blending setting between all output attachments else default colormask will be 0x0
			// and nothing will be rendered to the attachment!bv  
			m_sPipelineConfigInfo.vecColorBlendAttachments.clear();
			for (int i = 0; i < nOutputAttachments; ++i)
			{
				VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
				colorBlendAttachment.colorWriteMask = 0xf; // All channels
				colorBlendAttachment.blendEnable = VK_FALSE;

				m_sPipelineConfigInfo.vecColorBlendAttachments.push_back(colorBlendAttachment);
			}

			m_sPipelineConfigInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			m_sPipelineConfigInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
			// alternative to calculations is to use logical operations
			m_sPipelineConfigInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
			m_sPipelineConfigInfo.colorBlendInfo.attachmentCount = static_cast<uint32_t>(m_sPipelineConfigInfo.vecColorBlendAttachments.size());
			m_sPipelineConfigInfo.colorBlendInfo.pAttachments = m_sPipelineConfigInfo.vecColorBlendAttachments.data();
			m_sPipelineConfigInfo.colorBlendInfo.blendConstants[0] = 0.0f;
			m_sPipelineConfigInfo.colorBlendInfo.blendConstants[1] = 0.0f;
			m_sPipelineConfigInfo.colorBlendInfo.blendConstants[2] = 0.0f;
			m_sPipelineConfigInfo.colorBlendInfo.blendConstants[3] = 0.0f;
			m_sPipelineConfigInfo.colorBlendInfo.flags = 0;
			m_sPipelineConfigInfo.colorBlendInfo.pNext = nullptr;

			break;
		}
			
		case PipelineType::DEFERRED:
		{
			m_strVertexShader = "Assets/Shaders/Deferred.vert.spv";
			m_strFragmentShader = "Assets/Shaders/Deferred.frag.spv";

			vertShaderModule = Vulkan::CreateShaderModule(pDevice, m_strVertexShader);
			fragShaderModule = Vulkan::CreateShaderModule(pDevice, m_strFragmentShader);

			// No vertex data for Final beauty 
			vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
			vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
			vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
			vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

			// disable writing to depth buffer
			m_sPipelineConfigInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;

			//--- Color blending
			// We need to explicitly mention the blending setting between all output attachments else default colormask will be 0x0
			// and nothing will be rendered to the attachment!
			m_sPipelineConfigInfo.vecColorBlendAttachments.clear();
			for (int i = 0; i < nOutputAttachments; ++i)
			{
				VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
				colorBlendAttachment.colorWriteMask = 0xf; // All channels
				colorBlendAttachment.blendEnable = VK_FALSE;

				m_sPipelineConfigInfo.vecColorBlendAttachments.push_back(colorBlendAttachment);
			}

			m_sPipelineConfigInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			m_sPipelineConfigInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
			// alternative to calculations is to use logical operations
			m_sPipelineConfigInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
			m_sPipelineConfigInfo.colorBlendInfo.attachmentCount = static_cast<uint32_t>(m_sPipelineConfigInfo.vecColorBlendAttachments.size());
			m_sPipelineConfigInfo.colorBlendInfo.pAttachments = m_sPipelineConfigInfo.vecColorBlendAttachments.data();
			m_sPipelineConfigInfo.colorBlendInfo.blendConstants[0] = 0.0f;
			m_sPipelineConfigInfo.colorBlendInfo.blendConstants[1] = 0.0f;
			m_sPipelineConfigInfo.colorBlendInfo.blendConstants[2] = 0.0f;
			m_sPipelineConfigInfo.colorBlendInfo.blendConstants[3] = 0.0f;
			m_sPipelineConfigInfo.colorBlendInfo.flags = 0;
			m_sPipelineConfigInfo.colorBlendInfo.pNext = nullptr;

			break;
		}
	}
	
	//--- to actually use shaders, we need to assign them to a specific pipeline stage
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.flags = 0;
	vertShaderStageInfo.pNext = nullptr;
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.flags = 0;
	fragShaderStageInfo.pNext = nullptr;
	fragShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


	// Dynamic state (nullptr for now!)

	// Finally, Create Graphics Pipeline!!!
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo.sType				=	VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount			=	2;
	graphicsPipelineCreateInfo.pStages				=	shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState	=	&vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState	=	&m_sPipelineConfigInfo.inputAssemblyInfo;
	graphicsPipelineCreateInfo.pViewportState		=	&m_sPipelineConfigInfo.viewportInfo;
	graphicsPipelineCreateInfo.pDynamicState		=	nullptr;
	graphicsPipelineCreateInfo.pRasterizationState	=	&m_sPipelineConfigInfo.rasterizationInfo;
	graphicsPipelineCreateInfo.pMultisampleState	=	&m_sPipelineConfigInfo.multisampleInfo;
	graphicsPipelineCreateInfo.pColorBlendState		=	&m_sPipelineConfigInfo.colorBlendInfo;
	graphicsPipelineCreateInfo.pDepthStencilState	=	&m_sPipelineConfigInfo.depthStencilInfo;
	graphicsPipelineCreateInfo.layout				=	m_sPipelineConfigInfo.pipelineLayout;
	graphicsPipelineCreateInfo.renderPass			=	m_sPipelineConfigInfo.renderPass;
	graphicsPipelineCreateInfo.subpass				=	m_sPipelineConfigInfo.subpass;
	graphicsPipelineCreateInfo.pNext				=	nullptr;
	graphicsPipelineCreateInfo.basePipelineHandle	=	VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex	=	-1;
	graphicsPipelineCreateInfo.flags				=	0;
	graphicsPipelineCreateInfo.pTessellationState	=	nullptr;


	if (vkCreateGraphicsPipelines(pDevice->m_vkLogicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_vkGraphicsPipeline) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Graphics Pipeline!");
	}
	else
		LOG_INFO("Created Graphics Pipeline!");

	// Destroy shader modules...
	vkDestroyShaderModule(pDevice->m_vkLogicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(pDevice->m_vkLogicalDevice, fragShaderModule, nullptr);

	vertShaderModule = VK_NULL_HANDLE;
	fragShaderModule = VK_NULL_HANDLE;
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanGraphicsPipeline::Cleanup(VulkanDevice* pDevice)
{
	vkDestroyPipeline(pDevice->m_vkLogicalDevice, m_vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(pDevice->m_vkLogicalDevice, m_vkPipelineLayout, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanGraphicsPipeline::CleanupOnWindowResize(VulkanDevice* pDevice)
{
	vkDestroyPipeline(pDevice->m_vkLogicalDevice, m_vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(pDevice->m_vkLogicalDevice, m_vkPipelineLayout, nullptr);
}


