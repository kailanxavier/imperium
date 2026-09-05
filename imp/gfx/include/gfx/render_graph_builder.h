#pragma once

#include "render_graph_handles.h"
#include "gfx/resources.h"
#include "gfx/commands.h"

namespace imp::gfx
{
	class RenderGraph;
	class RenderGraphBuilder
	{
	public:
		RGTextureHandle createTexture(const char* name, const TextureDesc& desc);
		RGBufferHandle createBuffer(const char* name, const BufferDesc& desc);

		RGTextureHandle importTexture(const char* name, IRenderTarget* target);
		RGTextureHandle importTexture(const char* name, ITexture* texture);
		RGBufferHandle importBuffer(const char* name, IBuffer* buffer);

		RGTextureHandle readTexture(RGTextureHandle handle);
		RGBufferHandle readBuffer(RGBufferHandle handle);

		RGTextureHandle writeColour(RGTextureHandle handle, RGLoadOp loadOp, ClearColour clear = {});
		RGTextureHandle writeDepth(RGTextureHandle handle, RGLoadOp loadOp, float clearDepth = 1.f);
		RGTextureHandle writeResolve(RGTextureHandle handle, RGTextureHandle msaaColourSource);
		RGTextureHandle writeStorageTexture(RGTextureHandle handle);

		void hasSideEffect();

	private:
		friend class RenderGraph;
		RenderGraph* m_graph = nullptr;
		u32 m_passIndex = 0;
	};
}
