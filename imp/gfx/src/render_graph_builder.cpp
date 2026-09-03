#include <core/types/int_types.h>

#include "gfx/render_graph_builder.h"
#include "gfx/render_graph_types.h"
#include "gfx/render_graph.h"

namespace imp::gfx
{
	RGTextureHandle RenderGraphBuilder::createTexture(const char* name, const TextureDesc& desc)
	{
		RGResourceDesc entry{};
		entry.name = name ? name : "";
		entry.type = RGResourceType::Texture;
		entry.imported = false;
		entry.textureDesc = desc;

		const u32 index = static_cast<u32>( m_graph->m_resources.size() );
		m_graph->m_resources.push_back(std::move(entry));

		RGTextureHandle handle{};
		handle.index = index;
		return handle;
	}

	RGBufferHandle RenderGraphBuilder::createBuffer(const char* name, const BufferDesc& desc)
	{
		RGResourceDesc entry{};
		entry.name = name ? name : "";
		entry.type = RGResourceType::Buffer;
		entry.imported = false;
		entry.bufferDesc = desc;

		const u32 index = static_cast<u32>( m_graph->m_resources.size() );
		m_graph->m_resources.push_back(std::move(entry));

		RGBufferHandle handle{};
		handle.index = index;
		return handle;
	}

	RGTextureHandle RenderGraphBuilder::importTexture(const char* name, IRenderTarget* target)
	{
		RGResourceDesc entry{};
		entry.name = name ? name : "";
		entry.type = RGResourceType::Texture;
		entry.imported = true;
		entry.resolvedTarget = target;

		const u32 index = static_cast<u32>( m_graph->m_resources.size() );
		m_graph->m_resources.push_back(std::move(entry));

		RGTextureHandle handle{};
		handle.index = index;
		return handle;
	}

	RGTextureHandle RenderGraphBuilder::importTexture(const char* name, ITexture* texture)
	{
		RGResourceDesc entry{};
		entry.name = name ? name : "";
		entry.type = RGResourceType::Texture;
		entry.imported = true;
		entry.resolvedTextureOnly = texture;

		const u32 index = static_cast<u32>( m_graph->m_resources.size() );
		m_graph->m_resources.push_back(std::move(entry));

		RGTextureHandle handle{};
		handle.index = index;
		return handle;
	}

	RGBufferHandle RenderGraphBuilder::importBuffer(const char* name, IBuffer* buffer)
	{
		RGResourceDesc entry{};
		entry.name = name ? name : "";
		entry.type = RGResourceType::Buffer;
		entry.imported = true;
		entry.resolvedBuffer = buffer;

		const u32 index = static_cast<u32>( m_graph->m_resources.size() );
		m_graph->m_resources.push_back(std::move(entry));

		RGBufferHandle handle{};
		handle.index = index;
		return handle;
	}

	RGTextureHandle RenderGraphBuilder::readTexture(RGTextureHandle handle)
	{
		m_graph->recordRead(m_passIndex, handle.index);
		return handle;
	}

	RGBufferHandle RenderGraphBuilder::readBuffer(RGBufferHandle handle)
	{
		m_graph->recordRead(m_passIndex, handle.index);
		return handle;
	}

	RGTextureHandle RenderGraphBuilder::writeColour(RGTextureHandle handle, RGLoadOp loadOp, ClearColour clear)
	{
		if (loadOp == RGLoadOp::Load)
			m_graph->recordRead(m_passIndex, handle.index); // reads whatever was there before

		const u32 newVersion = m_graph->recordWrite(m_passIndex, handle.index);

		RGPass& pass = m_graph->m_passes[m_passIndex];

		RGPassAttachment attachment{};
		attachment.role = RGAttachmentRole::Colour;
		attachment.resourceIndex = handle.index;
		attachment.loadOp = loadOp;
		attachment.clearColour = clear;
		pass.colours.push_back(attachment);

		RGTextureHandle out = handle;
		out.version = newVersion;
		return out;
	}

	RGTextureHandle RenderGraphBuilder::writeDepth(RGTextureHandle handle, RGLoadOp loadOp, float clearDepth)
	{
		if (loadOp == RGLoadOp::Load)
			m_graph->recordRead(m_passIndex, handle.index);

		const u32 newVersion = m_graph->recordWrite(m_passIndex, handle.index);

		RGPass& pass = m_graph->m_passes[m_passIndex];
		pass.depth.role = RGAttachmentRole::Depth;
		pass.depth.resourceIndex = handle.index;
		pass.depth.loadOp = loadOp;
		pass.depth.clearDepth = clearDepth;

		RGTextureHandle out = handle;
		out.version = newVersion;
		return out;
	}

	RGTextureHandle RenderGraphBuilder::writeResolve(RGTextureHandle handle, RGTextureHandle msaaColourSource)
	{
		m_graph->recordRead(m_passIndex, msaaColourSource.index);

		const u32 newVersion = m_graph->recordWrite(m_passIndex, handle.index);

		RGPass& pass = m_graph->m_passes[m_passIndex];
		pass.resolve.role = RGAttachmentRole::Resolve;
		pass.resolve.resourceIndex = handle.index;
		pass.resolve.loadOp = RGLoadOp::DontCare;
		pass.resolve.resolveSourceIndex = msaaColourSource.index;

		RGTextureHandle out = handle;
		out.version = newVersion;
		return out;
	}

	void RenderGraphBuilder::hasSideEffect()
	{
		m_graph->m_passes[m_passIndex].hasSideEffect = true;
	}
}
