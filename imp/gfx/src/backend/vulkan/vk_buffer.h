#pragma once

#include <gfx/resources.h>

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

	class VulkanBuffer final : public gfx::IBuffer
	{
	public:
		VulkanBuffer() = default;
		~VulkanBuffer() override;

		VulkanBuffer(const VulkanBuffer&) = delete;
		VulkanBuffer& operator=(const VulkanBuffer&) = delete;

		bool create(const VulkanBufferCreateInfo& info);
		void destroy();

		[[nodiscard]] VkBuffer handle() const { return m_buffer; }
		[[nodiscard]] bool isValid() const { return m_buffer != VK_NULL_HANDLE; }

		[[nodiscard]] void* mappedData() override { return m_mappedData; }
		[[nodiscard]] const void* mappedData() const override { return m_mappedData; }
		[[nodiscard]] u64 size() const override { return static_cast<u64>(m_size); }

	private:
		VmaAllocator m_allocator = VK_NULL_HANDLE;
		VkBuffer m_buffer = VK_NULL_HANDLE;
		VmaAllocation m_allocation = VK_NULL_HANDLE;
		void* m_mappedData = nullptr;
		VkDeviceSize m_size = 0;
	};
}
