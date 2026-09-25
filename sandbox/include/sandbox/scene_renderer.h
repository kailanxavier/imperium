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

		bool taaEnabled = false;
		math::Vec2f jitterNdc{ 0.f, 0.f };
		u32 frameCounter = 0;
	};

	struct ShadowCascadePasses
	{
		std::array<gfx::RGTextureHandle, gfx::kCascadeCount> cascadeDepthTargets;
	};

	ShadowCascadePasses addShadowCascadePasses(gfx::RenderGraph& graph, RenderResources& resources,
		SandboxScene& scene, const SceneRenderParams& params);

	struct PrepassOutputs
	{
		gfx::RGTextureHandle normalTarget;
		gfx::RGTextureHandle depthTarget;
		gfx::RGTextureHandle albedoRoughnessTarget;
		gfx::RGTextureHandle velocityTarget;

		math::Mat4f viewProj = math::Mat4f::identity();
		math::Mat4f prevViewProj = math::Mat4f::identity();
	};

	PrepassOutputs addDepthNormalPrepass(gfx::RenderGraph& graph, RenderResources& resources,
		SandboxScene& scene, AppContext& ctx, const SceneRenderParams& params);

	void addGBufferDebugPass(gfx::RenderGraph& graph, RenderResources& resources,
		AppContext& ctx, const PrepassOutputs& prepass, gfx::IRenderTarget& target, gfx::RGTextureHandle ssgiTexture);

	gfx::RGTextureHandle addGTAOPass(gfx::RenderGraph& graph, RenderResources& resources,
		AppContext& ctx, const PrepassOutputs& prepass, const SceneRenderParams& params);

	gfx::RGTextureHandle addBilateralBlurPass(gfx::RenderGraph& graph, RenderResources& resources,
		AppContext& ctx, const PrepassOutputs& prepass, gfx::RGTextureHandle rawAO);

	gfx::RGTextureHandle addSSGIPass(gfx::RenderGraph& graph, RenderResources& resources,
		AppContext& ctx, const PrepassOutputs& prepass, const SceneRenderParams& params);

	gfx::RGTextureHandle addSSGIBlurPass(gfx::RenderGraph& graph, RenderResources& resources,
		AppContext& ctx, const PrepassOutputs& prepass, gfx::RGTextureHandle rawSSGI);

	gfx::RGTextureHandle addDeferredLightingPass(gfx::RenderGraph& graph, RenderResources& resources, AppContext& ctx,
		const SceneRenderParams& params, const PrepassOutputs& prepass, const ShadowCascadePasses& shadowPasses,
		gfx::RGTextureHandle aoTexture, gfx::RGTextureHandle ddgiIrradianceHandle, gfx::RGTextureHandle ddgiDepthHandle,
		gfx::RGBufferHandle thermalHeatBufferHandle, gfx::RGTextureHandle ssgiTexture);

	gfx::RGTextureHandle addHdrPass(gfx::RenderGraph& graph, RenderResources& resources, SandboxScene& scene, AppContext& ctx, 
		const SceneRenderParams& params, const ShadowCascadePasses& shadowPasses, gfx::RGTextureHandle prepassDepth, 
		gfx::RGTextureHandle hdrColourIn, gfx::RGTextureHandle aoTexture,
		gfx::RGTextureHandle ddgiIrradianceHandle, gfx::RGTextureHandle ddgiDepthHandle, 
		gfx::RGBufferHandle thermalHeatBufferHandle, gfx::RGTextureHandle ssgiTexture);

	gfx::RGTextureHandle addTaaResolvePass(gfx::RenderGraph& graph, RenderResources& resources, AppContext& ctx,
		const SceneRenderParams& params, const PrepassOutputs& prepass, gfx::RGTextureHandle hdrColour);

	gfx::RGTextureHandle addOverlayPass(gfx::RenderGraph& graph, RenderResources& resources, AppContext& ctx,
		const SceneRenderParams& params, gfx::RGTextureHandle colourIn, gfx::RGTextureHandle depthIn,
		gfx::RGTextureHandle ddgiIrradianceHandle, gfx::RGTextureHandle ddgiDepthHandle, gfx::RGBufferHandle ddgiRayBuffer);

	void addTonemapPass(gfx::RenderGraph& graph, RenderResources& resources,
		gfx::RGTextureHandle hdrResolve, gfx::RGTextureHandle bloomTexture, gfx::IRenderTarget& target, 
		const char* passName);

	gfx::RGBufferHandle addThermalUpdatePass(gfx::RenderGraph& graph, RenderResources& resources,
		SandboxScene& scene, AppContext& ctx, const SceneRenderParams& params, gfx::RGBufferHandle ddgiRayBuffer);

	gfx::RGTextureHandle addBloomPasses(gfx::RenderGraph& graph, RenderResources& resources,
		AppContext& ctx, gfx::RGTextureHandle hdrResolve);

	gfx::RGBufferHandle addDDGIRayTracePass(gfx::RenderGraph& graph, RenderResources& resources, SandboxScene& scene, AppContext& ctx, const SceneRenderParams& params);

	void addDDGIClassifyPass(gfx::RenderGraph& graph, RenderResources& resources, SandboxScene& scene,
		AppContext& ctx, const SceneRenderParams& params, gfx::RGBufferHandle rayBuffer);

	void addDDGIProbeUpdatePass(gfx::RenderGraph& graph, RenderResources& resources, SandboxScene& scene, AppContext& ctx, const SceneRenderParams& params,
		gfx::RGBufferHandle rayBuffer, gfx::RGTextureHandle& outIrradiance, gfx::RGTextureHandle& outDepth);
}
