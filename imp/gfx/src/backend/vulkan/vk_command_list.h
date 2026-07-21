#pragma once

#include <gfx/commands.h>
#include <vulkan/vulkan.h>

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
		void bindVertexBuffer(gfx::IBuffer& buffer) override;
		void bindIndexBuffer(gfx::IBuffer& buffer) override;
		void bindUniformBuffer(gfx::IBuffer& buffer, u32 binding) override;
		void bindTexture(gfx::ITexture& texture, gfx::ISampler& sampler, u32 binding) override;

		void pushConstants(const void* data, u32 size, u32 offset) override;

		void draw(u32 vertexCount, u32 instanceCount) override;
		void drawIndexed(u32 indexCount, u32 instanceCount) override;

		VkCommandBuffer commandBuffer() const { return m_cmd; }

	private:
		// Allocated m_currentDescriptorSet from m_descriptorAllocator
		// if not already done for the pipeline currently bound.
		// Returns false and logs if there's no descriptor allocator/layout
		// to allocate against.
		bool ensureDescriptorSet();

		VkCommandBuffer m_cmd = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		VkPipelineLayout m_currentPipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_currentDescriptorSetLayout = VK_NULL_HANDLE;

		VulkanRenderTarget* m_colourTarget = nullptr;

		VulkanDescriptorAllocator* m_descriptorAllocator = nullptr;
		u32 m_frameIndex = 0;

		VkDescriptorSet m_currentDescriptorSet = VK_NULL_HANDLE;
	};
}
