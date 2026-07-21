#if defined(IMP_GFX_VULKAN)

#include "vk_desc_alloc.h"
#include <core/log/log.h>

namespace imp::gfx::vulkan
{
	VulkanDescriptorAllocator::~VulkanDescriptorAllocator() { destroy(); }
	bool VulkanDescriptorAllocator::create(VkDevice device, u32 maxSetsPerFrame, const VkAllocationCallbacks* allocationCallbacks)
	{
		m_device = device;
		m_allocationCallbacks = allocationCallbacks;

		VkDescriptorPoolSize poolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxSetsPerFrame },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxSetsPerFrame }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = maxSetsPerFrame;
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = poolSizes;

		for (u32 i = 0; i < kMaxFramesInFlight; ++i)
		{
			if (vkCreateDescriptorPool(m_device, &poolInfo, m_allocationCallbacks, &m_pools[i]) != VK_SUCCESS)
			{
				LOG_ERROR("Vulkan", "vkCreateDescriptorPool failed");
				return false;
			}
		}

		return true;
	}

	void VulkanDescriptorAllocator::destroy()
	{
		for (VkDescriptorPool& p : m_pools)
		{
			if (p != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorPool(m_device, p, m_allocationCallbacks);
				p = VK_NULL_HANDLE;
			}
		}
		m_device = VK_NULL_HANDLE;
	}
	void VulkanDescriptorAllocator::resetFrame(u32 frameIndex)
	{
		vkResetDescriptorPool(m_device, m_pools[frameIndex], 0);
	}

	VkDescriptorSet VulkanDescriptorAllocator::allocate(u32 frameIndex, VkDescriptorSetLayout layout)
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_pools[frameIndex];
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		VkDescriptorSet set = VK_NULL_HANDLE;
		VkResult result = vkAllocateDescriptorSets(m_device, &allocInfo, &set); // can we put this inside the if?
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vkAllocateDescriptorSets failed (pool for this frame may be exhausted)");
			return VK_NULL_HANDLE;
		}
		return set;
	}
}

#endif // IMP_GFX_VULKAN
