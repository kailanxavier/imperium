#pragma once

#include <gfx/commands.h>
#include <vulkan/vulkan.h>

namespace imp::gfx::vulkan
{
	class VulkanRenderTarget;
	class VulkanCommandList final : public gfx::ICommandList
	{
	public:
		void reset(VkCommandBuffer cmd);

		void beginRenderPass(const gfx::RenderPassDesc& desc) override;
		void endRenderPass() override;

		void bindPipeline(gfx::IPipeline& pipeline) override;
		void bindVertexBuffer(gfx::IBuffer& buffer) override;
		void bindIndexBuffer(gfx::IBuffer& buffer) override;

		void pushConstants(const void* data, u32 size, u32 offset) override;

		void draw(u32 vertexCount, u32 instanceCount) override;
		void drawIndexed(u32 indexCount, u32 instanceCount) override;

		VkCommandBuffer commandBuffer() const { return m_cmd; }

	private:
		VkCommandBuffer m_cmd = VK_NULL_HANDLE;
		VkPipelineLayout m_currentPipelineLayout = VK_NULL_HANDLE;
		
		VulkanRenderTarget* m_colourTarget = nullptr;
	};
}
