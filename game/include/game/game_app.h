#pragma once

#include <core/fs/directory_watcher.h>

#include <script/system.h>
#include <script/file_watcher.h>

#include <camera/camera.h>
#include <engine/deferred_renderer.h>
#include <game/asset_manifest.h>
#include <game/game_scene.h>
#include <game/light_control_layer.h>

#include <app/iapp.h>
#include <app/render_target_formats.h>
#include <gfx/model_registry.h>
#include <sky/sky_state.h>
#include <sky/sky_system.h>

#include <memory>

namespace imp::game
{
	using app::AppContext;

	class GameApp final : public app::IApp
	{
	public:
		void onRegisterServices(AppContext& ctx) override;
		bool onInit(AppContext& ctx) override;
		void onUpdate(AppContext& ctx, float deltaSeconds) override;
		void onRender(AppContext& ctx, gfx::ICommandList& cmd) override;
		void onShutdown(AppContext& ctx) override;
		engine::DeferredRenderer& renderer() { return m_renderer; }

	private:
		void pollScriptHotReload(AppContext& ctx);

		fwk::Camera m_camera;
		AssetManifest m_assets;

		gfx::ModelRegistry m_modelRegistry;
		engine::DeferredRenderer m_renderer;

		GameScene m_scene;

		std::unique_ptr<script::ScriptSystem> m_scriptSystem;
		std::unique_ptr<fs::DirectoryWatcher> m_scriptSourceWatcher;

		app::RenderTargetFormats m_targetFormats{};
		LightControlRefs m_lightRefs{};

		sky::SkyState m_fallbackSky = sky::SkySystem{}.state();
	};
}
