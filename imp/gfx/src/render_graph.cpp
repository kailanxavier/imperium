#include <core/types/int_types.h>
#include <core/log/log.h>

#include "gfx/device.h"

#include "gfx/render_graph.h"
#include "gfx/render_graph_resource_pool.h"

#include <memory>
#include <queue>
#include <cassert>

namespace imp::gfx
{
	RenderGraph::RenderGraph(IDevice& device, RenderGraphResourcePool& pool)
		: m_device(&device), m_pool(&pool) {
	}

	void RenderGraph::recordRead(u32 passIndex, u32 resourceIndex)
	{
		RGPass& pass = m_passes[passIndex];
		pass.reads.push_back(resourceIndex);

		const u32 producer = m_resources[resourceIndex].lastWritePass;
		if (producer != ~0u && producer != passIndex)
			m_edges.emplace_back(producer, passIndex);
	}

	u32 RenderGraph::recordWrite(u32 passIndex, u32 resourceIndex)
	{
		RGPass& pass = m_passes[passIndex];
		pass.writes.push_back(resourceIndex);

		RGResourceDesc& resource = m_resources[resourceIndex];
		resource.lastWritePass = passIndex;
		resource.latestVersion += 1;
		return resource.latestVersion;
	}

	ITexture& RenderGraph::textureOf(u32 resourceIndex)
	{
		RGResourceDesc& r = m_resources[resourceIndex];

		if (r.resolvedTextureOnly)
			return *r.resolvedTextureOnly;

		assert(r.resolvedTarget
			&& "RenderGraph: texture() on an unresolved resource. Was compile() run, and is it actually used by a live pass?");

		ITexture* asTex = r.resolvedTarget->asTexture();
		assert(asTex && "RenderGraph: this render target can't be sampled as a texture");
		return *asTex;
	}

	IRenderTarget& RenderGraph::renderTargetOf(u32 resourceIndex)
	{
		RGResourceDesc& r = m_resources[resourceIndex];
		assert(r.resolvedTarget && "RenderGraph: renderTarget() on a resource with no backing IRenderTarget");
		return *r.resolvedTarget;
	}

	IBuffer& RenderGraph::bufferOf(u32 resourceIndex)
	{
		RGResourceDesc& r = m_resources[resourceIndex];
		assert(r.resolvedBuffer && "RenderGraph: buffer() on an unresolved buffer resource");
		return *r.resolvedBuffer;
	}

	bool RenderGraph::compile()
	{
		const u32 passCount = static_cast<u32>( m_passes.size() );

		std::vector<bool> live(passCount, false);
		for (u32 i{ 0 }; i < passCount; ++i)
			live[i] = m_passes[i].hasSideEffect;

		bool changed = true;
		while (changed)
		{
			changed = false;
			for (const auto& [producer, consumer] : m_edges)
			{
				if (live[consumer] && !live[producer])
				{
					live[producer] = true;
					changed = true;
				}
			}
		}

		for (u32 i{ 0 }; i < passCount; ++i)
			m_passes[i].culled = !live[i];

		// stolen from this fella: 
		// https://dev.to/leopfeiffer/topological-sort-with-kahns-algorithm-3dl1
		// jokes aside awesome article

		std::vector<u32> inDegree(passCount, 0);
		std::vector<std::vector<u32>> adjacency(passCount);
		for (const auto& [producer, consumer] : m_edges)
		{
			if (!live[producer] || !live[consumer])
				continue;
			adjacency[producer].push_back(consumer);
			++inDegree[consumer];
		}

		std::priority_queue<u32, std::vector<u32>, std::greater<u32>> ready;
		for (u32 i{ 0 }; i < passCount; ++i)
			if (live[i] && inDegree[i] == 0)
				ready.push(i);

		m_executionOrder.clear();
		m_executionOrder.reserve(passCount);

		while (!ready.empty())
		{
			const u32 passIndex = ready.top();
			ready.pop();
			m_executionOrder.push_back(passIndex);

			for (u32 next : adjacency[passIndex])
			{
				if (--inDegree[next] == 0)
					ready.push(next);
			}
		}

		const u32 liveCount = static_cast<u32>( std::count(live.begin(), live.end(), true) );
		if (m_executionOrder.size() != liveCount)
		{
			LOG_ERROR("RenderGraph",
				"compile() found a cycle among live passes ({} of {} live passes are schedulable).",
				m_executionOrder.size(), liveCount);
			return false;
		}

		// Compute transient resource lifetimes over the compiled order
		for (auto& resource : m_resources)
		{
			resource.firstPass = ~0u;
			resource.lastPass = 0;
		}

		for (u32 position{ 0 }; position < m_executionOrder.size(); ++position)
		{
			const RGPass& pass = m_passes[m_executionOrder[position]];
			auto touch = [&](u32 resourceIndex)
				{
					RGResourceDesc& r = m_resources[resourceIndex];
					if (r.firstPass == ~0u)
						r.firstPass = position;
					r.lastPass = position;
				};

			for (u32 r : pass.reads)
				touch(r);
			for (u32 r : pass.writes)
				touch(r);
		}

		// Then resolve every resource actually touched by a live pass
		for (auto& resource : m_resources)
		{
			if (resource.firstPass == ~0u)
				continue;

			if (resource.imported)
				continue;

			if (resource.type == RGResourceType::Texture)
				resource.resolvedTarget = &m_pool->acquireTexture(resource.textureDesc);
			else
				resource.resolvedBuffer = &m_pool->acquireBuffer(resource.bufferDesc);
		}

		m_compiled = true;
		return true;
	}

	void RenderGraph::execute(ICommandList& cmd)
	{
		assert(m_compiled && "RenderGraph::execute(): called before a successful compile()");

		RenderGraphContext ctx{};
		ctx.m_cmd = &cmd;
		ctx.m_graph = this;

		for (u32 passIndex : m_executionOrder)
		{
			RGPass& pass = m_passes[passIndex];
			const bool hasAttachments = !pass.colours.empty()
				|| pass.depth.role != RGAttachmentRole::None;

			if (hasAttachments)
			{
				RenderPassDesc desc{};
				desc.debugName = pass.name.c_str();
				desc.clearDepth = false;
				
				assert(pass.colours.size() <= RenderPassDesc::kMaxColourAttachments
					&& "RenderGraph: pass writes more colour attachments than RenderPassDesc::kMaxColourAttachments supports");

				for (const RGPassAttachment& colourAttachment : pass.colours)
				{
					if (desc.colourTargetCount >= RenderPassDesc::kMaxColourAttachments)
						break;

					RenderPassColourAttachment& slot = desc.colourTargets[desc.colourTargetCount++];
					slot.target = &renderTargetOf(colourAttachment.resourceIndex);
					slot.clear = { colourAttachment.loadOp == RGLoadOp::Clear };
					slot.clearValue = colourAttachment.clearColour;
				}

				if (pass.depth.role == RGAttachmentRole::Depth)
				{
					desc.depthTarget = &renderTargetOf(pass.depth.resourceIndex);
					desc.clearDepth = ( pass.depth.loadOp == RGLoadOp::Clear );
					desc.clearDepthValue = pass.depth.clearDepth;
				}

				if (pass.resolve.role == RGAttachmentRole::Resolve)
					desc.resolveTarget = &renderTargetOf(pass.resolve.resourceIndex);

				cmd.beginRenderPass(desc);
				if (pass.execute)
					pass.execute(ctx);
				cmd.endRenderPass();
			}
			else if (pass.execute)
			{
				pass.execute(ctx);
			}
		}

		m_pool->releaseAll(m_device->currentFrameIndex());
	}

	std::string RenderGraph::debugDump() const
	{
		std::string out;
		out += "RenderGraph: " + std::to_string(m_executionOrder.size()) + "/" + std::to_string(m_passes.size())
			+ " passes compiled (" + std::to_string(m_passes.size() - m_executionOrder.size()) + " culled)\n";

		for (u32 position = 0; position < m_executionOrder.size(); ++position)
		{
			const RGPass& pass = m_passes[m_executionOrder[position]];
			out += "  [" + std::to_string(position) + "] " + pass.name;
			if (pass.hasSideEffect)
				out += " (side effect)";
			out += "\n";

			if (!pass.reads.empty())
			{
				out += "      reads:  ";
				for (u32 r : pass.reads)
					out += m_resources[r].name + " ";
				out += "\n";
			}
			if (!pass.writes.empty())
			{
				out += "      writes: ";
				for (u32 w : pass.writes)
					out += m_resources[w].name + " ";
				out += "\n";
			}
		}

		bool printedLifetimeHeader = false;
		for (const auto& r : m_resources)
		{
			if (r.imported || r.firstPass == ~0u)
				continue;
			if (!printedLifetimeHeader)
			{
				out += "  transient resource lifetimes:\n";
				printedLifetimeHeader = true;
			}
			out += "    " + r.name + ": pass " + std::to_string(r.firstPass) + " - " + std::to_string(r.lastPass) + "\n";
		}

		for (const auto& pass : m_passes)
		{
			if (pass.culled)
				out += "  culled: " + pass.name + " (no reads, no side effect)\n";
		}

		return out;
	}
}
