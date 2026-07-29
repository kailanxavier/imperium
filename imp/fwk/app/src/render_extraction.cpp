#include <app/render_extraction.h>
#include <ecs/world.h>

namespace imp::app
{
	void extractRenderables(const ecs::World& world, RenderExtraction& out)
	{
		out.clear();

		const ecs::RenderableStorage& renderables = world.renderables;
		const std::vector<u32>& order = renderables.order();
		const std::vector<ecs::RenderableRange>& ranges = renderables.ranges();
		const std::vector<ecs::EntityId>& owners = renderables.owners();
		const std::vector<u8>& visibility = renderables.visibility();

		out.instanceData.reserve(renderables.size());
		out.batches.reserve(ranges.size());

		for (const ecs::RenderableRange& range : ranges)
		{
			const u32 firstInstance = static_cast<u32>( out.instanceData.size() );

			for (u32 pos = range.start; pos < range.end; ++pos)
			{
				const u32 dense = order[pos];
				if (!visibility[dense])
					continue;

				out.instanceData.push_back(world.transforms.worldMatrix(owners[dense]));
			}

			const u32 instanceCount = static_cast<u32>(out.instanceData.size()) - firstInstance;

			// Skip if all entities are invisible, no point in handing it to the renderer
			if (instanceCount > 0)
				out.batches.push_back(ModelBatch{ range.model, firstInstance, instanceCount });
		}
	}
}
