#if defined(IMP_GFX_VULKAN)

#include "vk_pipeline.h"
#include "vk_shader.h"
#include "vk_vertex.h"

#include <core/log/log.h>
#include <core/math/mat4.h>
#include <core/memory/int_types.h>

namespace imp::gfx::vulkan
{
	VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
	{
		destroy();
	}

	bool VulkanGraphicsPipeline::create(const VulkanGraphicsPipelineCreateInfo& info)
	{
		m_device = info.device;
		m_allocationCallbacks = info.allocationCallbacks;

		VulkanShaderModule vertModule;
		if (!vertModule.loadFromFile(m_device, info.vertexShaderPath, m_allocationCallbacks))
			return false;

		VulkanShaderModule fragModule;
		if (!fragModule.loadFromFile(m_device, info.fragmentShaderPath, m_allocationCallbacks))
			return false;

		VkPipelineShaderStageCreateInfo stages[2]{};
		stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stages[0].module = vertModule.handle();
		stages[0].pName = "main";

		stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stages[1].module = fragModule.handle();
		stages[1].pName = "main";

		auto bindingDesc = Vertex::bindingDescription();
		auto attributeDesc = Vertex::attributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInput{};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexBindingDescriptionCount = 1;
		vertexInput.pVertexBindingDescriptions = &bindingDesc;
		vertexInput.vertexAttributeDescriptionCount = static_cast<u32>( attributeDesc.size() );
		vertexInput.pVertexAttributeDescriptions = attributeDesc.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

		// TODO: revisit this once there's an actual 3d pipeline with
		// a defined coordinate convention (once forward has been defined essentially)
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState blendAttachment{};
		blendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachment.blendEnable = VK_FALSE; // no transparency yet

		VkPipelineColorBlendStateCreateInfo colourBlend{};
		colourBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colourBlend.attachmentCount = 1;
		colourBlend.pAttachments = &blendAttachment;

		static_assert( sizeof(imp::math::Mat4f) == 64,
			"Mat4f layout changed - VulkanDevice's push-constant upload assumes 4 tightly packed Vec4f columns" );

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(imp::math::Mat4f);

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(m_device, &layoutInfo, m_allocationCallbacks, &m_layout) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vkCreatePipelineLayout failed");
			return false;
		}

		VkPipelineRenderingCreateInfo renderingCreateInfo{};
		renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		renderingCreateInfo.colorAttachmentCount = 1;
		renderingCreateInfo.pColorAttachmentFormats = &info.colourAttachmentFormat;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = &renderingCreateInfo;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = stages;
		pipelineInfo.pVertexInputState = &vertexInput;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colourBlend;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = m_layout;
		pipelineInfo.renderPass = VK_NULL_HANDLE;
		pipelineInfo.subpass = 0;

		if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, m_allocationCallbacks, &m_pipeline) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vkCreateGraphicsPipelines failed");
			vkDestroyPipelineLayout(m_device, m_layout, m_allocationCallbacks);
			m_layout = VK_NULL_HANDLE;
			return false;
		}

		return true;
	}

	void VulkanGraphicsPipeline::destroy()
	{
		if (m_device != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(m_device);
			if (m_pipeline != VK_NULL_HANDLE)
			{
				vkDestroyPipeline(m_device, m_pipeline, m_allocationCallbacks);
				m_pipeline = VK_NULL_HANDLE;
			}
			if (m_layout != VK_NULL_HANDLE)
			{
				vkDestroyPipelineLayout(m_device, m_layout, m_allocationCallbacks);
				m_layout = VK_NULL_HANDLE;
			}

			m_device = VK_NULL_HANDLE;
		}
	}

}

#endif
