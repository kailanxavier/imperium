#include "vk_buffer.h"
#include <core/log/log.h>

namespace imp::gfx::vulkan
{
	VulkanBuffer::~VulkanBuffer()
	{
		destroy();
	}

	VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
		: m_allocator(other.m_allocator)
		, m_buffer(other.m_buffer)
		, m_allocation(other.m_allocation)
		, m_mappedData(other.m_mappedData)
		, m_indexFormat(other.m_indexFormat)
		, m_size(other.m_size)
	{
		other.m_allocator = VK_NULL_HANDLE;
		other.m_buffer = VK_NULL_HANDLE;
		other.m_allocation = VK_NULL_HANDLE;
		other.m_mappedData = nullptr;
		other.m_size = 0;
	}

	VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept
	{
		if (this == &other)
			return *this;

		destroy(); // release whatever this buffer currently owns first

		m_allocator = other.m_allocator;
		m_buffer = other.m_buffer;
		m_allocation = other.m_allocation;
		m_mappedData = other.m_mappedData;
		m_indexFormat = other.m_indexFormat;
		m_size = other.m_size;

		other.m_allocator = VK_NULL_HANDLE;
		other.m_buffer = VK_NULL_HANDLE;
		other.m_allocation = VK_NULL_HANDLE;
		other.m_mappedData = nullptr;
		other.m_size = 0;

		return *this;
	}

	bool VulkanBuffer::create(const VulkanBufferCreateInfo& info)
	{
		m_allocator = info.allocator;
		m_size = info.size;
		m_indexFormat = info.indexFormat;

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

	bool VulkanBuffer::update(const void* data, u64 size, u64 offset)
	{
		if (!data || offset + size > m_size)
			return false;

		std::memcpy(static_cast<std::byte*>( m_mappedData ) + offset, data, size);
		return true;
	}
}
