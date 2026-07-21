#pragma once

#include <vulkan/vulkan.h>
#include <core/memory/int_types.h>
#include "vk_config.h"

#include <array>

namespace imp::gfx::vulkan
{
	class VulkanDescriptorAllocator
	{
	public:
		VulkanDescriptorAllocator() = default;
		~VulkanDescriptorAllocator();

		VulkanDescriptorAllocator(const VulkanDescriptorAllocator&) = delete;
		VulkanDescriptorAllocator& operator=(const VulkanDescriptorAllocator&) = delete;

		bool create(VkDevice device, u32 maxSetsPerFrame = 64, const VkAllocationCallbacks* allocationCallbacks = nullptr);
		void destroy();

		void resetFrame(u32 frameIndex);
		VkDescriptorSet allocate(u32 frameIndex, VkDescriptorSetLayout layout);

	private:
		VkDevice m_device = VK_NULL_HANDLE;
		const VkAllocationCallbacks* m_allocationCallbacks = nullptr;
		std::array<VkDescriptorPool, kMaxFramesInFlight> m_pools{};
	};
}
