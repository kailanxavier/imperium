#if defined(IMP_GFX_VULKAN)

#include "vk_command_list.h"
#include "vk_render_target.h"
#include "vk_pipeline.h"
#include "vk_buffer.h"
#include "vk_desc_alloc.h"
#include "vk_texture.h"
#include "vk_sampler.h"
#include "vk_debug_utils.h"
#include "vk_accel_structure.h"
#include <core/log/log.h>

#include <algorithm>

namespace imp::gfx::vulkan
{
	void VulkanCommandList::reset(VkDevice device, VkCommandBuffer cmd, VulkanDescriptorAllocator* descriptorAllocator, u32 frameIndex)
	{
		const bool barriersFlushed = m_pendingImageBarriers.empty() && m_pendingMemoryBarriers.empty();
		if (!barriersFlushed)
			LOG_ERROR("Vulkan", "VulkanCommandList::reset() called with {} unflushed image barrier(s) and {} unflushed memory barrier(s) from the previous recording.",
				m_pendingImageBarriers.size(), m_pendingMemoryBarriers.size());

		m_pendingImageBarriers.clear();
		m_pendingMemoryBarriers.clear();

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

		auto* depthTarget = dynamic_cast<VulkanRenderTarget*>(desc.depthTarget);
		auto* resolveTarget = dynamic_cast<VulkanRenderTarget*>(desc.resolveTarget);

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

		// BE CAREFUL WITH THIS. I DON'T LIKE HOW SMALL IT IS *******************
		flushBarriers();

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

		flushBarriers();
	}

	void VulkanCommandList::bindPipeline(gfx::IPipeline& pipeline)
	{
		const auto& vkPipeline = dynamic_cast<VulkanGraphicsPipeline&>(pipeline);
		vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline.pipeline());
		m_currentBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		m_currentPipelineLayout = vkPipeline.layout();
		m_currentDescriptorSetLayout = vkPipeline.descriptorSetLayout();
		m_currentPushConstantStageFlags = vkPipeline.pushConstantStageFlags();
		m_currentBindingLayout = &vkPipeline.bindingLayout();
		m_currentDescriptorSet = VK_NULL_HANDLE;
		m_pendingBindings.clear();
	}

	void VulkanCommandList::bindComputePipeline(gfx::IPipeline& pipeline)
	{
		const auto& vkPipeline = dynamic_cast<VulkanComputePipeline&>( pipeline );
		vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline.pipeline());
		m_currentBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
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
		auto& vkTexture = dynamic_cast<VulkanTexture&>( texture );
		const auto& vkSampler = dynamic_cast<VulkanSampler&>( sampler );

		ensureReadableForSampling(vkTexture);

		PendingBinding pb{};
		pb.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pb.binding = binding;
		pb.imageView = vkTexture.imageView();
		pb.sampler = vkSampler.handle();
		setPendingBinding(pb);
	}

	void VulkanCommandList::pushConstants(const void* data, u32 size, u32 offset)
	{
		if (m_currentBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE)
		{
			vkCmdPushConstants(m_cmd, m_currentPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, offset, size, data);
			return;
		}
		vkCmdPushConstants(m_cmd, m_currentPipelineLayout, m_currentPushConstantStageFlags, offset, size, data);
	}

	void VulkanCommandList::draw(u32 vertexCount, u32 instanceCount)
	{
		flushBarriers();
		flushDescriptorBindings();
		vkCmdDraw(m_cmd, vertexCount, instanceCount, 0, 0);
	}

	void VulkanCommandList::drawIndexed(u32 indexCount, u32 instanceCount, u32 firstInstance)
	{
		flushBarriers();
		flushDescriptorBindings();
		vkCmdDrawIndexed(m_cmd, indexCount, instanceCount, 0, 0, firstInstance);
	}

	void VulkanCommandList::bindStorageImage(gfx::ITexture& texture, u32 binding)
	{
		const auto& vkTexture = dynamic_cast<VulkanTexture&>( texture );
		transitionImage(vkTexture.image(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);

		PendingBinding pb{};
		pb.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		pb.binding = binding;
		pb.imageView = vkTexture.imageView();
		pb.sampler = VK_NULL_HANDLE;
		pb.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		setPendingBinding(pb);
	}

	void VulkanCommandList::bindStorageBuffer(gfx::IBuffer& buffer, u32 binding)
	{
		const auto& vkBuffer = dynamic_cast<VulkanBuffer&>( buffer );

		PendingBinding pb{};
		pb.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		pb.binding = binding;
		pb.buffer = vkBuffer.handle();
		pb.range = vkBuffer.size();
		setPendingBinding(pb);
	}

	void VulkanCommandList::bindAccelerationStructure(const gfx::ITlas& tlas, u32 binding)
	{
		const auto& vkTlas = dynamic_cast<const VulkanTlas&>( tlas );

		PendingBinding pb{};
		pb.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		pb.binding = binding;
		pb.accelStruct = vkTlas.handle();
		setPendingBinding(pb);
	}

	void VulkanCommandList::dispatch(u32 groupCountX, u32 groupCountY, u32 groupCountZ)
	{
		flushBarriers();
		flushDescriptorBindings();
		vkCmdDispatch(m_cmd, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanCommandList::transitionToPresent(gfx::IRenderTarget& target)
	{
		const auto& vkTarget = dynamic_cast<VulkanRenderTarget&>( target );
		transitionImage(vkTarget.image(), VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE);
		flushBarriers();
	}

	void VulkanCommandList::computeToComputeBarrier()
	{
		VkMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;

		m_pendingMemoryBarriers.push_back(barrier);
	}

	void VulkanCommandList::computeToGraphicsBarrier()
	{
		VkMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		m_pendingMemoryBarriers.push_back(barrier);
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

			std::vector<VkAccelerationStructureKHR> accelHandles;
			std::vector<VkWriteDescriptorSetAccelerationStructureKHR> accelInfos;
			accelHandles.reserve(m_pendingBindings.size());
			accelInfos.reserve(m_pendingBindings.size());

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

				if (pb.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || pb.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
				{
					bufferInfos.push_back({ pb.buffer, 0, pb.range });
					write.pBufferInfo = &bufferInfos.back();
				}
				else if (pb.type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
				{
					accelHandles.push_back(pb.accelStruct);
					VkWriteDescriptorSetAccelerationStructureKHR accelWrite{};
					accelWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
					accelWrite.accelerationStructureCount = 1;
					accelWrite.pAccelerationStructures = &accelHandles.back();
					accelInfos.push_back(accelWrite);

					write.pNext = &accelInfos.back();
				}
				else
				{
					imageInfos.push_back({ pb.sampler, pb.imageView, pb.imageLayout });
					write.pImageInfo = &imageInfos.back();
				}

				writes.push_back(write);
			}

			vkUpdateDescriptorSets(m_device, static_cast<u32>( writes.size() ), writes.data(), 0, nullptr);
			m_descriptorSetCache.emplace(key, set);
		}

		if (set != m_currentDescriptorSet)
		{
			vkCmdBindDescriptorSets(m_cmd, m_currentBindPoint,
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
			mix(reinterpret_cast<u64>( pb.accelStruct ));
		}
		return hash;
	}

	bool VulkanCommandList::validatePendingBindings() const
	{
#ifndef NDEBUG

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
#else
		return true; // probably fine
#endif
	}

	void VulkanCommandList::ensureReadableForSampling(VulkanTexture& texture)
	{
		const bool isDepth = ( texture.format() == gfx::TextureFormat::Depth32Float );
		const VkImageAspectFlags aspect = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		const u32 layerCount = texture.arrayLayers() > 0 ? texture.arrayLayers() : 1;

		for (u32 layer = 0; layer < layerCount; ++layer)
		{
			ImageSyncState& state = m_imageStates[{texture.image(), layer}];
			if (state.layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				continue;

			transitionImage(texture.image(), aspect, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT, false, layer);
		}
	}

	namespace
	{
		constexpr VkAccessFlags2 kWriteAccessMask =
			VK_ACCESS_2_SHADER_WRITE_BIT
			| VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT
			| VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
			| VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
			| VK_ACCESS_2_TRANSFER_WRITE_BIT
			| VK_ACCESS_2_HOST_WRITE_BIT
			| VK_ACCESS_2_MEMORY_WRITE_BIT
			| VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
	}

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

		const bool sameLayout = !crossesPresentationEngine && ( state.layout == newLayout );
		const bool bothReadOnly = ( ( srcAccess & kWriteAccessMask ) == 0 ) && ( ( dstAccess & kWriteAccessMask ) == 0 );

		if (sameLayout && bothReadOnly)
		{
			state.stage |= dstStage;
			state.access |= dstAccess;
			return;
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

		m_pendingImageBarriers.push_back(barrier);

		state.layout = newLayout;
		state.stage = dstStage;
		state.access = dstAccess;
	}

	void VulkanCommandList::flushBarriers()
	{
		if (m_pendingImageBarriers.empty() && m_pendingMemoryBarriers.empty())
			return;

		VkDependencyInfo dep{};
		dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dep.memoryBarrierCount = static_cast<u32>( m_pendingMemoryBarriers.size() );
		dep.pMemoryBarriers = m_pendingMemoryBarriers.empty() ? nullptr : m_pendingMemoryBarriers.data();
		dep.imageMemoryBarrierCount = static_cast<u32>( m_pendingImageBarriers.size() );
		dep.pImageMemoryBarriers = m_pendingImageBarriers.empty() ? nullptr : m_pendingImageBarriers.data();

		vkCmdPipelineBarrier2(m_cmd, &dep);

		m_pendingImageBarriers.clear();
		m_pendingMemoryBarriers.clear();
	}
}

#endif
