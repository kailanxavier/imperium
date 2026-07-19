#if defined(IMP_GFX_VULKAN)

#include "vk_render_target.h"
#include "vk_swapchain.h"

namespace imp::gfx::vulkan
{
	gfx::TextureFormat toGfxFormat(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_B8G8R8A8_SRGB: return gfx::TextureFormat::BGRA8Srgb;
		case VK_FORMAT_R8G8B8A8_SRGB: return gfx::TextureFormat::RGBA8Srgb;
		case VK_FORMAT_R8G8B8A8_UNORM: return gfx::TextureFormat::RGBA8Unorm;
		case VK_FORMAT_D32_SFLOAT: return gfx::TextureFormat::Depth32Float;
		default: return gfx::TextureFormat::Unknown;
		}
	}

	VkFormat toVkFormat(gfx::TextureFormat format)
	{
		switch (format)
		{
		case gfx::TextureFormat::BGRA8Srgb: return VK_FORMAT_B8G8R8A8_SRGB;
		case gfx::TextureFormat::RGBA8Srgb: return VK_FORMAT_R8G8B8A8_SRGB;
		case gfx::TextureFormat::RGBA8Unorm: return VK_FORMAT_R8G8B8A8_UNORM;
		case gfx::TextureFormat::Depth32Float: return VK_FORMAT_D32_SFLOAT;
		default: return VK_FORMAT_UNDEFINED;
		}
	}

	u32 VulkanRenderTarget::width() const { return m_swapchain.extent().width; }
	u32 VulkanRenderTarget::height() const { return m_swapchain.extent().height; }

	gfx::TextureFormat VulkanRenderTarget::format() const
	{
		return toGfxFormat(m_kind == VulkanRenderTargetKind::Colour ? m_swapchain.imageFormat() : m_swapchain.depthFormat());
	}

	VkImage VulkanRenderTarget::image() const
	{
		return m_kind == VulkanRenderTargetKind::Colour ? m_swapchain.currentImage() : m_swapchain.depthImage();
	}

	VkImageView VulkanRenderTarget::imageView() const
	{
		return m_kind == VulkanRenderTargetKind::Colour ? m_swapchain.currentImageView() : m_swapchain.depthImageView();
	}

	VkFormat VulkanRenderTarget::vkFormat() const
	{
		return m_kind == VulkanRenderTargetKind::Colour ? m_swapchain.imageFormat() : m_swapchain.depthFormat();
	}
}

#endif
