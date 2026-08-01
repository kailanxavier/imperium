#pragma once
#include <core/math/math.h>
#include <core/types/int_types.h>
#include <gfx/model_handle.h>
#include <vector>

namespace imp::ecs { class World; }

namespace imp::app
{
	struct ModelBatch
	{
		gfx::ModelHandle model;
		u32 firstInstance = 0;
		u32 instanceCount = 0;
	};

	struct RenderExtraction
	{
		std::vector<math::Mat4f> instanceData;
		std::vector<ModelBatch> batches;

		void clear()
		{
			instanceData.clear();
			batches.clear();
		}
	};

	void extractRenderables(const ecs::World& world, RenderExtraction& out);
}
