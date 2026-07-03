#pragma once

#include <vulkan/vulkan.h>
#include <core/memory/int_types.h>
#include "vk_config.h"

#include <array>

namespace imp::gfx::vulkan
{
	class VulkanCommandContext
	{
	public:
		VulkanCommandContext() = default;
		~VulkanCommandContext();

		VulkanCommandContext(const VulkanCommandContext&) = delete;
		VulkanCommandContext& operator=(const VulkanCommandContext&) = delete;

		bool create(VkDevice device, u32 queueFamily);
		void destroy();

		// Resets and begins recording into the command buffer for frameIndex
		// Caller must have already waited on that frame's in-flight fence.
		VkCommandBuffer beginRecording(u32 frameIndex);

		// Ends recording of the buffer most recently returned by
		// beginRecording() for this frameIndex
		void endRecording(u32 frameIndex);

		VkCommandBuffer commandBuffer(u32 frameIndex) const { return m_commandBuffers[frameIndex]; }

	private:
		VkDevice m_device = VK_NULL_HANDLE;
		VkCommandPool m_commandPool = VK_NULL_HANDLE;
		std::array<VkCommandBuffer, kMaxFramesInFlight> m_commandBuffers{};
	};
}