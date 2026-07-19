#pragma once

#include <gfx/pipeline.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace imp::fs { class VirtualFileSystem; }
namespace imp::gfx::vulkan
{
	struct VulkanGraphicsPipelineCreateInfo
	{
		VkDevice device = VK_NULL_HANDLE;
		VkShaderModule vertexShader = VK_NULL_HANDLE;
		VkShaderModule fragmentShader = VK_NULL_HANDLE;

		VkVertexInputBindingDescription vertexBinding{};
		std::vector<VkVertexInputAttributeDescription> vertexAttributes;

		VkCullModeFlags cullMode = VK_CULL_MODE_NONE;

		bool depthTestEnable = false;
		bool depthWriteEnable = false;
		VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;

		VkFormat colourAttachmentFormat = VK_FORMAT_UNDEFINED;
		VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;

		u32 pushConstantSize = 0;

		const fs::VirtualFileSystem* vfs = nullptr;
		const VkAllocationCallbacks* allocationCallbacks = nullptr;
	};

	class VulkanGraphicsPipeline final : public gfx::IPipeline
	{
	public:
		VulkanGraphicsPipeline() = default;
		~VulkanGraphicsPipeline();

		VulkanGraphicsPipeline(const VulkanGraphicsPipeline&) = delete;
		VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline&) = delete;

		bool create(const VulkanGraphicsPipelineCreateInfo& info);
		void destroy();

		[[nodiscard]] VkPipeline pipeline() const { return m_pipeline; }
		[[nodiscard]] VkPipelineLayout layout() const { return m_layout; }
		[[nodiscard]] bool isValid() const { return m_pipeline != VK_NULL_HANDLE; }

	private:
		VkDevice m_device = VK_NULL_HANDLE;
		VkPipelineLayout m_layout = VK_NULL_HANDLE;
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		const VkAllocationCallbacks* m_allocationCallbacks = nullptr;
	};
}
