#pragma once

#include <gfx/resources.h>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <vector>

namespace imp::gfx::vulkan
{
	struct VulkanTextureCreateInfo
	{
		VmaAllocator allocator = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		u32 width = 0;
		u32 height = 0;
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
		gfx::TextureUsage usage = gfx::TextureUsage::Sampled;
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

		bool transient = false;
		u32 mipLevels = 1;
		u32 arrayLayers = 1;

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

		[[nodiscard]] VkSampleCountFlagBits sampleCount() const { return m_sampleCount; }
		[[nodiscard]] bool isSampled() const { return m_isSampled; }

		[[nodiscard]] VkImage image() const { return m_image; }
		[[nodiscard]] VkImageView imageView() const { return m_arrayLayers > 1 ? m_arrayView : m_imageView; }
		[[nodiscard]] VkFormat vkFormat() const { return m_vkFormat; }
		[[nodiscard]] bool isValid() const { return m_image != VK_NULL_HANDLE; }

		[[nodiscard]] u32 mipLevels() const { return m_mipLevels; }

		[[nodiscard]] VkImageView arrayView() const { return m_arrayView; }
		[[nodiscard]] VkImageView layerView(u32 layer) const { 
			return layer < m_layerViews.size() ? m_layerViews[layer] : m_imageView; }
		[[nodiscard]] u32 arrayLayers() const { return m_arrayLayers; }

	private:
		VmaAllocator m_allocator = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		const VkAllocationCallbacks* m_allocationCallbacks = nullptr;

		VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
		bool m_isSampled = false;

		VkImage m_image = VK_NULL_HANDLE;
		VmaAllocation m_allocation = VK_NULL_HANDLE;
		VkImageView m_imageView = VK_NULL_HANDLE;
		VkFormat m_vkFormat = VK_FORMAT_UNDEFINED;
		u32 m_width = 0;
		u32 m_height = 0;

		u32 m_mipLevels = 1;
		u32 m_arrayLayers = 1;
		VkImageView m_arrayView = VK_NULL_HANDLE;
		std::vector<VkImageView> m_layerViews;
	};
}
