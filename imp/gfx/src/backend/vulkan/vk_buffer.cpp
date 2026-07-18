#include "vk_buffer.h"
#include <core/log/log.h>

namespace imp::gfx::vulkan
{
	VulkanBuffer::~VulkanBuffer()
	{
		destroy();
	}

	bool VulkanBuffer::create(const VulkanBufferCreateInfo& info)
	{
		m_allocator = info.allocator;
		m_size = info.size;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = info.size;
		bufferInfo.usage = info.usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		if (info.hostVisible)
		{
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
				| VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}

		VmaAllocationInfo resultInfo{};
		VkResult result = vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, 
			&m_buffer, &m_allocation, &resultInfo);

		if (result != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vmaCreateBuffer failed");
			m_buffer = VK_NULL_HANDLE;
			m_allocation = VK_NULL_HANDLE;
			return false;
		}

		// not null ONLY IF MAPPED_BIT was requested above
		m_mappedData = resultInfo.pMappedData;
		return true;
	}

	void VulkanBuffer::destroy()
	{
		if (m_buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
			m_buffer = VK_NULL_HANDLE;
			m_allocation = VK_NULL_HANDLE;
			m_mappedData = nullptr;
		}
		m_allocator = VK_NULL_HANDLE;
	}
}
