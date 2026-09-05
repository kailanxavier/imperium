#if defined(IMP_GFX_VULKAN)

#include "vk_render_target.h"
#include "vk_swapchain.h"
#include "vk_texture.h"

namespace imp::gfx::vulkan
{
	gfx::TextureFormat toGfxFormat(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_B8G8R8A8_SRGB: return gfx::TextureFormat::BGRA8Srgb;
		case VK_FORMAT_R8G8B8A8_SRGB: return gfx::TextureFormat::RGBA8Srgb;
		case VK_FORMAT_R8G8B8A8_UNORM: return gfx::TextureFormat::RGBA8Unorm;
		case VK_FORMAT_R16G16B16A16_SFLOAT: return gfx::TextureFormat::RGBA16Float;
		case VK_FORMAT_D32_SFLOAT: return gfx::TextureFormat::Depth32Float;
		case VK_FORMAT_R16G16_SFLOAT: return gfx::TextureFormat::RG16Float;
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
		case gfx::TextureFormat::RGBA16Float: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case gfx::TextureFormat::Depth32Float: return VK_FORMAT_D32_SFLOAT;
		case gfx::TextureFormat::RG16Float: return VK_FORMAT_R16G16_SFLOAT;
		default: return VK_FORMAT_UNDEFINED;
		}
	}

	gfx::SampleCount toGfxSampleCount(VkSampleCountFlagBits count)
	{
		switch (count)
		{
		case VK_SAMPLE_COUNT_1_BIT: return gfx::SampleCount::One;
		case VK_SAMPLE_COUNT_2_BIT: return gfx::SampleCount::Two;
		case VK_SAMPLE_COUNT_4_BIT: return gfx::SampleCount::Four;
		case VK_SAMPLE_COUNT_8_BIT: return gfx::SampleCount::Eight;
		case VK_SAMPLE_COUNT_16_BIT: return gfx::SampleCount::Sixteen;
		default: return gfx::SampleCount::One;
		}
	}

	u32 VulkanRenderTarget::width() const 
	{ 
		if (m_kind == VulkanRenderTargetKind::OwnedTexture) return m_ownedTexture->width();
		return m_swapchain->extent().width;
	}
	u32 VulkanRenderTarget::height() const 
	{ 
		if (m_kind == VulkanRenderTargetKind::OwnedTexture) return m_ownedTexture->height();
		return m_swapchain->extent().height;
	}

	gfx::TextureFormat VulkanRenderTarget::format() const
	{
		if (m_kind == VulkanRenderTargetKind::OwnedTexture) 
			return m_ownedTexture->format();

		return toGfxFormat(m_kind == VulkanRenderTargetKind::Colour 
			? m_swapchain->imageFormat() 
			: m_swapchain->depthFormat());
	}

	gfx::SampleCount VulkanRenderTarget::sampleCount() const
	{
		if (m_kind == VulkanRenderTargetKind::OwnedTexture)
			return toGfxSampleCount(m_ownedTexture->sampleCount());

		return gfx::SampleCount::One;
	}

	bool VulkanRenderTarget::isSampledOwned() const
	{
		return m_kind == VulkanRenderTargetKind::OwnedTexture && m_ownedTexture->isSampled();
	}

	VkImage VulkanRenderTarget::image() const
	{
		if (m_kind == VulkanRenderTargetKind::OwnedTexture) 
			return m_ownedTexture->image();

		return m_kind == VulkanRenderTargetKind::Colour 
			? m_swapchain->currentImage() 
			: m_swapchain->depthImage();
	}

	VkImageView VulkanRenderTarget::imageView() const
	{
		if (m_kind == VulkanRenderTargetKind::OwnedTexture) 
			return m_ownedTexture->imageView();

		return m_kind == VulkanRenderTargetKind::Colour 
			? m_swapchain->currentImageView() 
			: m_swapchain->depthImageView();
	}

	VkFormat VulkanRenderTarget::vkFormat() const
	{
		if (m_kind == VulkanRenderTargetKind::OwnedTexture) 
			return toVkFormat(m_ownedTexture->format());

		return m_kind == VulkanRenderTargetKind::Colour 
			? m_swapchain->imageFormat() 
			: m_swapchain->depthFormat();
	}
}

#endif
