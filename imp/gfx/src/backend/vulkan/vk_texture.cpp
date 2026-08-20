#if defined(IMP_GFX_VULKAN)
#include "vk_texture.h"
#include "vk_render_target.h" // toGfxFormat
#include "vk_check.h"
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
		m_sampleCount = info.sampleCount;
		m_isSampled = gfx::hasFlag(info.usage, gfx::TextureUsage::Sampled);
		m_mipLevels = info.mipLevels;

		VkImageUsageFlags usageFlags = 0;
		if (!info.transient)
			usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if (gfx::hasFlag(info.usage, gfx::TextureUsage::Sampled))
			usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
		if (gfx::hasFlag(info.usage, gfx::TextureUsage::RenderTarget))
			usageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (gfx::hasFlag(info.usage, gfx::TextureUsage::DepthStencil))
			usageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (m_mipLevels > 1 && !info.transient)
			usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (info.transient)
			usageFlags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent = { m_width, m_height, 1 };
		imageInfo.mipLevels = m_mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = m_vkFormat;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usageFlags;
		imageInfo.samples = info.sampleCount;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.arrayLayers = info.arrayLayers;
		m_arrayLayers = info.arrayLayers;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

		if (info.transient)
			allocInfo.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;

		if (vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vmaCreateImage (texture) failed");
			return false;
		}

		VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		if (info.format == VK_FORMAT_D32_SFLOAT
			|| info.format == VK_FORMAT_D32_SFLOAT_S8_UINT
			|| info.format == VK_FORMAT_D24_UNORM_S8_UINT
			|| info.format == VK_FORMAT_D16_UNORM)
		{
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		if (m_arrayLayers > 1)
		{
			VkImageViewCreateInfo arrayViewInfo{};
			arrayViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			arrayViewInfo.image = m_image;
			arrayViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			arrayViewInfo.format = m_vkFormat;
			arrayViewInfo.subresourceRange.aspectMask = aspectMask;
			arrayViewInfo.subresourceRange.baseMipLevel = 0;
			arrayViewInfo.subresourceRange.levelCount = m_mipLevels;
			arrayViewInfo.subresourceRange.baseArrayLayer = 0;
			arrayViewInfo.subresourceRange.layerCount = m_arrayLayers;

			if (vkCreateImageView(m_device, &arrayViewInfo, m_allocationCallbacks, &m_arrayView) != VK_SUCCESS)
			{
				LOG_ERROR("Vulkan", "VulkanTexture::create(): array view creation failed!");
				destroy();
				return false;
			}

			m_layerViews.resize(m_arrayLayers);
			for (u32 layer = 0; layer < m_arrayLayers; ++layer)
			{
				VkImageViewCreateInfo layerViewInfo = arrayViewInfo;
				layerViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				layerViewInfo.subresourceRange.baseArrayLayer = layer;
				layerViewInfo.subresourceRange.layerCount = 1;
				if (vkCreateImageView(m_device, &layerViewInfo, m_allocationCallbacks, &m_layerViews[layer]) != VK_SUCCESS)
				{
					LOG_ERROR("Vulkan", "VulkanTexture::create(): layer view {} creation failed!", layer);
					destroy();
					return false;
				}
			}
		}
		else
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_vkFormat;
			const bool isDepthFormat = gfx::hasFlag(info.usage, gfx::TextureUsage::DepthStencil);
			viewInfo.subresourceRange.aspectMask = isDepthFormat
				? VK_IMAGE_ASPECT_DEPTH_BIT
				: VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.levelCount = m_mipLevels;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_device, &viewInfo, m_allocationCallbacks, &m_imageView) != VK_SUCCESS)
			{
				LOG_ERROR("Vulkan", "vkCreateImageView (texture) failed");
				destroy();
				return false;
			}
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

		for (VkImageView v : m_layerViews)
			if (v != VK_NULL_HANDLE) vkDestroyImageView(m_device, v, m_allocationCallbacks);
		m_layerViews.clear();

		if (m_arrayView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_device, m_arrayView, m_allocationCallbacks);
			m_arrayView = VK_NULL_HANDLE;
		}
	}

	gfx::TextureFormat VulkanTexture::format() const
	{
		return toGfxFormat(m_vkFormat);
	}	
}

#endif
