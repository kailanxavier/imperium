#pragma once
#include <app/iapp.h>
#include <camera/camera.h>
#include <gfx/model_renderer.h>

namespace imp::app
{
	class RenderResources;
	class SandboxScene;

	struct SceneRenderParams
	{
		fwk::Camera* camera = nullptr;
		float aspect = 1.f;
		u32 currentFrame = 0;
		bool enableFrustumCulling = true;
	};

	void renderShadowCascades(gfx::ICommandList& cmd, RenderResources& resources, SandboxScene& scene, const SceneRenderParams& params);
	void renderHdrPass(gfx::ICommandList& cmd, RenderResources& resources, SandboxScene& scene, AppContext& ctx, const SceneRenderParams& params);
	void renderTonemapPass(gfx::ICommandList& cmd, RenderResources& resources, AppContext& ctx);
}
