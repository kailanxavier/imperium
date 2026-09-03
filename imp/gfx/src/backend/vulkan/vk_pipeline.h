#pragma once

#include <unordered_map>
#include <gfx/pipeline.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace imp::fs { class VirtualFileSystem; }
namespace imp::gfx::vulkan
{
	class VulkanShaderModule;

	struct PipelineBindingInfo
	{
		VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
		u32 count = 1;
		VkShaderStageFlags stageFlags = 0;
		std::string name;
	};

	struct VulkanGraphicsPipelineCreateInfo
	{
		VkDevice device = VK_NULL_HANDLE;
		VulkanShaderModule* vertexShader = nullptr;
		VulkanShaderModule* fragmentShader = nullptr;
		VkVertexInputBindingDescription vertexBinding{};

		bool hasInstanceBinding = false;
		VkVertexInputBindingDescription instanceBinding{};

		std::vector<VkVertexInputAttributeDescription> vertexAttributes;
		VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		bool depthTestEnable = false;
		bool depthWriteEnable = false;
		VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;

		VkFormat colourAttachmentFormat = VK_FORMAT_UNDEFINED;
		VkFormat colourAttachmentFormat1 = VK_FORMAT_UNDEFINED;

		VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

		bool blendEnable = false;
		const fs::VirtualFileSystem* vfs = nullptr;
		const VkAllocationCallbacks* allocationCallbacks = nullptr;
	};

	class VulkanGraphicsPipeline final : public gfx::IPipeline
	{
	public:
		VulkanGraphicsPipeline() = default;
		~VulkanGraphicsPipeline() override;

		VulkanGraphicsPipeline(const VulkanGraphicsPipeline&) = delete;
		VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline&) = delete;

		bool create(const VulkanGraphicsPipelineCreateInfo& info);
		void destroy();

		[[nodiscard]] VkPipeline pipeline() const { return m_pipeline; }
		[[nodiscard]] VkPipelineLayout layout() const { return m_layout; }
		[[nodiscard]] VkDescriptorSetLayout descriptorSetLayout() const { return m_descriptorSetLayout; }
		[[nodiscard]] bool isValid() const { return m_pipeline != VK_NULL_HANDLE; }
		[[nodiscard]] const std::unordered_map<u32, PipelineBindingInfo>& bindingLayout() const { return m_bindingLayout; }

	private:
		VkDevice m_device = VK_NULL_HANDLE;
		VkPipelineLayout m_layout = VK_NULL_HANDLE;
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
		const VkAllocationCallbacks* m_allocationCallbacks = nullptr;
		std::unordered_map<u32, PipelineBindingInfo> m_bindingLayout;
	};
}
