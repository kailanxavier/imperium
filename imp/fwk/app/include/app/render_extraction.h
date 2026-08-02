#pragma once
#include <core/math/math.h>
#include <core/types/int_types.h>
#include <gfx/model_handle.h>
#include <gfx/lighting.h>
#include <vector>

namespace imp::ecs { class World; }
namespace imp::gfx { class ModelRegistry; }
namespace imp::app
{
	struct ModelBatch
	{
		gfx::ModelHandle model;
		u32 firstInstance = 0;
		u32 instanceCount = 0;
	};

	struct BlendInstance
	{
		gfx::ModelHandle model;
		u32 instanceOffset = 0;
		float cameraDistanceSq = 0.f;
	};

	struct RenderExtraction
	{
		std::vector<math::Mat4f> instanceData;
		std::vector<ModelBatch> batches;
		std::vector<BlendInstance> blendInstances;
		gfx::LightUBO lightData;
		void clear()
		{
			instanceData.clear();
			batches.clear();
			blendInstances.clear();
			lightData = gfx::LightUBO{};
		}
	};

	void extractRenderables(const ecs::World& world, const gfx::ModelRegistry& modelRegistry,
		const math::Vec3f& cameraPositionWS, RenderExtraction& out);
}
