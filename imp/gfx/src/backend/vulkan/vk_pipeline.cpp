#if defined(IMP_GFX_VULKAN)

#include "vk_pipeline.h"
#include "vk_shader.h"

#include "vk_check.h"

#include <core/log/log.h>
#include <core/math/mat4.h>
#include <core/types/int_types.h>
#include <core/fs/vfs.h>

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

		VkPipelineShaderStageCreateInfo stages[2]{};
		stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stages[0].module = info.vertexShader->handle();
		stages[0].pName = "main";

		stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stages[1].module = info.fragmentShader->handle();
		stages[1].pName = "main";

		VkVertexInputBindingDescription bindingDescs[2] = { info.vertexBinding, info.instanceBinding };
		const u32 bindingCount = info.hasInstanceBinding ? 2u : 1u;

		VkPipelineVertexInputStateCreateInfo vertexInput{};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexBindingDescriptionCount = bindingCount;
		vertexInput.pVertexBindingDescriptions = bindingDescs;
		vertexInput.vertexAttributeDescriptionCount = static_cast<u32>( info.vertexAttributes.size() );
		vertexInput.pVertexAttributeDescriptions = info.vertexAttributes.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = info.topology;

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
		rasterizer.cullMode = info.cullMode;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.lineWidth = 1.f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.rasterizationSamples = info.sampleCount;

		VkPipelineColorBlendAttachmentState blendAttachment{};
		blendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachment.blendEnable = info.blendEnable ? VK_TRUE : VK_FALSE;
		if (info.blendEnable)
		{
			blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		}

		VkPipelineColorBlendStateCreateInfo colourBlend{};
		colourBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colourBlend.attachmentCount = 1;
		colourBlend.pAttachments = &blendAttachment;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = info.depthTestEnable ? VK_TRUE : VK_FALSE;
		depthStencil.depthWriteEnable = info.depthWriteEnable ? VK_TRUE : VK_FALSE;
		depthStencil.depthCompareOp = info.depthCompareOp;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		static_assert( sizeof(imp::math::Mat4f) == 64,
			"Mat4f layout changed the push-constant assumption in VulkanCommandList::pushConstants need re-checking" );

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = info.pushConstantSize;

		m_bindingLayout.clear();
		auto mergeStage = [&](const std::vector<ReflectedBinding>& bindings, VkShaderStageFlagBits stageFlag, const char* stageName) -> bool
		{
			for (const ReflectedBinding& b : bindings)
			{
				auto it = m_bindingLayout.find(b.binding);
				if (it == m_bindingLayout.end())
				{
					m_bindingLayout[b.binding] = { b.descriptorType, b.descriptorCount, static_cast<VkShaderStageFlags>( stageFlag ), stageName };
				}
				else if (it->second.type != b.descriptorType || it->second.count != b.descriptorCount)
				{
					LOG_ERROR("Vulkan", "Descriptor binding {} ('{}') disagrees between {} and {} stages. Refusing to create pipeline...",
						b.binding, b.name, it->second.name, stageName);
					return false;
				}
				else
				{
					it->second.stageFlags |= stageFlag;
				}
			}
			return true;
		};

		if (!mergeStage(info.vertexShader->reflectedBindings(), VK_SHADER_STAGE_VERTEX_BIT, "vertex")
			|| !mergeStage(info.fragmentShader->reflectedBindings(), VK_SHADER_STAGE_FRAGMENT_BIT, "fragment"))
		{
			return false;
		}

		std::vector<VkDescriptorSetLayoutBinding> bindings;
		bindings.reserve(m_bindingLayout.size());
		for (const auto& [bindingIndex, mb] : m_bindingLayout)
		{
			VkDescriptorSetLayoutBinding b{};
			b.binding = bindingIndex;
			b.descriptorType = mb.type;
			b.descriptorCount = mb.count;
			b.stageFlags = mb.stageFlags;
			bindings.push_back(b);
		}
		if (!bindings.empty())
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<u32>( bindings.size() );
			layoutInfo.pBindings = bindings.data();

			VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, m_allocationCallbacks, &m_descriptorSetLayout));
		}

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		if (info.pushConstantSize > 0)
		{
			layoutInfo.pushConstantRangeCount = 1;
			layoutInfo.pPushConstantRanges = &pushConstantRange;
		}
		if (m_descriptorSetLayout != VK_NULL_HANDLE)
		{
			layoutInfo.setLayoutCount = 1;
			layoutInfo.pSetLayouts = &m_descriptorSetLayout;
		}

		VK_CHECK(vkCreatePipelineLayout(m_device, &layoutInfo, m_allocationCallbacks, &m_layout));

		VkPipelineRenderingCreateInfo renderingCreateInfo{};
		renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		if (info.colourAttachmentFormat == VK_FORMAT_UNDEFINED)
		{
			renderingCreateInfo.colorAttachmentCount = 0;
			renderingCreateInfo.pColorAttachmentFormats = nullptr;
		}
		else
		{
			renderingCreateInfo.colorAttachmentCount = 1;
			renderingCreateInfo.pColorAttachmentFormats = &info.colourAttachmentFormat;
		}
		renderingCreateInfo.depthAttachmentFormat = info.depthAttachmentFormat;
		renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

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
		pipelineInfo.pDepthStencilState = &depthStencil;
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
		if (m_descriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, m_allocationCallbacks);
			m_descriptorSetLayout = VK_NULL_HANDLE;
		}
		m_device = VK_NULL_HANDLE;
	}
}

#endif // IMP_GFX_VULKAN
