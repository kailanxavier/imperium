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


		[[nodiscard]] virtual u32 layer() const { return 0; }

		u32 width() const override;
		u32 height() const override;
		gfx::TextureFormat format() const override;

		gfx::SampleCount sampleCount() const override;

		[[nodiscard]] virtual bool isSampledOwnedDepth() const;
		[[nodiscard]] virtual VkImage image() const;
		[[nodiscard]] virtual VkImageView imageView() const;
		[[nodiscard]] virtual VkFormat vkFormat() const;
		[[nodiscard]] virtual VulkanRenderTargetKind kind() const { return m_kind; }

	private:
		VulkanSwapchain* m_swapchain = nullptr;
		VulkanTexture* m_ownedTexture = nullptr;
		VulkanRenderTargetKind m_kind;
	};

	class VulkanCascadeLayerTarget final : public VulkanRenderTarget
	{
	public:
		VulkanCascadeLayerTarget(std::shared_ptr<VulkanTexture> owner, u32 layer) 
			: VulkanRenderTarget(*owner), m_owner(std::move(owner)), m_layer(layer) {}

		[[nodiscard]] u32 layer() const override { return m_layer; }
		[[nodiscard]] VkImage image() const override { return m_owner->image(); }
		[[nodiscard]] VkImageView imageView() const override { return m_owner->layerView(m_layer); }
		[[nodiscard]] bool isSampledOwnedDepth() const override { return true; }

	private:
		std::shared_ptr<VulkanTexture> m_owner;
		u32 m_layer;
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
