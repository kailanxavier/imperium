#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace imp::gfx::vulkan
{
	struct VulkanBufferCreateInfo
	{
		VmaAllocator allocator = VK_NULL_HANDLE;
		VkDeviceSize size = 0;
		VkBufferUsageFlags usage = 0;

		bool hostVisible = true;
	};

	class VulkanBuffer
	{
	public:
		VulkanBuffer() = default;
		~VulkanBuffer();

		VulkanBuffer(const VulkanBuffer&) = delete;
		VulkanBuffer& operator=(const VulkanBuffer&) = delete;

		bool create(const VulkanBufferCreateInfo& info);
		void destroy();

		VkBuffer handle() const { return m_buffer; }
		void* mappedData() const { return m_mappedData; }
		VkDeviceSize size() const { return m_size; }
		bool isValid() const { return m_buffer != VK_NULL_HANDLE; }

	private:
		VmaAllocator m_allocator = VK_NULL_HANDLE;
		VkBuffer m_buffer = VK_NULL_HANDLE;
		VmaAllocation m_allocation = VK_NULL_HANDLE;
		void* m_mappedData = nullptr;
		VkDeviceSize m_size = 0;
	};
}
