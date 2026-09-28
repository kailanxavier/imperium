#include <game/game_app.h>
#include <engine/scene_renderer.h>

#include <core/log/log.h>
#include <fstream>
#include <filesystem>
#include <memory>
#include <algorithm>

#include <gfx/render_graph.h>
#include <gfx/ao_cvars.h>
#include <gfx/gbuffer_debug_cvars.h>
#include <gfx/taa.h>
#include <gfx/taa_cvars.h>

using namespace imp::engine;

namespace imp::game
{
	void GameApp::onRegisterServices(AppContext& ctx)
	{
		m_targetFormats = { 
			m_resources.hdrColourFormat(), 
			m_resources.hdrDepthFormat(), 
			engine::RenderResources::kMsaaSampleCount };

		m_lightRefs = { &m_scene.sunDirection(), &m_scene.cascadeConfig() };

		ctx.services.provide(m_camera);
		ctx.services.provide(m_scene.modelRegistry());
		ctx.services.provide(m_targetFormats);
		ctx.services.provide(m_lightRefs);
	}

	bool GameApp::onInit(AppContext& ctx)
	{
		m_camera.setPosition({ 0.f, 1.f, 0.f });
		m_camera.setYawPitch(math::toRadians(90.f), 0.f);

		if (!m_resources.init(ctx, m_rendererManifest, m_scene.cascadeConfig()))
			return false;

		if (!m_scene.init(ctx, m_assets))
			return false;

		m_scriptSystem = std::make_unique<script::ScriptSystem>(ctx.vfs);

		m_scriptSourceWatcher = std::make_unique<fs::DirectoryWatcher>(
			std::filesystem::path(IMP_SCRIPT_SOURCE_DIR), std::vector<std::string>{ ".lua" });

		if (m_scriptSourceWatcher->isValid())
			LOG_INFO("Game", "Script hot reload active");

		return true;
	}

	void GameApp::pollScriptHotReload(AppContext &ctx)
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
		m_scene.update(ctx, m_camera);

		m_resources.pollShaderHotReload(ctx, m_rendererManifest);
		pollScriptHotReload(ctx);

		if (m_scriptSystem)
			m_scriptSystem->update(ctx.ecs, deltaSeconds);

		m_resources.ensureInstanceBufferCapacity(ctx, m_scene.instanceCount());
		if (m_resources.hasInstanceBuffers() && m_scene.instanceCount() > 0)
		{
			const u32 currentFrame = ctx.gfx.currentFrameIndex();
			std::memcpy(m_resources.instanceBuffer(currentFrame).mappedData(),
				m_scene.extraction().instanceData.data(),
				m_scene.extraction().instanceData.size() * sizeof(math::Mat4f));
		}
	}

	void GameApp::onRender(AppContext& ctx, gfx::ICommandList& cmd)
	{
		const u32 w = ctx.gfx.backBuffer().width();
		const u32 h = ctx.gfx.backBuffer().height();
		const float aspect = h > 0 ? static_cast<float>( w ) / static_cast<float>( h ) : 1.f;

		m_scene.recomputeCascades(m_camera, aspect);

		SceneRenderParams params{};
		params.camera = &m_camera;
		params.aspect = aspect;
		params.currentFrame = ctx.gfx.currentFrameIndex();
		params.enableFrustumCulling = m_enableFrustumCulling;

		params.frameCounter = m_resources.nextFrameCounter();
		params.taaEnabled = gfx::taa::cvarEnabled && w > 0 && h > 0;
		if (params.taaEnabled)
		{
			const u32 sampleCount = static_cast<u32>(std::max<i32>(1, gfx::taa::cvarJitterSamples));
			const math::Vec2f jitterPx = gfx::taa::jitterPixels(params.frameCounter, sampleCount);
			const float scale = gfx::taa::cvarJitterScale;
			params.jitterNdc = gfx::taa::pixelsToNdc(math::Vec2f{jitterPx.x * scale, jitterPx.y * scale}, w, h);
		}

		gfx::RenderGraph graph(ctx.gfx, m_resources.graphPool());

		const PrepassOutputs prepass = addDepthNormalPrepass(graph, m_resources, m_scene, ctx, params);

		const gfx::RGTextureHandle rawAO = addGTAOPass(graph, m_resources, ctx, prepass, params);
		const gfx::RGTextureHandle aoTexture = gfx::ao::cvarBlurEnabled
			? addBilateralBlurPass(graph, m_resources, ctx, prepass, rawAO)
			: rawAO;

		const gfx::RGTextureHandle rawSSGI = addSSGIPass(graph, m_resources, ctx, prepass, params);
		const gfx::RGTextureHandle ssgiTexture = addSSGIBlurPass(graph, m_resources, ctx, prepass, rawSSGI);

		if (gfx::gbufferdebug::cvarMode > 0)
		{
			addGBufferDebugPass(graph, m_resources, ctx, prepass, ctx.gfx.backBuffer(), ssgiTexture);
			if (!graph.compile())
			{
				LOG_ERROR("Game", "RenderGraph::compile() failed");
				return;
			}

			graph.execute(cmd);
			return;
		}
		
		const ShadowCascadePasses shadowPasses = addShadowCascadePasses(graph, m_resources, m_scene, params);

		gfx::RGTextureHandle ddgiIrradianceHandle{};
		gfx::RGTextureHandle ddgiDepthHandle{};

		const gfx::RGBufferHandle ddgiRayBuffer = addDDGIRayTracePass(graph, m_resources, m_scene, ctx, params);
		addDDGIClassifyPass(graph, m_resources, m_scene, ctx, params, ddgiRayBuffer);
		addDDGIProbeUpdatePass(graph, m_resources, m_scene, ctx, params, ddgiRayBuffer, ddgiIrradianceHandle, ddgiDepthHandle);

		gfx::RGBufferHandle thermalBuffer = addThermalUpdatePass(graph, m_resources, m_scene, ctx, params, ddgiRayBuffer);

		const gfx::RGTextureHandle hdrColour = addDeferredLightingPass(graph, m_resources, ctx, params, prepass,
			shadowPasses, aoTexture, ddgiIrradianceHandle, ddgiDepthHandle, thermalBuffer, ssgiTexture);

		const gfx::RGTextureHandle hdrLit = addHdrPass(graph, m_resources, m_scene, ctx, params, shadowPasses,
			prepass.depthTarget, hdrColour, aoTexture, ddgiIrradianceHandle, ddgiDepthHandle, thermalBuffer, ssgiTexture);

		const gfx::RGTextureHandle taaResolved = addTaaResolvePass(graph, m_resources, ctx, params, prepass, hdrLit);

		const gfx::RGTextureHandle hdrResolve = addOverlayPass(graph, m_resources, ctx, params,
			taaResolved, prepass.depthTarget, ddgiIrradianceHandle, ddgiDepthHandle, ddgiRayBuffer);

		gfx::RGTextureHandle bloomTexture = addBloomPasses(graph, m_resources, ctx, hdrResolve);

		addTonemapPass(graph, m_resources, hdrResolve, bloomTexture, ctx.gfx.backBuffer(), "Tonemap");
		if (m_readbackTarget)
			addTonemapPass(graph, m_resources, hdrResolve, bloomTexture, *m_readbackTarget, "Tonemap Readback");

		if (!graph.compile())
		{
			LOG_ERROR("Game", "RenderGraph::compile() failed");
			return;
		}

#ifndef NDEBUG
		static constexpr int s_framesToDump = 3;
		static int framesDumped = 0;
		if (framesDumped < s_framesToDump)
		{
			LOG_DEBUG("Vulkan", "{}", graph.debugDump());
			framesDumped++;
		}
#endif

		graph.execute(cmd);
	}

	void GameApp::onShutdown(AppContext& ctx)
	{
		m_scene.shutdown(ctx);
		m_resources.shutdown();
	}
}
