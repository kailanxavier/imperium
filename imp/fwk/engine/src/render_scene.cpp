#include "detail/render_scene.h"

#include <core/config/cvar.h>
#include <core/log/log.h>

#include <ecs/world.h>
#include <jobs/job_system.h>

#include <gfx/ddgi_volume.h>
#include <gfx/device.h>
#include <gfx/lighting.h>
#include <gfx/model_renderer.h>
#include <gfx/render_extraction.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace imp::engine
{
	void RenderScene::init(gfx::ModelRegistry& registry)
	{
		m_modelRegistry = &registry;
	}

	void RenderScene::shutdown()
	{
		m_staticTlas.reset();
		m_ddgiInstanceMaterials.reset();
		m_extraction.clear();
		m_modelRegistry = nullptr;
	}

	void RenderScene::update(RenderContext& ctx, ecs::World& world, const fwk::Camera& camera, const sky::SkyState& sky, bool buildTlas)
	{
		m_mainLightDirection = math::normalise(sky.main.direction);
		world.transforms.updateWorldMatricesParallel(ctx.jobs);

		updateSunViewProj();
		gfx::extractRenderables(world, *m_modelRegistry, camera.position(), m_extraction);

		m_extraction.lightData.sunViewProj = m_sunViewProj;
		gfx::setMainLight(m_extraction.lightData, m_mainLightDirection, sky.main.colour,
			sky.main.intensity, sky.main.shadowStrength);

		if (buildTlas)
			updateDynamicTlas(ctx);
	}

	void RenderScene::recomputeCascades(const fwk::Camera& camera, float aspect)
	{
		m_cascades = gfx::computeCascades(camera, aspect, m_mainLightDirection, m_cascadeConfig);
	}

	void RenderScene::updateSunViewProj()
	{
		using namespace imp::math;

		static CVarFloat cvarSceneRadius{ "shadow.sun_scene_radius", 80.f };
		const float sceneRadius = cvarSceneRadius;

		Vec3f sunDir = normalise(m_mainLightDirection);
		Vec3f up = std::abs(dot(sunDir, Vec3f::up())) > 0.99f ? Vec3f::unitX() : Vec3f::up();

		const Vec3f sceneCentre = Vec3f::zero();
		const Vec3f eye = sceneCentre - sunDir * sceneRadius;

		Mat4f lightView = makeLookAtLH(eye, sceneCentre, up);
		Mat4f lightProj = makeOrthographicOffcentreLH(-sceneRadius, sceneRadius, -sceneRadius, sceneRadius, 0.1f, sceneRadius * 2.f);

		m_sunViewProj = lightProj * lightView;
	}

	void RenderScene::releaseTlas(RenderContext& ctx)
	{
		if (!m_staticTlas && !m_ddgiInstanceMaterials)
			return;

		auto oldTlas = std::shared_ptr<gfx::ITlas>(std::move(m_staticTlas));
		auto oldMats = std::shared_ptr<gfx::IBuffer>(std::move(m_ddgiInstanceMaterials));

		ctx.gfx.deferredDestroy([oldTlas, oldMats]() {});
	}

	void RenderScene::updateDynamicTlas(RenderContext& ctx)
	{
		if (!ctx.gfx.supportsRayTracing())
			return;

		std::vector<gfx::DDGIInstanceMaterial> materials;
		std::vector<gfx::TlasInstanceDesc> instances = gfx::gatherTlasInstances(*m_modelRegistry, m_extraction, &materials);
		if (instances.empty())
			return;

		gfx::TlasBuildDesc tlasDesc{};
		tlasDesc.instances = std::move(instances);
		tlasDesc.debugName = "Dynamic Scene TLAS";

		auto newTlas = ctx.gfx.createTlas(tlasDesc);
		if (!newTlas)
		{
			LOG_ERROR("Engine", "updateDynamicTlas(): createTlas() failed");
			return;
		}

		gfx::BufferDesc materialsDesc{};
		materialsDesc.size = materials.size() * sizeof(gfx::DDGIInstanceMaterial);
		materialsDesc.usage = gfx::BufferUsage::Storage;
		materialsDesc.memoryAccess = gfx::MemoryAccess::HostVisible;
		auto newMaterials = ctx.gfx.createBuffer(materialsDesc);
		if (newMaterials)
			newMaterials->update(materials.data(), materialsDesc.size, 0);
		else
			LOG_ERROR("Engine", "buildStaticTlasOnce(): instance material buffer allocation failed");

		releaseTlas(ctx);

		m_staticTlas = std::move(newTlas);
		m_ddgiInstanceMaterials = std::move(newMaterials);
	}
}
