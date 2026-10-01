#include <game/game_scene.h>
#include <core/log/log.h>

#include <ecs/world.h>
#include <scene/scene.h>
#include <sky/sky_system.h>

#include <filesystem>
#include <optional>
#include <vector>

namespace imp::game
{
	namespace
	{
		// TODO: We need to give the scene the ability to store colliders
		const math::Vec3f kDefaultColliderHalfExtents{ 0.5f, 0.5f, 0.5f };

		void addPickColliders(ecs::World& world)
		{
			for (const ecs::EntityId id : world.transforms.m_owner)
			{
				if (world.colliders.contains(id))
					continue;

				if (!world.renderables.contains(id) && !world.lights.contains(id))
					continue;

				world.colliders.createAABB(id, -kDefaultColliderHalfExtents, kDefaultColliderHalfExtents);
			}
		}
	}

	bool loadStartupScene(AppContext& ctx, gfx::ModelRegistry& models, const std::string& path)
	{
		std::optional<fwk::Scene> scene = fwk::Scene::loadFromFile(ctx.vfs, path);
		if (!scene && std::filesystem::exists(path))
			scene = fwk::Scene::loadFromFile(std::filesystem::path(path));

		if (!scene)
		{
			LOG_ERROR("Game", "Could not load startup scene '{}'. World will be empty.", path);
			return false;
		}

		const fwk::Scene::ModelLoader loadModel = [&](const std::string& modelPath) -> ecs::ModelHandle
			{
				const ecs::ModelHandle handle = models.load(ctx.gfx, modelPath, ctx.jobs, &ctx.vfs);
				if (!handle.isValid())
					LOG_ERROR("Game", "Startup scene '{}': failed to load model '{}'", path, modelPath);
				return handle;
			};

		sky::SkySystem* skySystem = ctx.services.tryGet<sky::SkySystem>();
		scene->applyToWorld(ctx.ecs, loadModel, skySystem ? &skySystem->settings() : nullptr);

		if (skySystem && scene->environment)
			skySystem->snap();

		addPickColliders(ctx.ecs);

		LOG_INFO("Game", "Loaded startup scene '{}' ({} entities)", path, scene->entities.size());
		return true;
	}

	void unloadScene(AppContext& ctx)
	{
		const std::vector<ecs::EntityId> entities = ctx.ecs.transforms.m_owner;
		for (const ecs::EntityId id : entities)
			ctx.ecs.destroyEntity(id);
	}
}
