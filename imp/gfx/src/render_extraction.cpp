#include <gfx/render_extraction.h>
#include <gfx/model_registry.h>
#include <ecs/world.h>
#include <core/log/log.h>
#include <algorithm>


namespace imp::gfx
{
	namespace
	{
		math::Vec3f translationOf(const math::Mat4f& worldMatrix)
		{
			return math::Vec3f{ worldMatrix.col[3][0], worldMatrix.col[3][1], worldMatrix.col[3][2] };
		}

		void extractLights(const ecs::World& world, gfx::LightUBO& out)
		{
			gfx::GPULight& mainSlot = out.lights[gfx::kMainLightSlot];
			mainSlot.positionOrDirWS = math::Vec4f{ 0.f, -1.f, 0.f, 0.f };
			mainSlot.colourIntensity = math::Vec4f{ 1.f, 1.f, 1.f, 0.f };

			const ecs::LightStorage& lights = world.lights;
			const std::vector<ecs::EntityId>& owners = lights.owners();
			const std::vector<ecs::LightType>& types = lights.types();
			const std::vector<math::Vec3f>& colours = lights.colours();
			const std::vector<float>& intensities = lights.intensities();

			u32 count = gfx::kMainLightSlot + 1;
			bool skippedDirectional = false;

			for (size_t i{ 0 }; i < owners.size() && count < gfx::kMaxLights; ++i)
			{
				if (types[i] == ecs::LightType::Directional)
				{
					skippedDirectional = true;
					continue;
				}

				const math::Mat4f worldMatrix = world.transforms.worldMatrix(owners[i]);
				gfx::GPULight& gpuLight = out.lights[count++];

				gpuLight.positionOrDirWS = math::Vec4f{ translationOf(worldMatrix), 1.f };
				gpuLight.colourIntensity = math::Vec4f{ colours[i], intensities[i] };
			}

			out.lightCount = count;
			static bool warned = false;
			if (skippedDirectional && !warned)
			{
				warned = true;
				LOG_WARN("Renderer", "Ignoring directional light entities. The sky owns the main direction light.");
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
