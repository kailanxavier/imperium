#if defined(IMP_GFX_VULKAN)
#include "vk_texture.h"
#include "vk_render_target.h" // toGfxFormat
#include <core/log/log.h>

namespace imp::gfx::vulkan
{
	VulkanTexture::~VulkanTexture() { destroy(); }

	bool VulkanTexture::create(const VulkanTextureCreateInfo& info)
	{
		m_allocator = info.allocator;
		m_device = info.device;
		m_allocationCallbacks = info.allocationCallbacks;
		m_width = info.width;
		m_height = info.height;
		m_vkFormat = info.format;

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent = { m_width, m_height, 1 };
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = m_vkFormat;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

		if (vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vmaCreateImage (texture) failed");
			return false;
		}

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_vkFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_device, &viewInfo, m_allocationCallbacks, &m_imageView) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vkCreateImageView (texture) failed");
			vmaDestroyImage(m_allocator, m_image, m_allocation);
			m_image = VK_NULL_HANDLE;
			m_allocation = VK_NULL_HANDLE;
			return false;
		}

		return true;
	}

	void VulkanTexture::destroy()
	{
		if (m_imageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_device, m_imageView, m_allocationCallbacks);
			m_imageView = VK_NULL_HANDLE;
		}
		if (m_image != VK_NULL_HANDLE)
		{
			vmaDestroyImage(m_allocator, m_image, m_allocation);
			m_image = VK_NULL_HANDLE;
			m_allocation = VK_NULL_HANDLE;
		}
	}

	gfx::TextureFormat VulkanTexture::format() const
	{
		return toGfxFormat(m_vkFormat);
	}	
}

#endif
