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
		m_depthTarget = nullptr;

		m_descriptorAllocator = descriptorAllocator;
		m_frameIndex = frameIndex;
	}

	void VulkanCommandList::beginRenderPass(const gfx::RenderPassDesc& desc)
	{
		auto* colourTarget = static_cast<VulkanRenderTarget*>( desc.colourTarget );
		auto* depthTarget = static_cast<VulkanRenderTarget*>( desc.depthTarget );

		if (colourTarget)
		{
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
		}

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
		if (colourTarget)
		{
			colourAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colourAttachment.imageView = colourTarget->imageView();
			colourAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colourAttachment.loadOp = desc.clearColour ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colourAttachment.clearValue.color = {
				{ desc.clearColourValue.r, desc.clearColourValue.g, desc.clearColourValue.b, desc.clearColourValue.a }
			};
		}

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

		VulkanRenderTarget* extentSource = colourTarget ? colourTarget : depthTarget;
		VkExtent2D extent{ extentSource->width(), extentSource->height() };

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = { {0,0}, extent };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = colourTarget ? 1 : 0;
		renderingInfo.pColorAttachments = colourTarget ? &colourAttachment : nullptr;
		if (depthTarget) renderingInfo.pDepthAttachment = &depthAttachment;

		vkCmdBeginRendering(m_cmd, &renderingInfo);

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
		m_depthTarget = depthTarget;
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

			const bool isSwapchainTarget = ( m_colourTarget->kind() != VulkanRenderTargetKind::OwnedTexture );

			VkImageMemoryBarrier2 toNext{};
			toNext.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			toNext.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			toNext.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			toNext.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			toNext.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toNext.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toNext.image = m_colourTarget->image();
			toNext.subresourceRange = colourRange;

			if (isSwapchainTarget)
			{
				toNext.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
				toNext.dstAccessMask = VK_ACCESS_2_NONE;
				toNext.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			}
			else
			{
				toNext.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
				toNext.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
				toNext.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			VkDependencyInfo dep{};
			dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dep.imageMemoryBarrierCount = 1;
			dep.pImageMemoryBarriers = &toNext;
			vkCmdPipelineBarrier2(m_cmd, &dep);
		}

		if (m_depthTarget)
		{
			VkImageSubresourceRange depthRange{};
			depthRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			depthRange.levelCount = 1;
			depthRange.layerCount = 1;

			const bool isSwapchainDepth = (m_depthTarget->kind() != VulkanRenderTargetKind::OwnedTexture);

			VkImageMemoryBarrier2 toNext{};
			toNext.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			toNext.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			toNext.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			toNext.oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			toNext.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toNext.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toNext.image = m_depthTarget->image();
			toNext.subresourceRange = depthRange;

			if (isSwapchainDepth)
			{
				// main scene depth buffer isn't sampled
				toNext.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
				toNext.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				toNext.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			}
			else
			{
				// owned depth texture, about to be sampled in the fragment shader
				toNext.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
				toNext.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
				toNext.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			VkDependencyInfo dep{};
			dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dep.imageMemoryBarrierCount = 1;
			dep.pImageMemoryBarriers = &toNext;
			vkCmdPipelineBarrier2(m_cmd, &dep);
		}

		m_depthTarget = nullptr;
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

	void VulkanCommandList::bindVertexBuffer(gfx::IBuffer& buffer, u32 binding)
	{
		auto& vkBuffer = static_cast<VulkanBuffer&>( buffer );
		VkBuffer buffers[] = { vkBuffer.handle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_cmd, binding, 1, buffers, offsets);
	}

	void VulkanCommandList::bindIndexBuffer(gfx::IBuffer& buffer)
	{
		auto& vkBuffer = static_cast<VulkanBuffer&>( buffer );
		const VkIndexType indexType = ( vkBuffer.indexFormat() == gfx::IndexFormat::Uint32
			? VK_INDEX_TYPE_UINT32
			: VK_INDEX_TYPE_UINT16
			);

		vkCmdBindIndexBuffer(m_cmd, vkBuffer.handle(), 0, indexType);
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
		m_currentDescriptorSet = VK_NULL_HANDLE;
	}

	void VulkanCommandList::drawIndexed(u32 indexCount, u32 instanceCount, u32 firstInstance)
	{
		vkCmdDrawIndexed(m_cmd, indexCount, instanceCount, 0, 0, firstInstance);
		m_currentDescriptorSet = VK_NULL_HANDLE;
	}

	bool VulkanCommandList::ensureDescriptorSet()
	{
		if (m_currentDescriptorSet != VK_NULL_HANDLE)
			return true;

		if (!m_descriptorAllocator || m_currentDescriptorSetLayout == VK_NULL_HANDLE)
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
