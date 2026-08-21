#include <sandbox/sandbox_app.h>
#include <sandbox/scene_renderer.h>
#include <core/log/log.h>

namespace imp::app
{
	bool SandboxApp::onInit(AppContext& ctx)
	{
		m_camera.setPosition({ 0.f, 1.f, 4.f });
		m_camera.setYawPitch(math::toRadians(-90.f), 0.f);

		if (!m_resources.init(ctx, m_assets, m_scene.cascadeConfig()))
			return false;

		if (!m_scene.init(ctx, m_assets))
			return false;

		return true;
	}

	void SandboxApp::onUpdate(AppContext& ctx, float deltaSeconds)
	{
		m_camera.update(ctx.input, deltaSeconds);
		m_scene.update(ctx, m_camera);

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
		m_resources.ensureHdrTargetSize(ctx);

		const u32 w = ctx.gfx.backBuffer().width();
		const u32 h = ctx.gfx.backBuffer().height();
		const float aspect = h > 0 ? static_cast<float>( w ) / static_cast<float>( h ) : 1.f;

		m_scene.recomputeCascades(m_camera, aspect);

		SceneRenderParams params{};
		params.camera = &m_camera;
		params.aspect = aspect;
		params.currentFrame = ctx.gfx.currentFrameIndex();
		params.enableFrustumCulling = m_enableFrustumCulling;

		renderShadowCascades(cmd, m_resources, m_scene, params);
		renderHdrPass(cmd, m_resources, m_scene, ctx, params);
		renderTonemapPass(cmd, m_resources, ctx);
	}

	void SandboxApp::onShutdown(AppContext& ctx)
	{
		m_scene.shutdown(ctx);
		m_resources.shutdown();
	}
}
