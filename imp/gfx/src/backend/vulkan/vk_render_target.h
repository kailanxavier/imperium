#pragma once

#include <gfx/resources.h>
#include <vulkan/vulkan.h>
#include "vk_texture.h"

#include <core/types/int_types.h>

#include <memory>

namespace imp::gfx::vulkan
{
	class VulkanSwapchain;

	enum class VulkanRenderTargetKind { Colour, Depth, OwnedTexture };

	gfx::TextureFormat toGfxFormat(VkFormat format);
	VkFormat toVkFormat(gfx::TextureFormat format);

	class VulkanRenderTarget : public gfx::IRenderTarget
	{
	public:
		VulkanRenderTarget(VulkanSwapchain& swapchain, VulkanRenderTargetKind kind)
			: m_swapchain(&swapchain), m_kind(kind) {}

		VulkanRenderTarget(VulkanTexture& ownedTexture)
			: m_ownedTexture(&ownedTexture)
			, m_kind(VulkanRenderTargetKind::OwnedTexture) {}

		u32 width() const override;
		u32 height() const override;
		gfx::TextureFormat format() const override;

		gfx::SampleCount sampleCount() const override;
		[[nodiscard]] bool isSampledOwnedDepth() const;

		VkImage image() const;
		VkImageView imageView() const;
		VkFormat vkFormat() const;

		[[nodiscard]] VulkanRenderTargetKind kind() const { return m_kind; }

	private:
		VulkanSwapchain* m_swapchain = nullptr;
		VulkanTexture* m_ownedTexture = nullptr;
		VulkanRenderTargetKind m_kind;
	};

	class VulkanOwnedColourTarget final : public VulkanRenderTarget
	{
	public:
		explicit VulkanOwnedColourTarget(std::shared_ptr<VulkanTexture> texture)
			: VulkanRenderTarget(*texture), m_texture(std::move(texture)) {}

		[[nodiscard]] gfx::ITexture* asTexture() override { return m_texture.get(); }

	private:
		std::shared_ptr<VulkanTexture> m_texture;
	};
}
