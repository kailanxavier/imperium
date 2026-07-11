#pragma once

// TODO: 
// This is NOT a general pipeline/material system.
// When we inevitably come to the point that we need multiple
// shaders, vertex layouts, blend modes, etc. we will need a
// builder that fills VulkanGraphicsPipelineCreateInfo per material
// and a cache keyed on that description rather than this class itself
// growing arms.

#include <vulkan/vulkan.h>
#include <string>

namespace imp::gfx::vulkan
{
	struct VulkanGraphicsPipelineCreateInfo
	{
		VkDevice device = VK_NULL_HANDLE;
		std::string vertexShaderPath;
		std::string fragmentShaderPath;

		VkFormat colourAttachmentFormat = VK_FORMAT_UNDEFINED;

		const VkAllocationCallbacks* allocationCallbacks = nullptr;
	};

	class VulkanGraphicsPipeline
	{
	public:
		VulkanGraphicsPipeline() = default;
		~VulkanGraphicsPipeline();

		VulkanGraphicsPipeline(const VulkanGraphicsPipeline&) = delete;
		VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline&) = delete;

		bool create(const VulkanGraphicsPipelineCreateInfo& info);
		void destroy();

		VkPipeline pipeline() const { return m_pipeline; }
		VkPipelineLayout layout() const { return m_layout; }
		bool isValid() const { return m_pipeline != VK_NULL_HANDLE; }

	private:
		VkDevice m_device = VK_NULL_HANDLE;
		VkPipelineLayout m_layout = VK_NULL_HANDLE;
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		const VkAllocationCallbacks* m_allocationCallbacks = nullptr;
	};
}
