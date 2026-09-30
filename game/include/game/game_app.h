#pragma once

#include <core/fs/directory_watcher.h>
#include <engine/render_resources.h>

#include <script/system.h>
#include <script/file_watcher.h>

#include <camera/camera.h>
#include <game/asset_manifest.h>
#include <game/game_scene.h>
#include <game/light_control_layer.h>

#include <app/iapp.h>
#include <app/render_target_formats.h>
#include <sky/sky_state.h>
#include <sky/sky_system.h>

#include <memory>

namespace imp::gfx
{
	class IShader;
	class IPipeline;
	class ISampler;
	class IBuffer;
}

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
		void setReadbackTarget(gfx::IRenderTarget* target) { m_readbackTarget = target; }

	private:
		fwk::Camera m_camera;
		AssetManifest m_assets;
		engine::RendererManifest m_rendererManifest;
		engine::RenderResources m_resources;
		GameScene m_scene;
		bool m_enableFrustumCulling = true;

		std::unique_ptr<script::ScriptSystem> m_scriptSystem;

		std::unique_ptr<fs::DirectoryWatcher> m_scriptSourceWatcher;
		void pollScriptHotReload(AppContext& ctx);

		gfx::IRenderTarget* m_readbackTarget = nullptr;

		app::RenderTargetFormats m_targetFormats{};
		LightControlRefs m_lightRefs{};

		sky::SkyState m_fallbackSky = sky::SkySystem{}.state();
		const sky::SkyState* m_currentSky = nullptr;
	};
}
