#include <app/render_extraction.h>
#include <ecs/world.h>
#include <gfx/model_registry.h>
#include <algorithm>

namespace imp::app
{
	namespace
	{
		math::Vec3f translationOf(const math::Mat4f& worldMatrix)
		{
			return math::Vec3f{ worldMatrix.col[3][0], worldMatrix.col[3][1], worldMatrix.col[3][2] };
		}

		void extractLights(const ecs::World& world, gfx::LightUBO& out)
		{
			const ecs::LightStorage& lights = world.lights;
			const std::vector<ecs::EntityId>& owners = lights.owners();
			const std::vector<ecs::LightType>& types = lights.types();
			const std::vector<math::Vec3f>& colours = lights.colours();
			const std::vector<float>& intensities = lights.intensities();

			const u32 count = std::min<u32>( static_cast<u32>( owners.size() ), gfx::kMaxLights );
			out.lightCount = count;

			for (u32 i = 0; i < count; ++i)
			{
				const ecs::EntityId owner = owners[i];
				const math::Mat4f worldMatrix = world.transforms.worldMatrix(owner);
				gfx::GPULight& gpuLight = out.lights[i];

				if (types[i] == ecs::LightType::Directional)
				{
					const math::Vec3f worldDir = math::rotate(world.transforms.localTransform(owner).rotation, math::Vec3f::forward());
					gpuLight.positionOrDirWS = math::Vec4f{ worldDir, 0.f };
				}
				else
				{
					gpuLight.positionOrDirWS = math::Vec4f{ translationOf(worldMatrix), 1.f };
				}

				gpuLight.colourIntensity = math::Vec4f{ colours[i], intensities[i] };
			}
		}

	}

	void extractRenderables(const ecs::World& world, const gfx::ModelRegistry& modelRegistry,
		const math::Vec3f& cameraPositionWS, RenderExtraction& out)
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

			const gfx::Model* model = modelRegistry.tryGet(range.model);
			const bool hasBlendPrimitives = model && model->hasBlendPrimitives;

			for (u32 pos = range.start; pos < range.end; ++pos)
			{
				const u32 dense = order[pos];
				if (!visibility[dense])
					continue;

				const math::Mat4f worldMatrix = world.transforms.worldMatrix(owners[dense]);
				const u32 instanceOffset = static_cast<u32>(out.instanceData.size());
				out.instanceData.push_back(worldMatrix);

				if (hasBlendPrimitives)
				{
					const math::Vec3f toEntity = translationOf(worldMatrix) - cameraPositionWS;
					const float distSq = math::lengthSq(toEntity);
					out.blendInstances.push_back(BlendInstance{ range.model, instanceOffset, distSq });
				}
			}

			const u32 instanceCount = static_cast<u32>( out.instanceData.size() ) - firstInstance;
			if (instanceCount > 0)
				out.batches.push_back(ModelBatch{ range.model, firstInstance, instanceCount });
		}

		std::sort(out.blendInstances.begin(), out.blendInstances.end(), 
			[](const BlendInstance& a, const BlendInstance& b) { return a.cameraDistanceSq > b.cameraDistanceSq; });

		out.lightData.cameraPositionWS = math::Vec4f{ cameraPositionWS, 0.f };
		extractLights(world, out.lightData);
	}
}
