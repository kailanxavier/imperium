#include <sandbox/sandbox_scene.h>
#include <gfx/render_extraction.h>
#include <core/config/cvar.h>
#include <core/log/log.h>
#include <cmath>
#include <gfx/texture_cache.h>
#include <gfx/model_renderer.h>

namespace imp::app
{
	SandboxScene::SandboxScene() = default;
	SandboxScene::~SandboxScene() = default;

	bool SandboxScene::init(AppContext& ctx, const AssetManifest& assets)
	{
		m_environmentHandle = m_modelRegistry.load(ctx.gfx, assets.environmentModel, ctx.jobs, &ctx.vfs);
		if (!m_environmentHandle.isValid())
			LOG_ERROR("Sandbox", "Failed to load environment model");

		m_environmentTestHandle = m_modelRegistry.load(ctx.gfx, assets.environmentTestModel, ctx.jobs, &ctx.vfs);
		if (!m_environmentTestHandle.isValid())
			LOG_ERROR("Sandbox", "Failed to load environment test model");

		if (!m_environmentHandle.isValid())
			return false;

		const ecs::EntityId sunEntity = ctx.ecs.createEntity();
		ecs::Transform sunTransform;
		sunTransform.rotation = math::Quaternionf::fromAxisAngle(math::Vec3f::unitX(), math::toRadians(80.6f))
			* math::Quaternionf::fromAxisAngle(math::Vec3f::unitY(), math::toRadians(21.1f));
		m_sunDirection = math::normalise(math::rotate(sunTransform.rotation, math::Vec3f::forward()));
		ctx.ecs.transforms.create(sunEntity, sunTransform);
		ctx.ecs.lights.create(sunEntity, ecs::LightType::Directional, math::Vec3f{ 1.f, 0.82f, 0.55f }, 50.f);
		m_sunEntity = sunEntity;
		m_instances.push_back(sunEntity);

		m_localLight = ctx.ecs.createEntity();
		ecs::Transform pointTransform;
		pointTransform.position = math::Vec3f{ 0.f, 5.f, 0.f };
		ctx.ecs.transforms.create(m_localLight, pointTransform);
		ctx.ecs.colliders.createAABB(m_localLight, math::Vec3f{ -1.f, -1.f, -1.f }, math::Vec3f{ 1.f, 1.f, 1.f });
		ctx.ecs.lights.create(m_localLight, ecs::LightType::Point, math::Vec3f{ 1.f, 0.6f, 0.3f }, 1.5f);
		m_instances.push_back(m_localLight);

		{
			ecs::Transform t;
			t.position = math::Vec3f{ 0.f, 0.f, 15.f };
			spawnInstance(ctx, t, m_environmentTestHandle);
		}

		const ecs::EntityId entity = ctx.ecs.createEntity();
		ecs::Transform t;
		t.position = math::Vec3f{ 0.f, 0.f, 0.f };
		ctx.ecs.transforms.create(entity, t);
		ctx.ecs.renderables.create(entity, m_environmentHandle);
		ctx.ecs.scripts.create(entity, "assets/scripts/sponza.lua", true);
		ctx.ecs.colliders.createAABB(entity, math::Vec3f{ -1.f, -1.f, -1.f }, math::Vec3f{ 1.f, 1.f, 1.f });
		m_instances.push_back(entity);

		return true;
	}

	void SandboxScene::shutdown(AppContext& ctx)
	{
		for (ecs::EntityId instance : m_instances)
			ctx.ecs.destroyEntity(instance);

		m_instances.clear();
		m_extraction.clear();

		m_modelRegistry.shutdown();
		m_modelRegistry.clear();
	}

	ecs::EntityId SandboxScene::spawnInstance(AppContext& ctx, const ecs::Transform& t, const gfx::ModelHandle& model)
	{
		ecs::EntitySpawnDesc desc;
		desc.transform = t;
		desc.model = model;

		const ecs::EntityId entity = ctx.ecs.spawnEntity(desc);
		m_instances.push_back(entity);
		return entity;
	}

	void SandboxScene::update(AppContext& ctx, const fwk::Camera& camera)
	{
		ctx.ecs.transforms.updateWorldMatricesParallel(ctx.jobs);
		ctx.ecs.transforms.setLocalTransform(m_localLight, m_localLightTransform);

		updateSunViewProj();
		extractRenderables(ctx.ecs, m_modelRegistry, camera.position(), m_extraction);

		m_extraction.lightData.sunViewProj = m_sunViewProj;
		m_extraction.lightData.shadowMapSize = static_cast<float>( m_cascadeConfig.shadowMapResolution );

		buildStaticTlasOnce(ctx);
	}

	void SandboxScene::recomputeCascades(const fwk::Camera& camera, float aspect)
	{
		m_cascades = gfx::computeCascades(camera, aspect, m_sunDirection, m_cascadeConfig);
	}

	void SandboxScene::updateSunViewProj()
	{
		using namespace imp::math;

		static CVarFloat cvarSceneRadius{ "shadow.sun_scene_radius", 80.f };
		const float sceneRadius = cvarSceneRadius;

		Vec3f sunDir = normalise(m_sunDirection);
		Vec3f up = std::abs(dot(sunDir, Vec3f::up())) > 0.99f ? Vec3f::unitX() : Vec3f::up();

		const Vec3f sceneCentre = Vec3f::zero();
		const Vec3f eye = sceneCentre - sunDir * sceneRadius;

		Mat4f lightView = makeLookAtLH(eye, sceneCentre, up);
		Mat4f lightProj = makeOrthographicOffcentreLH(-sceneRadius, sceneRadius, -sceneRadius, sceneRadius, 0.1f, sceneRadius * 2.f);

		m_sunViewProj = lightProj * lightView;
	}

	void SandboxScene::buildStaticTlasOnce(AppContext& ctx)
	{
		if (m_staticTlasBuildAttempted)
			return;
		m_staticTlasBuildAttempted = true;

		if (!ctx.gfx.supportsRayTracing())
			return;

		std::vector<gfx::TlasInstanceDesc> instances = gfx::gatherTlasInstances(m_modelRegistry, m_extraction);
		if (instances.empty())
		{
			LOG_WARN("Sandbox", "buildStaticTlasOnce(): no instances with a built BLAS found");
			return;
		}

		gfx::TlasBuildDesc tlasDesc{};
		tlasDesc.instances = std::move(instances);
		tlasDesc.debugName = "Static Scene TLAS";

		m_staticTlas = ctx.gfx.createTlas(tlasDesc);
		if (!m_staticTlas)
			LOG_ERROR("Sandbox", "buildStaticTlasOnce(): createTlas() failed");
	}
}
