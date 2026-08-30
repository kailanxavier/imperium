#include "gfx/render_graph_context.h"
#include "gfx/render_graph.h"

namespace imp::gfx
{
	ITexture& RenderGraphContext::texture(RGTextureHandle handle) const
	{
		return m_graph->textureOf(handle.index);
	}

	IRenderTarget& RenderGraphContext::renderTarget(RGTextureHandle handle) const
	{
		return m_graph->renderTargetOf(handle.index);
	}

	IBuffer& RenderGraphContext::buffer(RGBufferHandle handle) const
	{
		return m_graph->bufferOf(handle.index);
	}
}
