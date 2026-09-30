#include <game/game_scene.h>

#include <gfx/render_extraction.h>
#include <gfx/model_renderer.h>
#include <gfx/ddgi_volume.h>
#include <gfx/texture_cache.h>

#include <core/config/cvar.h>
#include <core/log/log.h>

#include <cmath>
#include <algorithm>

namespace imp::game
{
	GameScene::GameScene() = default;
	GameScene::~GameScene() = default;

	bool GameScene::init(AppContext& ctx, const AssetManifest& assets)
	{
		m_environmentHandle = m_modelRegistry.load(ctx.gfx, assets.environmentModel, ctx.jobs, &ctx.vfs);
		if (!m_environmentHandle.isValid())
			LOG_ERROR("Game", "Failed to load environment model");

		m_environmentTestHandle = m_modelRegistry.load(ctx.gfx, assets.environmentTestModel, ctx.jobs, &ctx.vfs);
		if (!m_environmentTestHandle.isValid())
			LOG_ERROR("Game", "Failed to load environment test model");

		if (!m_environmentHandle.isValid())
			return false;

		m_localLight = ctx.ecs.createEntity();
		ecs::Transform pointTransform;
		pointTransform.position = math::Vec3f{ 0.f, 5.f, 0.f };
		ctx.ecs.transforms.create(m_localLight, pointTransform);
		ctx.ecs.colliders.createAABB(m_localLight, math::Vec3f{ -1.f, -1.f, -1.f }, math::Vec3f{ 1.f, 1.f, 1.f });
		ctx.ecs.lights.create(m_localLight, ecs::LightType::Point, math::Vec3f{ 1.f, 0.6f, 0.3f }, 0.f);
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

	void GameScene::shutdown(AppContext& ctx)
	{
		for (ecs::EntityId instance : m_instances)
			ctx.ecs.destroyEntity(instance);

		m_instances.clear();
		m_extraction.clear();

		m_modelRegistry.shutdown();
		m_modelRegistry.clear();
	}

	ecs::EntityId GameScene::spawnInstance(AppContext& ctx, const ecs::Transform& t, const gfx::ModelHandle& model)
	{
		ecs::EntitySpawnDesc desc;
		desc.transform = t;
		desc.model = model;

		const ecs::EntityId entity = ctx.ecs.spawnEntity(desc);
		m_instances.push_back(entity);
		return entity;
	}

	void GameScene::update(AppContext& ctx, const fwk::Camera& camera, const sky::SkyState& sky)
	{
		m_mainLightDirection = math::normalise(sky.main.direction);
		ctx.ecs.transforms.updateWorldMatricesParallel(ctx.jobs);

		updateSunViewProj();
		extractRenderables(ctx.ecs, m_modelRegistry, camera.position(), m_extraction);

		m_extraction.lightData.sunViewProj = m_sunViewProj;
		gfx::setMainLight(m_extraction.lightData, m_mainLightDirection, sky.main.colour,
			sky.main.intensity, sky.main.shadowStrength);

		updateDynamicTlas(ctx);
	}

	void GameScene::recomputeCascades(const fwk::Camera& camera, float aspect)
	{
		m_cascades = gfx::computeCascades(camera, aspect, m_mainLightDirection, m_cascadeConfig);
	}

	void GameScene::updateSunViewProj()
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

	void GameScene::updateDynamicTlas(AppContext& ctx)
	{
		if (!ctx.gfx.supportsRayTracing())
			return;

		std::vector<gfx::DDGIInstanceMaterial> materials;
		std::vector<gfx::TlasInstanceDesc> instances = gfx::gatherTlasInstances(m_modelRegistry, m_extraction, &materials);
		if (instances.empty())
		{
			//LOG_WARN("Game", "buildStaticTlasOnce(): no instances with a built BLAS found");
			return;
		}

		gfx::TlasBuildDesc tlasDesc{};
		tlasDesc.instances = std::move(instances);
		tlasDesc.debugName = "Dynamic Scene TLAS";

		auto newTlas = ctx.gfx.createTlas(tlasDesc);
		if (!newTlas)
		{
			LOG_ERROR("Game", "updateDynamicTlas(): createTlas() failed");
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
			LOG_ERROR("Game", "buildStaticTlasOnce(): instance material buffer allocation failed");

		if (m_staticTlas || m_ddgiInstanceMaterials)
		{
			auto oldTlas = std::shared_ptr<gfx::ITlas>(std::move(m_staticTlas));
			auto oldMats = std::shared_ptr<gfx::IBuffer>(std::move(m_ddgiInstanceMaterials));

			ctx.gfx.deferredDestroy([oldTlas, oldMats]()
				{

				});
		}

		m_staticTlas = std::move(newTlas);
		m_ddgiInstanceMaterials = std::move(newMaterials);
	}
}
