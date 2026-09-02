#include <sandbox/sandbox_app.h>
#include <sandbox/scene_renderer.h>
#include <gfx/render_graph.h>
#include <core/log/log.h>

#include <gfx/shader_compiler.h>
#include <fstream>

#include <filesystem>
#include <memory>

#include <gfx/ao_cvars.h>

namespace imp::app
{
	bool SandboxApp::onInit(AppContext& ctx)
	{
		m_camera.setPosition({ 0.f, 1.f, 0.f });
		m_camera.setYawPitch(math::toRadians(90.f), 0.f);

		if (!m_resources.init(ctx, m_assets, m_scene.cascadeConfig()))
			return false;

		if (!m_scene.init(ctx, m_assets))
			return false;

		m_scriptSystem = std::make_unique<script::ScriptSystem>(ctx.vfs);

		m_scriptSourceWatcher = std::make_unique<fs::DirectoryWatcher>(
			std::filesystem::path(IMP_SCRIPT_SOURCE_DIR), std::vector<std::string>{ ".lua" });

		gfx::ShaderCompiler compiler(IMP_SHADER_COMPILER_PATH, IMP_SHADER_COMPILER_IS_GLSLANG_VALIDATOR != 0);
		const std::string shaderOutputDir = ctx.vfs.resolvePhysicalPath("assets/shaders/", true);
		m_shaderWatcher = std::make_unique<gfx::ShaderHotReloadWatcher>(
			std::filesystem::path(IMP_SHADER_SOURCE_DIR), std::filesystem::path(shaderOutputDir), std::move(compiler));

		if (m_scriptSourceWatcher->isValid() || m_shaderWatcher->isValid())
			LOG_INFO("Sandbox", "Hot reload active (scripts: {}, shaders: {})",
				m_scriptSourceWatcher->isValid(), m_shaderWatcher->isValid());

		return true;
	}

	void SandboxApp::pollShaderHotReload(AppContext &ctx)
	{
		if (!m_shaderWatcher || !m_shaderWatcher->isValid())
			return;

		if (m_shaderWatcher->poll())
			m_resources.reloadShaders(ctx, m_assets);
	}

	void SandboxApp::pollScriptHotReload(AppContext &ctx)
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

	void SandboxApp::onUpdate(AppContext& ctx, float deltaSeconds)
	{
		m_camera.update(ctx.input, deltaSeconds);
		m_scene.update(ctx, m_camera);

		pollShaderHotReload(ctx);
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

	void SandboxApp::onRender(AppContext& ctx, gfx::ICommandList& cmd)
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

		gfx::RenderGraph graph(ctx.gfx, m_resources.graphPool());

		const PrepassOutputs prepass = addDepthNormalPrepass(graph, m_resources, m_scene, ctx, params);
		const gfx::RGTextureHandle rawAO = addGTAOPass(graph, m_resources, ctx, prepass, params);
		const gfx::RGTextureHandle aoTexture = gfx::ao::cvarBlurEnabled
			? addBilateralBlurPass(graph, m_resources, ctx, prepass, rawAO)
			: rawAO;

		const ShadowCascadePasses shadowPasses = addShadowCascadePasses(graph, m_resources, m_scene, params);
		const gfx::RGTextureHandle hdrResolve = addHdrPass(graph, m_resources, m_scene, ctx, params, shadowPasses, aoTexture);

		addTonemapPass(graph, m_resources, hdrResolve, ctx.gfx.backBuffer(), "Tonemap");
		if (m_readbackTarget)
			addTonemapPass(graph, m_resources, hdrResolve, *m_readbackTarget, "Tonemap Readback");

		if (!graph.compile())
		{
			LOG_ERROR("Sandbox", "RenderGraph::compile() failed");
			return;
		}

		graph.execute(cmd);
	}

	void SandboxApp::onShutdown(AppContext& ctx)
	{
		m_scene.shutdown(ctx);
		m_resources.shutdown();
	}
}
