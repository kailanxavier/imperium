#include "vk_commands.h"

#include <core/log/log.h>

namespace imp::gfx::vulkan
{
	VulkanCommandContext::~VulkanCommandContext()
	{
		destroy();
	}

	bool VulkanCommandContext::create(VkDevice device, u32 queueFamily, const VkAllocationCallbacks* allocationCallbacks)
	{
		m_device = device;
		m_allocationCallbacks = allocationCallbacks;

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamily;

		// RESET_COMMAND_BUFFER lets us reset individual buffer
		// rather than only being able to reset the whole pool at once;
		// we want to reuse each frame's buffer independently.
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(m_device, &poolInfo, allocationCallbacks, &m_commandPool) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vkCreateCommandPool failed");
			return false;
		}

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<u32>( m_commandBuffers.size() );

		if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vkAllocateCommandBuffers failed");
			return false;
		}

		return true;
	}

	void VulkanCommandContext::destroy()
	{
		if (m_commandPool != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(m_device);

			// Destroying the pool implicitly frees all command buffers
			// allocated from it; no need to vkFreeCommandBuffers first.
			vkDestroyCommandPool(m_device, m_commandPool, m_allocationCallbacks);
			m_commandPool = VK_NULL_HANDLE;
			m_commandBuffers.fill(VK_NULL_HANDLE);
		}

		m_device = VK_NULL_HANDLE;
	}

	VkCommandBuffer VulkanCommandContext::beginRecording(u32 frameIndex)
	{
		VkCommandBuffer cmd = m_commandBuffers[frameIndex];

		// vkBeginCommandBuffer implicitly resets a buffer allocated
		// from a pool created with RESET_COMMAND_BUFFER, so no explicit
		// vkResetCommandBuffer call is needed here.
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(cmd, &beginInfo);
		return cmd;
	}

	void VulkanCommandContext::endRecording(u32 frameIndex)
	{
		vkEndCommandBuffer(m_commandBuffers[frameIndex]);
	}
}
