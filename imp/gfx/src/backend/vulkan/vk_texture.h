#pragma once

#include <gfx/resources.h>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace imp::gfx::vulkan
{
	struct VulkanTextureCreateInfo
	{
		VmaAllocator allocator = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		u32 width = 0;
		u32 height = 0;
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

		const VkAllocationCallbacks* allocationCallbacks = nullptr;
	};

	class VulkanTexture final : public gfx::ITexture
	{
	public:
		VulkanTexture() = default;
		~VulkanTexture() override;

		VulkanTexture(const VulkanTexture&) = delete;
		VulkanTexture& operator=(const VulkanTexture&) = delete;

		bool create(const VulkanTextureCreateInfo& info);
		void destroy();

		[[nodiscard]] u32 width() const override { return m_width; }
		[[nodiscard]] u32 height() const override { return m_height; }
		[[nodiscard]] gfx::TextureFormat format() const override;

		[[nodiscard]] VkImage image() const { return m_image; }
		[[nodiscard]] VkImageView imageView() const { return m_imageView; }
		[[nodiscard]] VkFormat vkFormat() const { return m_vkFormat; }
		[[nodiscard]] bool isValid() const { return m_image != VK_NULL_HANDLE; }

	private:
		VmaAllocator m_allocator = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		const VkAllocationCallbacks* m_allocationCallbacks = nullptr;

		VkImage m_image = VK_NULL_HANDLE;
		VmaAllocation m_allocation = VK_NULL_HANDLE;
		VkImageView m_imageView = VK_NULL_HANDLE;
		VkFormat m_vkFormat = VK_FORMAT_UNDEFINED;
		u32 m_width = 0;
		u32 m_height = 0;
	};
}
