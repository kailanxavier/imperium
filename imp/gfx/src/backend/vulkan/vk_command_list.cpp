#if defined(IMP_GFX_VULKAN)

#include "vk_command_list.h"
#include "vk_render_target.h"
#include "vk_pipeline.h"
#include "vk_buffer.h"
#include "vk_desc_alloc.h"
#include "vk_texture.h"
#include "vk_sampler.h"

namespace imp::gfx::vulkan
{
	void VulkanCommandList::reset(VkDevice device, VkCommandBuffer cmd, VulkanDescriptorAllocator* descriptorAllocator, u32 frameIndex)
	{
		m_cmd = cmd;
		m_device = device;
		m_currentPipelineLayout = VK_NULL_HANDLE;
		m_currentDescriptorSetLayout = VK_NULL_HANDLE;
		m_currentDescriptorSet = VK_NULL_HANDLE;
		m_colourTarget = nullptr;
		m_descriptorAllocator = descriptorAllocator;
		m_frameIndex = frameIndex;
	}

	void VulkanCommandList::beginRenderPass(const gfx::RenderPassDesc& desc)
	{
		auto* colourTarget = static_cast<VulkanRenderTarget*>( desc.colourTarget );
		auto* depthTarget = static_cast<VulkanRenderTarget*>( desc.depthTarget );

		VkImageSubresourceRange colourRange{};
		colourRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colourRange.levelCount = 1;
		colourRange.layerCount = 1;

		VkImageMemoryBarrier2 toColourAttachment{};
		toColourAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		toColourAttachment.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		toColourAttachment.srcAccessMask = VK_ACCESS_2_NONE;
		toColourAttachment.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		toColourAttachment.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		toColourAttachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		toColourAttachment.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		toColourAttachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toColourAttachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toColourAttachment.image = colourTarget->image();
		toColourAttachment.subresourceRange = colourRange;

		VkDependencyInfo toColourDep{};
		toColourDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		toColourDep.imageMemoryBarrierCount = 1;
		toColourDep.pImageMemoryBarriers = &toColourAttachment;
		vkCmdPipelineBarrier2(m_cmd, &toColourDep);

		if (depthTarget)
		{
			VkImageSubresourceRange depthRange{};
			depthRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			depthRange.levelCount = 1;
			depthRange.layerCount = 1;

			VkImageMemoryBarrier2 toDepthAttachment{};
			toDepthAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			toDepthAttachment.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			toDepthAttachment.srcAccessMask = VK_ACCESS_2_NONE;
			toDepthAttachment.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			toDepthAttachment.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			toDepthAttachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			toDepthAttachment.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			toDepthAttachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toDepthAttachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toDepthAttachment.image = depthTarget->image();
			toDepthAttachment.subresourceRange = depthRange;

			VkDependencyInfo toDepthDep{};
			toDepthDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			toDepthDep.imageMemoryBarrierCount = 1;
			toDepthDep.pImageMemoryBarriers = &toDepthAttachment;
			vkCmdPipelineBarrier2(m_cmd, &toDepthDep);
		}

		VkRenderingAttachmentInfo colourAttachment{};
		colourAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colourAttachment.imageView = colourTarget->imageView();
		colourAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colourAttachment.loadOp = desc.clearColour ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colourAttachment.clearValue.color = {
			{ desc.clearColourValue.r, desc.clearColourValue.g, desc.clearColourValue.b, desc.clearColourValue.a }
		};

		VkRenderingAttachmentInfo depthAttachment{};
		if (depthTarget)
		{
			depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachment.imageView = depthTarget->imageView();
			depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			depthAttachment.loadOp = desc.clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.clearValue.depthStencil.depth = desc.clearDepthValue;
		}

		VkExtent2D extent{ colourTarget->width(), colourTarget->height() };

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = { {0,0}, extent };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colourAttachment;
		if (depthTarget) renderingInfo.pDepthAttachment = &depthAttachment;

		vkCmdBeginRendering(m_cmd, &renderingInfo);

		// TODO:
		// y = height, height = -height: Vulkan's NDC is natively
		// Y-down, so a "normal" positive-height viewport combined
		// with this engine's Y-up LH math library renders
		// everything upside-down
		//
		// Side effect worth remembering in the future here
		// is that when culling gets turned on, this flip
		// also reverses the effective winding order the
		// rasterizer sees, so whichever frontFace looks correct
		// without culling will need to be inverted once
		// VK_CULL_MODE_BACK_BIT is enabled

		VkViewport viewport{};
		viewport.x = 0.f;
		viewport.y = static_cast<float>( extent.height );;
		viewport.width = static_cast<float>( extent.width );
		viewport.height = -static_cast<float>( extent.height );
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;
		vkCmdSetViewport(m_cmd, 0, 1, &viewport);

		VkRect2D scissor{ { 0, 0 }, extent };
		vkCmdSetScissor(m_cmd, 0, 1, &scissor);

		m_colourTarget = colourTarget;
	}

	void VulkanCommandList::endRenderPass()
	{
		vkCmdEndRendering(m_cmd);

		if (m_colourTarget)
		{
			VkImageSubresourceRange colourRange{};
			colourRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			colourRange.levelCount = 1;
			colourRange.layerCount = 1;

			VkImageMemoryBarrier2 toPresent{};
			toPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			toPresent.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			toPresent.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			toPresent.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
			toPresent.dstAccessMask = VK_ACCESS_2_NONE;
			toPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toPresent.image = m_colourTarget->image();
			toPresent.subresourceRange = colourRange;

			VkDependencyInfo toPresentDep{};
			toPresentDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			toPresentDep.imageMemoryBarrierCount = 1;
			toPresentDep.pImageMemoryBarriers = &toPresent;
			vkCmdPipelineBarrier2(m_cmd, &toPresentDep);
		}

		m_colourTarget = nullptr;
	}

	void VulkanCommandList::bindPipeline(gfx::IPipeline& pipeline)
	{
		auto& vkPipeline = static_cast<VulkanGraphicsPipeline&>( pipeline );
		vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline.pipeline());
		m_currentPipelineLayout = vkPipeline.layout();
		m_currentDescriptorSetLayout = vkPipeline.descriptorSetLayout();
		m_currentDescriptorSet = VK_NULL_HANDLE;
	}

	void VulkanCommandList::bindVertexBuffer(gfx::IBuffer& buffer)
	{
		auto& vkBuffer = static_cast<VulkanBuffer&>( buffer );
		VkBuffer buffers[] = { vkBuffer.handle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_cmd, 0, 1, buffers, offsets);
	}

	void VulkanCommandList::bindIndexBuffer(gfx::IBuffer& buffer)
	{
		// TODO: Always UINT16
		auto& vkBuffer = static_cast<VulkanBuffer&>( buffer );
		vkCmdBindIndexBuffer(m_cmd, vkBuffer.handle(), 0, VK_INDEX_TYPE_UINT16);
	}

	void VulkanCommandList::bindUniformBuffer(gfx::IBuffer& buffer, u32 binding)
	{
		if (!ensureDescriptorSet())
			return;

		auto& vkBuffer = static_cast<VulkanBuffer&>( buffer );

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = vkBuffer.handle();
		bufferInfo.offset = 0;
		bufferInfo.range = vkBuffer.size();

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = m_currentDescriptorSet;
		write.dstBinding = binding;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
	}

	void VulkanCommandList::bindTexture(gfx::ITexture& texture, gfx::ISampler& sampler, u32 binding)
	{
		if (!ensureDescriptorSet())
			return;

		auto& vkTexture = static_cast<VulkanTexture&>( texture );
		auto& vkSampler = static_cast<VulkanSampler&>( sampler );

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = vkTexture.imageView();
		imageInfo.sampler = vkSampler.handle();

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = m_currentDescriptorSet;
		write.dstBinding = binding;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
	}

	void VulkanCommandList::pushConstants(const void* data, u32 size, u32 offset)
	{
		// TODO: Always VK_SHADER_STAGE_VERTEX_BIT
		// My simpleton mind was not aware of the implications when I wrote it
		vkCmdPushConstants(m_cmd, m_currentPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, offset, size, data);
	}

	void VulkanCommandList::draw(u32 vertexCount, u32 instanceCount)
	{
		vkCmdDraw(m_cmd, vertexCount, instanceCount, 0, 0);
	}

	void VulkanCommandList::drawIndexed(u32 indexCount, u32 instanceCount)
	{
		vkCmdDrawIndexed(m_cmd, indexCount, instanceCount, 0, 0, 0);
	}

	bool VulkanCommandList::ensureDescriptorSet()
	{
		if (m_currentDescriptorSet != VK_NULL_HANDLE)
			return true;

		if (m_descriptorAllocator || m_currentDescriptorSetLayout == VK_NULL_HANDLE)
		{
			// This should only get triggered if the caller did something
			// wrong, so we don't need to handle this here.
			return false;
		}

		m_currentDescriptorSet = m_descriptorAllocator->allocate(m_frameIndex, m_currentDescriptorSetLayout);
		if (m_currentDescriptorSet == VK_NULL_HANDLE)
			return false;

		vkCmdBindDescriptorSets(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_currentPipelineLayout, 0, 1, &m_currentDescriptorSet, 0, nullptr);

		return true;
	}
}

#endif
