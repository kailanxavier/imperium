#pragma once
#include <app/iapp.h>
#include <camera/camera.h>
#include <gfx/model_renderer.h>
#include <gfx/cascade_shadow.h>
#include <gfx/render_graph.h>
#include <array>

#include "imgui.h"
#include "render_resources.h"
#include "sandbox_scene.h"

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

	struct ShadowCascadePasses
	{
		std::array<gfx::RGTextureHandle, gfx::kCascadeCount> cascadeDepthTargets;
	};

	ShadowCascadePasses addShadowCascadePasses(gfx::RenderGraph& graph, RenderResources& resources,
		SandboxScene& scene, const SceneRenderParams& params);

	gfx::RGTextureHandle addHdrPass(gfx::RenderGraph& graph, RenderResources& resources,
		SandboxScene& scene, AppContext& ctx,
		const SceneRenderParams& params, const ShadowCascadePasses& shadowPasses, gfx::RGTextureHandle aoTexture);

	void addTonemapPass(gfx::RenderGraph& graph, RenderResources& resources,
		gfx::RGTextureHandle hdrResolve, gfx::IRenderTarget& target, const char* passName);

	struct PrepassOutputs
	{
		gfx::RGTextureHandle normalTarget;
		gfx::RGTextureHandle depthTarget;
	};

	PrepassOutputs addDepthNormalPrepass(gfx::RenderGraph& graph, RenderResources& resources,
		SandboxScene& scene, AppContext& ctx, const SceneRenderParams& params);

	gfx::RGTextureHandle addGTAOPass(gfx::RenderGraph& graph, RenderResources& resources,
		AppContext& ctx, const PrepassOutputs& prepass, const SceneRenderParams& params);

	gfx::RGTextureHandle addBilateralBlurPass(gfx::RenderGraph& graph, RenderResources& resources,
		AppContext& ctx, const PrepassOutputs& prepass, gfx::RGTextureHandle rawAO);
}
