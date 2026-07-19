#pragma once

#include <gfx/resources.h>
#include <vulkan/vulkan.h>

#include <core/memory/int_types.h>

namespace imp::gfx::vulkan
{
	class VulkanSwapchain;
	enum class VulkanRenderTargetKind { Colour, Depth };

	gfx::TextureFormat toGfxFormat(VkFormat format);
	VkFormat toVkFormat(gfx::TextureFormat format);

	class VulkanRenderTarget final : public gfx::IRenderTarget
	{
	public:
		VulkanRenderTarget(VulkanSwapchain& swapchain, VulkanRenderTargetKind kind)
			: m_swapchain(swapchain), m_kind(kind) {
		}

		u32 width() const override;
		u32 height() const override;
		gfx::TextureFormat format() const override;

		VkImage image() const;
		VkImageView imageView() const;
		VkFormat vkFormat() const;

	private:
		VulkanSwapchain& m_swapchain;
		VulkanRenderTargetKind m_kind;
	};
}
