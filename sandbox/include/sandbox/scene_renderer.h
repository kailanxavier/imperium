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
		SandboxScene& scene, AppContext& ctx, const SceneRenderParams& params, 
		const ShadowCascadePasses& shadowPasses, gfx::RGTextureHandle aoTexture, 
		gfx::RGTextureHandle ddgiIrradianceHandle, gfx::RGTextureHandle ddgiDepthHandle, 
		gfx::RGBufferHandle thermalHeatBufferHandle);

	void addTonemapPass(gfx::RenderGraph& graph, RenderResources& resources,
		gfx::RGTextureHandle hdrResolve, gfx::RGTextureHandle bloomTexture, gfx::IRenderTarget& target, 
		const char* passName);

	gfx::RGBufferHandle addThermalUpdatePass(gfx::RenderGraph& graph, RenderResources& resources,
		SandboxScene& scene, AppContext& ctx, const SceneRenderParams& params, gfx::RGBufferHandle ddgiRayBuffer);

	gfx::RGTextureHandle addBloomPasses(gfx::RenderGraph& graph, RenderResources& resources,
		AppContext& ctx, gfx::RGTextureHandle hdrResolve);

	struct PrepassOutputs
	{
		gfx::RGTextureHandle normalTarget;
		gfx::RGTextureHandle depthTarget;
		gfx::RGTextureHandle albedoRoughnessTarget;
	};

	PrepassOutputs addDepthNormalPrepass(gfx::RenderGraph& graph, RenderResources& resources,
		SandboxScene& scene, AppContext& ctx, const SceneRenderParams& params);

	gfx::RGTextureHandle addGTAOPass(gfx::RenderGraph& graph, RenderResources& resources,
		AppContext& ctx, const PrepassOutputs& prepass, const SceneRenderParams& params);

	gfx::RGTextureHandle addBilateralBlurPass(gfx::RenderGraph& graph, RenderResources& resources,
		AppContext& ctx, const PrepassOutputs& prepass, gfx::RGTextureHandle rawAO);

	gfx::RGBufferHandle addDDGIRayTracePass(gfx::RenderGraph& graph, RenderResources& resources, SandboxScene& scene, AppContext& ctx, const SceneRenderParams& params);

	void addDDGIClassifyPass(gfx::RenderGraph& graph, RenderResources& resources, SandboxScene& scene,
		AppContext& ctx, const SceneRenderParams& params, gfx::RGBufferHandle rayBuffer);

	void addDDGIProbeUpdatePass(gfx::RenderGraph& graph, RenderResources& resources, SandboxScene& scene, AppContext& ctx, const SceneRenderParams& params,
		gfx::RGBufferHandle rayBuffer, gfx::RGTextureHandle& outIrradiance, gfx::RGTextureHandle& outDepth);
}
