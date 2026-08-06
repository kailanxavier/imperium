#pragma once

#include <gfx/commands.h>
#include <vulkan/vulkan.h>
#include <unordered_map>

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

		VkCommandBuffer commandBuffer() const { return m_cmd; }

		void transitionToPresent(gfx::IRenderTarget& target);
		void forgetImageState(VkImage image) { m_imageStates.erase(image); }

	private:
		// Allocated m_currentDescriptorSet from m_descriptorAllocator
		// if not already done for the pipeline currently bound.
		// Returns false and logs if there's no descriptor allocator/layout
		// to allocate against.
		bool ensureDescriptorSet();

		struct ImageSyncState
		{
			VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			VkAccessFlags2 access = VK_ACCESS_NONE;
		};
		std::unordered_map<VkImage, ImageSyncState> m_imageStates;

		void transitionImage(VkImage image, VkImageAspectFlags aspect,
			VkImageLayout newLayout, VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess, 
			bool crossesPresentationEngine = false);

		VkCommandBuffer m_cmd = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		VkPipelineLayout m_currentPipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_currentDescriptorSetLayout = VK_NULL_HANDLE;

		VulkanRenderTarget* m_colourTarget = nullptr;
		VulkanRenderTarget* m_depthTarget = nullptr;
		VulkanRenderTarget* m_resolveTarget = nullptr;

		VulkanDescriptorAllocator* m_descriptorAllocator = nullptr;
		u32 m_frameIndex = 0;

		VkDescriptorSet m_currentDescriptorSet = VK_NULL_HANDLE;
	};
}
