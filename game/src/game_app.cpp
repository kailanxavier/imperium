#include <game/game_app.h>
#include <core/log/log.h>

#include <fstream>
#include <filesystem>
#include <memory>

namespace imp::game
{
	namespace
	{
		engine::RenderContext makeRenderContext(AppContext& ctx)
		{
			return engine::RenderContext{ ctx.gfx, ctx.vfs, ctx.jobs, &ctx.layers };
		}
	}

	void GameApp::onRegisterServices(AppContext& ctx)
	{
		m_targetFormats = { 
			m_renderer.hdrColourFormat(), 
			m_renderer.hdrDepthFormat(), 
			engine::DeferredRenderer::kSampleCount };

		m_lightRefs = { &m_renderer.cascadeConfig() };

		ctx.services.provide(m_camera);
		ctx.services.provide(m_modelRegistry);
		ctx.services.provide(m_targetFormats);
		ctx.services.provide(m_lightRefs);
	}

	bool GameApp::onInit(AppContext& ctx)
	{
		m_camera.setPosition({ 0.f, 1.f, 0.f });
		m_camera.setYawPitch(math::toRadians(90.f), 0.f);

		engine::RenderContext renderCtx = makeRenderContext(ctx);

		engine::RendererDesc rendererDesc{};
		rendererDesc.modelRegistry = &m_modelRegistry;
		if (!m_renderer.init(renderCtx, rendererDesc))
			return false;

		if (!m_scene.init(ctx, m_modelRegistry, m_assets))
			return false;

		m_scriptSystem = std::make_unique<script::ScriptSystem>(ctx.vfs);

		m_scriptSourceWatcher = std::make_unique<fs::DirectoryWatcher>(
			std::filesystem::path(IMP_SCRIPT_SOURCE_DIR), std::vector<std::string>{ ".lua" });

		if (m_scriptSourceWatcher->isValid())
			LOG_INFO("Game", "Script hot reload active");

		return true;
	}

	void GameApp::pollScriptHotReload(AppContext& ctx)
	{
		if (!m_scriptSourceWatcher || !m_scriptSourceWatcher->isValid())
			return;

		for (const std::string& relative : m_scriptSourceWatcher->poll())
		{
			std::ifstream sourceFile(m_scriptSourceWatcher->root() / relative, std::ios::binary);
			if (!sourceFile)
			{
				LOG_ERROR("Script", "Hot reload of '{}' failed. Could not open source file", relative.c_str());
				continue;
			}


			const fs::Bytes bytes((std::istreambuf_iterator<char>(sourceFile)), std::istreambuf_iterator<char>());

			const std::string virtualPath = "assets/scripts/" + relative;
			if (!ctx.vfs.writeEntireFile(virtualPath, bytes))
			{
				LOG_ERROR("Script", "Hot reload of '{}' failed. Could not copy to '{}'",
					relative.c_str(), virtualPath.c_str());
				continue;
			}

			LOG_INFO("Script", "Hot reloading '{}'", virtualPath.c_str());
			m_scriptSystem->reloadScript(virtualPath);
		}
	}

	void GameApp::onUpdate(AppContext& ctx, float deltaSeconds)
	{
		m_camera.update(ctx.input, deltaSeconds);

		const sky::SkyState* sky = ctx.services.tryGet<sky::SkyState>();
		if (!sky)
			sky = &m_fallbackSky;

		engine::RenderContext renderCtx = makeRenderContext(ctx);
		m_renderer.update(renderCtx, ctx.ecs, m_camera, *sky);

		pollScriptHotReload(ctx);

		if (m_scriptSystem)
			m_scriptSystem->update(ctx.ecs, deltaSeconds);
	}

	void GameApp::onRender(AppContext& ctx, gfx::ICommandList& cmd)
	{
		engine::RenderContext renderCtx = makeRenderContext(ctx);
		m_renderer.render(renderCtx, cmd, m_camera);
	}

	void GameApp::onShutdown(AppContext& ctx)
	{
		m_scene.shutdown(ctx);
		m_renderer.shutdown();

		m_modelRegistry.shutdown();
		m_modelRegistry.clear();
	}
}
