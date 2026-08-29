#pragma once
#include "gfx/resources.h"
#include "gfx/commands.h"
#include "gfx/render_graph_handles.h"

namespace imp::gfx
{
	class RenderGraphContext
	{
	public:
		[[nodiscard]] ICommandList& cmd() const { return *m_cmd; }
		[[nodiscard]] ITexture& texture(RGTextureHandle handle) const;
		[[nodiscard]] IRenderTarget& renderTarget(RGTextureHandle handle) const;
		[[nodiscard]] IBuffer& buffer(RGBufferHandle handle) const;

	private:
		friend class RenderGraph;
		ICommandList* m_cmd = nullptr;
		RenderGraph* m_graph = nullptr;
	};
}
