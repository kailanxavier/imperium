#pragma once

#include <app/iapp.h>
#include <game/asset_manifest.h>

#include <ecs/world.h>
#include <gfx/model_registry.h>

#include <vector>

namespace imp::game
{
	using app::AppContext;

	class GameScene
	{
	public:
		GameScene() = default;
		~GameScene() = default;

		GameScene(const GameScene&) = delete;
		GameScene& operator=(const GameScene&) = delete;

		bool init(AppContext& ctx, gfx::ModelRegistry& models, const AssetManifest& assets);
		void shutdown(AppContext& ctx);

		ecs::EntityId spawnInstance(AppContext& ctx, const ecs::Transform& t, const gfx::ModelHandle& model);

	private:
		ecs::ModelHandle m_environmentHandle{};
		ecs::ModelHandle m_environmentTestHandle{};

		ecs::EntityId m_localLight{};
		std::vector<ecs::EntityId> m_instances;
	};
}
