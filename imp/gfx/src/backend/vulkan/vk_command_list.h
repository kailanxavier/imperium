#pragma once

#include <gfx/commands.h>
#include <vulkan/vulkan.h>
#include "vk_pipeline.h"
#include <unordered_map>
#include <vector>

namespace imp::gfx::vulkan
{
	class VulkanRenderTarget;
	class VulkanDescriptorAllocator;

	class VulkanCommandList final : public gfx::ICommandList
	{
	public:
		void reset(VkDevice device, VkCommandBuffer cmd, VulkanDescriptorAllocator* descriptorAllocator, u32 frameIndex);

		void beginRenderPass(const gfx::RenderPassDesc& desc) override;
		void endRenderPass() override;

		void bindPipeline(gfx::IPipeline& pipeline) override;
		void bindVertexBuffer(gfx::IBuffer& buffer, u32 binding) override;
		void bindIndexBuffer(gfx::IBuffer& buffer) override;
		void bindUniformBuffer(gfx::IBuffer& buffer, u32 binding) override;
		void bindTexture(gfx::ITexture& texture, gfx::ISampler& sampler, u32 binding) override;

		void pushConstants(const void* data, u32 size, u32 offset) override;

		void draw(u32 vertexCount, u32 instanceCount) override;
		void drawIndexed(u32 indexCount, u32 instanceCount, u32 firstInstance) override;

		[[nodiscard]] VkCommandBuffer commandBuffer() const { return m_cmd; }

		void transitionToPresent(gfx::IRenderTarget& target);
		void resetImageTracking() { m_imageStates.clear(); }

	private:
		struct ImageStateKey
		{
			VkImage image;
			u32 layer;
			bool operator==(const ImageStateKey& o) const 
			{ return image == o.image && layer == o.layer; }
		};

		struct ImageStateKeyHash
		{
			size_t operator()(const ImageStateKey& k) const
			{
				return std::hash<void*>()( k.image ) ^ ( std::hash<u32>()( k.layer ) << 1 );
			}
		};

		struct ImageSyncState
		{
			VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			VkAccessFlags2 access = VK_ACCESS_NONE;
		};

		struct PendingBinding
		{
			VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			u32 binding = 0;
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceSize range = 0;
			VkImageView imageView = VK_NULL_HANDLE;
			VkSampler sampler = VK_NULL_HANDLE;
		};

		void setPendingBinding(const PendingBinding& pb);
		void flushDescriptorBindings();
		[[nodiscard]] u64 hashPendingBindings() const;
		[[nodiscard]] bool validatePendingBindings() const;

		std::unordered_map<ImageStateKey, ImageSyncState, ImageStateKeyHash> m_imageStates;

		void transitionImage(VkImage image, VkImageAspectFlags aspect,
			VkImageLayout newLayout, VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess, 
			bool crossesPresentationEngine = false, u32 baseArrayLayer = 0);

		VkCommandBuffer m_cmd = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		VkPipelineLayout m_currentPipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_currentDescriptorSetLayout = VK_NULL_HANDLE;

		const std::unordered_map<u32, PipelineBindingInfo>* m_currentBindingLayout = nullptr;

		VulkanRenderTarget* m_colourTarget = nullptr;
		VulkanRenderTarget* m_depthTarget = nullptr;
		VulkanRenderTarget* m_resolveTarget = nullptr;

		VulkanDescriptorAllocator* m_descriptorAllocator = nullptr;
		u32 m_frameIndex = 0;

		VkDescriptorSet m_currentDescriptorSet = VK_NULL_HANDLE;

		std::vector<PendingBinding> m_pendingBindings;
		std::unordered_map<u64, VkDescriptorSet> m_descriptorSetCache;
	};
}
