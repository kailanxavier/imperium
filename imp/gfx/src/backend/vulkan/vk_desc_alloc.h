#pragma once

#include <vulkan/vulkan.h>
#include <core/types/int_types.h>
#include <gfx/config.h>

#include <array>

namespace imp::gfx::vulkan
{
	struct DescriptorBindingCount
	{
		VkDescriptorType type;
		u32 countPerSet;
	};

	inline constexpr DescriptorBindingCount kDescriptorBindingTable[] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4 },
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
	};

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
