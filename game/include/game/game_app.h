#pragma once

#include <core/fs/directory_watcher.h>
#include <engine/render_resources.h>

#include <script/system.h>
#include <script/file_watcher.h>

#include <camera/camera.h>
#include <app/iapp.h>
#include <game/asset_manifest.h>
#include <game/game_scene.h>

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
		bool onInit(AppContext& ctx) override;
		void onUpdate(AppContext& ctx, float deltaSeconds) override;
		void onRender(AppContext& ctx, gfx::ICommandList& cmd) override;
		void onShutdown(AppContext& ctx) override;

		math::Vec3f& sunDirection() { return m_scene.sunDirection(); }
		const math::Vec3f& sunDirection() const { return m_scene.sunDirection(); }
		gfx::CascadeConfig& cascadeConfig() { return m_scene.cascadeConfig(); }
		ecs::Transform& pointPos() { return m_scene.pointLightTransform(); }
		const ecs::Transform& pointPos() const { return m_scene.pointLightTransform(); }

		const fwk::Camera& camera() const { return m_camera; }
		gfx::ModelRegistry& modelRegistry() { return m_scene.modelRegistry(); }
		gfx::TextureFormat hdrColourFormat() const { return m_resources.hdrColourFormat(); }
		gfx::TextureFormat hdrDepthFormat() const { return m_resources.hdrDepthFormat(); }
		gfx::SampleCount sampleCount() const { return engine::RenderResources::kMsaaSampleCount; }

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
	};
}
