#if defined(IMP_GFX_VULKAN)

#include "vk_command_list.h"
#include "vk_render_target.h"
#include "vk_pipeline.h"
#include "vk_buffer.h"
#include "vk_desc_alloc.h"
#include "vk_texture.h"
#include "vk_sampler.h"
#include "vk_debug_utils.h"
#include <core/log/log.h>

#include <algorithm>

namespace imp::gfx::vulkan
{
	void VulkanCommandList::reset(VkDevice device, VkCommandBuffer cmd, VulkanDescriptorAllocator* descriptorAllocator, u32 frameIndex)
	{
		m_cmd = cmd;
		m_device = device;
		m_currentPipelineLayout = VK_NULL_HANDLE;
		m_currentDescriptorSetLayout = VK_NULL_HANDLE;
		m_currentDescriptorSet = VK_NULL_HANDLE;

		for (auto& target : m_colourTargets)
			target = nullptr;
		m_colourTargetCount = 0;

		m_depthTarget = nullptr;
		m_resolveTarget = nullptr;

		m_descriptorAllocator = descriptorAllocator;
		m_frameIndex = frameIndex;

		m_pendingBindings.clear();
		m_descriptorSetCache.clear();

		// NOTE: do NOT reset m_imageStates here, it must survive
		// across frames to do its job.
	}

	void VulkanCommandList::beginRenderPass(const gfx::RenderPassDesc& desc)
	{
		VulkanRenderTarget* colourTargets[gfx::RenderPassDesc::kMaxColourAttachments]{};
		u32 colourTargetCount = 0;

		for (u32 i = 0; i < desc.colourTargetCount; ++i)
			colourTargets[colourTargetCount++] = dynamic_cast<VulkanRenderTarget*>(
				desc.colourTargets[i].target);

		auto* depthTarget = dynamic_cast<VulkanRenderTarget*>( desc.depthTarget );
		auto* resolveTarget = dynamic_cast<VulkanRenderTarget*>( desc.resolveTarget );

		for (u32 i = 0; i < colourTargetCount; ++i)
		{
			VulkanRenderTarget* colourTarget = colourTargets[i];
			if (!colourTarget) 
				continue;

			VkAccessFlags2 dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

			if (!desc.colourTargets[i].clear)
				dstAccess |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT; // loadOp LOAD

			const bool isSwapchainImage = colourTarget->kind() != VulkanRenderTargetKind::OwnedTexture;
			transitionImage(colourTarget->image(), VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, dstAccess, isSwapchainImage);
		}

		if (resolveTarget)
		{
			transitionImage(resolveTarget->image(), VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
		}

		if (depthTarget)
		{
			VkAccessFlags2 dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			if (!desc.clearDepth)
				dstAccess |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT; // same as colour

			const bool isSwapchainImage = depthTarget->kind() != VulkanRenderTargetKind::OwnedTexture;
			transitionImage(depthTarget->image(), VK_IMAGE_ASPECT_DEPTH_BIT,
				VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
				dstAccess, isSwapchainImage, depthTarget->layer());
		}

		VkRenderingAttachmentInfo colourAttachments[gfx::RenderPassDesc::kMaxColourAttachments]{};
		for (u32 i = 0; i < colourTargetCount; ++i)
		{
			VulkanRenderTarget* colourTarget = colourTargets[i];
			if (!colourTarget) 
				continue;

			VkRenderingAttachmentInfo& attachment = colourAttachments[i];

			attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			attachment.imageView = colourTarget->imageView();
			attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachment.loadOp = desc.colourTargets[i].clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			const gfx::ClearColour& clearValue = desc.colourTargets[i].clearValue;
			attachment.clearValue.color = { { clearValue.r, clearValue.g, clearValue.b, clearValue.a } };

			if (i == 0 && resolveTarget)
			{
				attachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
				attachment.resolveImageView = resolveTarget->imageView();
				attachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
		}

		VkRenderingAttachmentInfo depthAttachment{};
		if (depthTarget)
		{
			depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachment.imageView = depthTarget->imageView();
			depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			depthAttachment.loadOp = desc.clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			const bool depthWillBeSampled = depthTarget->isSampledOwned();
			depthAttachment.storeOp = depthWillBeSampled ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.clearValue.depthStencil.depth = desc.clearDepthValue;
		}

		VulkanRenderTarget* extentSource = colourTargetCount > 0 ? colourTargets[0] : depthTarget;

		VkExtent2D extent{};
		if (extentSource)
		{
			extent.width = extentSource->width();
			extent.height = extentSource->height();
		}

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = { {0,0}, extent };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = colourTargetCount;
		renderingInfo.pColorAttachments = colourTargetCount > 0 ? colourAttachments : nullptr;

		if (depthTarget) 
			renderingInfo.pDepthAttachment = &depthAttachment;

		vkCmdBeginRendering(m_cmd, &renderingInfo);

		cmdBeginDebugLabel(m_cmd, desc.debugName);

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

		for (u32 i = 0; i < colourTargetCount; ++i)
			m_colourTargets[i] = colourTargets[i];

		m_colourTargetCount = colourTargetCount;
		m_depthTarget = depthTarget;
		m_resolveTarget = resolveTarget;
	}

	void VulkanCommandList::endRenderPass()
	{
		vkCmdEndRendering(m_cmd);
		cmdEndDebugLabel(m_cmd);


		if (m_resolveTarget)
		{
			transitionImage(m_resolveTarget->image(), VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
		}

		for (u32 i = 0; i < m_colourTargetCount; ++i)
		{
			VulkanRenderTarget* colourTarget = m_colourTargets[i];
			if (colourTarget && colourTarget->isSampledOwned())
			{
				transitionImage(colourTarget->image(), VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
			}
		}

		if (m_depthTarget && m_depthTarget->isSampledOwned())
		{
			transitionImage(m_depthTarget->image(), VK_IMAGE_ASPECT_DEPTH_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
				false, m_depthTarget->layer());
		}

		m_depthTarget = nullptr;

		for (u32 i = 0; i < m_colourTargetCount; ++i)
			m_colourTargets[i] = nullptr;
		m_colourTargetCount = 0;

		m_resolveTarget = nullptr;
	}

	void VulkanCommandList::bindPipeline(gfx::IPipeline& pipeline)
	{
		const auto& vkPipeline = dynamic_cast<VulkanGraphicsPipeline&>( pipeline );
		vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline.pipeline());
		m_currentPipelineLayout = vkPipeline.layout();
		m_currentDescriptorSetLayout = vkPipeline.descriptorSetLayout();
		m_currentBindingLayout = &vkPipeline.bindingLayout();
		m_currentDescriptorSet = VK_NULL_HANDLE;
		m_pendingBindings.clear();
	}

	void VulkanCommandList::bindVertexBuffer(gfx::IBuffer& buffer, u32 binding)
	{
		const auto& vkBuffer = dynamic_cast<VulkanBuffer&>( buffer );
		const VkBuffer buffers[] = { vkBuffer.handle() };
		constexpr VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_cmd, binding, 1, buffers, offsets);
	}

	void VulkanCommandList::bindIndexBuffer(gfx::IBuffer& buffer)
	{
		const auto& vkBuffer = dynamic_cast<VulkanBuffer&>( buffer );
		const VkIndexType indexType = ( vkBuffer.indexFormat() == gfx::IndexFormat::Uint32
			? VK_INDEX_TYPE_UINT32
			: VK_INDEX_TYPE_UINT16
			);

		vkCmdBindIndexBuffer(m_cmd, vkBuffer.handle(), 0, indexType);
	}

	void VulkanCommandList::bindUniformBuffer(gfx::IBuffer& buffer, u32 binding)
	{
		const auto& vkBuffer = dynamic_cast<VulkanBuffer&>( buffer );

		PendingBinding pb{};
		pb.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pb.binding = binding;
		pb.buffer = vkBuffer.handle();
		pb.range = vkBuffer.size();
		setPendingBinding(pb);
	}

	void VulkanCommandList::bindTexture(gfx::ITexture& texture, gfx::ISampler& sampler, u32 binding)
	{
		const auto& vkTexture = dynamic_cast<VulkanTexture&>( texture );
		const auto& vkSampler = dynamic_cast<VulkanSampler&>( sampler );

		PendingBinding pb{};
		pb.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pb.binding = binding;
		pb.imageView = vkTexture.imageView();
		pb.sampler = vkSampler.handle();
		setPendingBinding(pb);
	}

	void VulkanCommandList::pushConstants(const void* data, u32 size, u32 offset)
	{
		// TODO: Always VK_SHADER_STAGE_VERTEX_BIT
		// My simpleton mind was not aware of the implications when I wrote it
		vkCmdPushConstants(m_cmd, m_currentPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, offset, size, data);
	}

	void VulkanCommandList::draw(u32 vertexCount, u32 instanceCount)
	{
		flushDescriptorBindings();
		vkCmdDraw(m_cmd, vertexCount, instanceCount, 0, 0);
	}

	void VulkanCommandList::drawIndexed(u32 indexCount, u32 instanceCount, u32 firstInstance)
	{
		flushDescriptorBindings();
		vkCmdDrawIndexed(m_cmd, indexCount, instanceCount, 0, 0, firstInstance);
	}

	void VulkanCommandList::transitionToPresent(gfx::IRenderTarget& target)
	{
		const auto& vkTarget = dynamic_cast<VulkanRenderTarget&>( target );
		transitionImage(vkTarget.image(), VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE);
	}

	void VulkanCommandList::setPendingBinding(const PendingBinding& pb)
	{
		for (PendingBinding& existing : m_pendingBindings)
		{
			if (existing.binding == pb.binding)
			{
				existing = pb;
				return;
			}
		}
		m_pendingBindings.push_back(pb);
	}

	void VulkanCommandList::flushDescriptorBindings()
	{
		if (m_pendingBindings.empty())
			return;

		if (!m_descriptorAllocator || m_currentDescriptorSetLayout == VK_NULL_HANDLE)
		{
			m_pendingBindings.clear();
			return;
		}

#ifndef NDEBUG
		if (!validatePendingBindings())
			LOG_ERROR("Vulkan", "Refusing to flush descriptor bindings due to failure above");
#endif

		std::ranges::sort(m_pendingBindings,
		                  [](const PendingBinding& a, const PendingBinding& b) { return a.binding < b.binding; });

		const u64 key = hashPendingBindings();

		VkDescriptorSet set = VK_NULL_HANDLE;
		auto cached = m_descriptorSetCache.find(key);
		if (cached != m_descriptorSetCache.end())
		{
			set = cached->second;
		}
		else
		{
			set = m_descriptorAllocator->allocate(m_frameIndex, m_currentDescriptorSetLayout);
			if (set == VK_NULL_HANDLE)
			{
				m_pendingBindings.clear();
				return;
			}

			std::vector<VkDescriptorBufferInfo> bufferInfos;
			std::vector<VkDescriptorImageInfo> imageInfos;
			bufferInfos.reserve(m_pendingBindings.size());
			imageInfos.reserve(m_pendingBindings.size());

			std::vector<VkWriteDescriptorSet> writes;
			writes.reserve(m_pendingBindings.size());

			for (const PendingBinding& pb : m_pendingBindings)
			{
				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = set;
				write.dstBinding = pb.binding;
				write.descriptorCount = 1;
				write.descriptorType = pb.type;

				if (pb.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				{
					bufferInfos.push_back({ pb.buffer, 0, pb.range });
					write.pBufferInfo = &bufferInfos.back();
				}
				else
				{
					imageInfos.push_back({ pb.sampler, pb.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
					write.pImageInfo = &imageInfos.back();
				}

				writes.push_back(write);
			}

			vkUpdateDescriptorSets(m_device, static_cast<u32>( writes.size() ), writes.data(), 0, nullptr);
			m_descriptorSetCache.emplace(key, set);
		}

		if (set != m_currentDescriptorSet)
		{
			vkCmdBindDescriptorSets(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_currentPipelineLayout, 0, 1, &set, 0, nullptr);
			m_currentDescriptorSet = set;
		}

		m_pendingBindings.clear();
	}

	u64 VulkanCommandList::hashPendingBindings() const
	{
		u64 hash = 14695981039346656037ull;
		auto mix = [&hash](u64 v)
			{
				hash ^= v;
				hash *= 1099511628211ull;
			};

		mix(reinterpret_cast<u64>( m_currentDescriptorSetLayout )); // sets from different layouts must never collide
		for (const PendingBinding& pb : m_pendingBindings)
		{
			mix(static_cast<u64>( pb.binding ));
			mix(static_cast<u64>( pb.type ));
			mix(reinterpret_cast<u64>( pb.buffer ));
			mix(reinterpret_cast<u64>( pb.imageView ));
			mix(reinterpret_cast<u64>( pb.sampler ));
		}
		return hash;
	}

#ifndef NDEBUG
	bool VulkanCommandList::validatePendingBindings() const
	{
		if (!m_currentBindingLayout)
			return true;

		bool ok = true;

		for (const PendingBinding& pb : m_pendingBindings)
		{
			auto it = m_currentBindingLayout->find(pb.binding);
			if (it == m_currentBindingLayout->end())
			{
				LOG_ERROR("Vulkan",
						"Draw call bound resource at binding {} but the active shader doesn't declare a descriptor there \n{}",
						pb.binding, "(likely a stale or incorrect binding index at the call site)");
				ok = false;
				continue;
			}

			if (it->second.type != pb.type)
			{
				LOG_ERROR("Vulkan",
					"Draw call bound binding {} ('{}') as {} but the shader declares it as {}",
					pb.binding, it->second.name, static_cast<int>( pb.type ), static_cast<int>( it->second.type ));
				ok = false;
			}
		}

		for (const auto& [bindingIndex, info] : *m_currentBindingLayout)
		{
			const bool staged = std::ranges::any_of(m_pendingBindings,
			                                        [bindingIndex](const PendingBinding& pb) { return pb.binding == bindingIndex; });

			if (!staged)
			{
				LOG_ERROR("Vulkan",
					"Shader declares binding {} ('{}') but this draw call never bound anything to it",
					bindingIndex, info.name);
				ok = false;
			}
		}

		return ok;
	}
#endif

	void VulkanCommandList::transitionImage(VkImage image, VkImageAspectFlags aspect, VkImageLayout newLayout,
	                                        VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess, bool crossesPresentationEngine, u32 baseArrayLayer /* = 0*/)
	{
		ImageSyncState& state = m_imageStates[{image, baseArrayLayer}];

		VkPipelineStageFlags2 srcStage = state.stage;
		VkAccessFlags2 srcAccess = state.access;

		if (crossesPresentationEngine)
		{
			srcStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			srcAccess = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
		}

		VkImageMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.srcStageMask = srcStage;
		barrier.srcAccessMask = srcAccess;
		barrier.dstStageMask = dstStage;
		barrier.dstAccessMask = dstAccess;
		barrier.oldLayout = state.layout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange = { aspect, 0, 1, baseArrayLayer, 1 };

		VkDependencyInfo dep{};
		dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dep.imageMemoryBarrierCount = 1;
		dep.pImageMemoryBarriers = &barrier;
		vkCmdPipelineBarrier2(m_cmd, &dep);

		state.layout = newLayout;
		state.stage = dstStage;
		state.access = dstAccess;
	}
}

#endif
