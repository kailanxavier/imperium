#include <game/game_scene.h>
#include <core/log/log.h>

namespace imp::game
{
	bool GameScene::init(AppContext& ctx, gfx::ModelRegistry& models, const AssetManifest& assets)
	{
		m_environmentHandle = models.load(ctx.gfx, assets.environmentModel, ctx.jobs, &ctx.vfs);
		if (!m_environmentHandle.isValid())
			LOG_ERROR("Game", "Failed to load environment model");

		m_environmentTestHandle = models.load(ctx.gfx, assets.environmentTestModel, ctx.jobs, &ctx.vfs);
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
}
