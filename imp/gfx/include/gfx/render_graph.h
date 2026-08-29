#pragma once
#include <functional>

#include "gfx/render_graph_builder.h"
#include "gfx/render_graph_context.h"
#include "gfx/render_graph_types.h"

#include <string>

namespace imp::gfx
{
	class IDevice;
	class RenderGraphResourcePool;

	class RenderGraph
	{
	public:
		RenderGraph(IDevice& device, RenderGraphResourcePool& pool);

		template <typename PassData>
		const PassData& addPass(const char* name,
			std::function<void(RenderGraphBuilder&, PassData&)> setup,
			std::function<void(const PassData&, RenderGraphContext&)> execute);

		bool compile();

		void execute(ICommandList& cmd);
		[[nodiscard]] std::string debugDump() const;

	private:
		friend class RenderGraphBuilder;
		friend class RenderGraphContext;

		void recordRead(u32 passIndex, u32 resourceIndex);
		u32 recordWrite(u32 passIndex, u32 resourceIndex);

		[[nodiscard]] ITexture& textureOf(u32 resourceIndex);
		[[nodiscard]] IRenderTarget& renderTargetOf(u32 resourceIndex);
		[[nodiscard]] IBuffer& bufferOf(u32 resourceIndex);

		IDevice* m_device = nullptr;
		RenderGraphResourcePool* m_pool = nullptr;

		std::vector<RGResourceDesc> m_resources;
		std::vector<RGPass> m_passes;
		std::vector<std::pair<u32, u32>> m_edges;
		std::vector<u32> m_executionOrder;

		bool m_compiled = false;
	};
}
