#include <engine/deferred_renderer.h>

#include "detail/render_passes.h"
#include "detail/render_resources.h"
#include "detail/render_scene.h"

#include <core/log/log.h>
#include <ecs/world.h>

#include <gfx/ao_cvars.h>
#include <gfx/gbuffer_debug_cvars.h>
#include <gfx/render_graph.h>
#include <gfx/taa.h>
#include <gfx/taa_cvars.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace imp::engine
{
	struct DeferredRenderer::Impl
	{
		struct Output
		{
			gfx::IRenderTarget* target = nullptr;
			std::string passName;
		};

		RendererDesc desc;
		RenderResources resources;
		RenderScene scene;

		// A copy because we don't want the renderer holding a pointer belonging to the host
		sky::SkyState sky;

		std::vector<Output> outputs;
		u32 nextOutputId = 0;
		bool initialised = false;
	};

	DeferredRenderer::DeferredRenderer() : m_impl(std::make_unique<Impl>()) {}
	DeferredRenderer::~DeferredRenderer() = default;

	bool DeferredRenderer::init(RenderContext& ctx, const RendererDesc& desc)
	{
		Impl& i = *m_impl;
		if (i.initialised)
			return false;

		if (!desc.modelRegistry)
		{
			LOG_ERROR("Engine", "DeferredRenderer::init() requires RendererDesc::modelRegistry");
			return false;
		}

		i.desc = desc;
		i.scene.init(*desc.modelRegistry);
		i.scene.cascadeConfig() = desc.cascades;

		if (!i.resources.init(ctx, i.desc.manifest, i.scene.cascadeConfig()))
			return false;

		i.initialised = true;
		return true;
	}

	void DeferredRenderer::shutdown()
	{
		Impl& i = *m_impl;
		if (!i.initialised)
			return;

		i.outputs.clear();
		i.scene.shutdown();
		i.resources.shutdown();
		i.initialised = false;
	}

	void DeferredRenderer::update(RenderContext& ctx, ecs::World& world, const fwk::Camera& camera, const sky::SkyState& sky)
	{
		Impl& i = *m_impl;
		if (!i.initialised)
			return;

		i.sky = sky;
		i.resources.pollShaderHotReload(ctx, i.desc.manifest);

		i.scene.update(ctx, world, camera, i.sky, i.desc.features.ddgi);
		const u32 instanceCount = i.scene.instanceCount();
		i.resources.ensureInstanceBufferCapacity(ctx, instanceCount);
		if (i.resources.hasInstanceBuffers() && instanceCount > 0)
		{
			const u32 currentFrame = ctx.gfx.currentFrameIndex();
			const auto& instanceData = i.scene.extraction().instanceData; 
			std::memcpy(i.resources.instanceBuffer(currentFrame).mappedData(),
				instanceData.data(), instanceData.size() * sizeof(math::Mat4f));
		}
	}

	void DeferredRenderer::render(RenderContext& ctx, gfx::ICommandList& cmd, const fwk::Camera& camera)
	{
		Impl& i = *m_impl;
		if (!i.initialised)
			return;

		RenderResources& resources = i.resources;
		RenderScene& scene = i.scene;
		const RenderFeatures& features = i.desc.features;

		const u32 w = ctx.gfx.backBuffer().width();
		const u32 h = ctx.gfx.backBuffer().height();
		const float aspect = h > 0 ? static_cast<float>( w ) / static_cast<float>( h ) : 1.f;

		scene.recomputeCascades(camera, aspect);

		SceneRenderParams params{};
		params.camera = &camera;
		params.sky = &i.sky;
		params.aspect = aspect;
		params.currentFrame = ctx.gfx.currentFrameIndex();
		params.enableFrustumCulling = i.desc.enableFrustumCulling;
		params.features = features;

		params.frameCounter = resources.nextFrameCounter();
		params.taaEnabled = features.taa && gfx::taa::cvarEnabled && w > 0 && h > 0;
		if (params.taaEnabled)
		{
			const u32 sampleCount = static_cast<u32>(std::max<i32>(1, gfx::taa::cvarJitterSamples));
			const math::Vec2f jitterPx = gfx::taa::jitterPixels(params.frameCounter, sampleCount);
			const float scale = gfx::taa::cvarJitterScale;
			params.jitterNdc = gfx::taa::pixelsToNdc(math::Vec2f{ jitterPx.x * scale, jitterPx.y * scale }, w, h);
		}

		gfx::RenderGraph graph(ctx.gfx, resources.graphPool());
		const PrepassOutputs prepass = addDepthNormalPrepass(graph, resources, scene, ctx, params);

		const gfx::RGTextureHandle rawAO = addGTAOPass(graph, resources, ctx, prepass, params);
		const gfx::RGTextureHandle aoTexture = ( features.ao && gfx::ao::cvarBlurEnabled )
			? addBilateralBlurPass(graph, resources, ctx, prepass, rawAO)
			: rawAO;

		gfx::RGTextureHandle ssgiTexture{};
		if (features.ssgi)
		{
			const gfx::RGTextureHandle rawSSGI = addSSGIPass(graph, resources, ctx, prepass, params);
			ssgiTexture = addSSGIBlurPass(graph, resources, ctx, prepass, rawSSGI);
		}

		if (gfx::gbufferdebug::cvarMode > 0)
		{
			addGBufferDebugPass(graph, resources, ctx, prepass, ctx.gfx.backBuffer(), ssgiTexture);
			if (!graph.compile())
			{
				LOG_ERROR("Engine", "RenderGraph::compile() failed");
				return;
			}

			graph.execute(cmd);
			return;
		}

		const ShadowCascadePasses shadowPasses = addShadowCascadePasses(graph, resources, scene, params);

		gfx::RGTextureHandle ddgiIrradianceHandle{};
		gfx::RGTextureHandle ddgiDepthHandle{};
		gfx::RGBufferHandle ddgiRayBuffer{}; 
		if (features.ddgi)
		{
			ddgiRayBuffer = addDDGIRayTracePass(graph, resources, scene, ctx, params);
			addDDGIClassifyPass(graph, resources, scene, ctx, params, ddgiRayBuffer);
			addDDGIProbeUpdatePass(graph, resources, scene, ctx, params, ddgiRayBuffer, ddgiIrradianceHandle, ddgiDepthHandle);
		}

		gfx::RGBufferHandle thermalBuffer{};
		if (features.thermal)
			thermalBuffer = addThermalUpdatePass(graph, resources, scene, ctx, params, ddgiRayBuffer);

		const gfx::RGTextureHandle hdrColour = addDeferredLightingPass(graph, resources, ctx, params, prepass,
			shadowPasses, aoTexture, ddgiIrradianceHandle, ddgiDepthHandle, thermalBuffer, ssgiTexture);

		const gfx::RGTextureHandle hdrLit = addHdrPass(graph, resources, scene, ctx, params, shadowPasses,
			prepass.depthTarget, hdrColour, aoTexture, ddgiIrradianceHandle, ddgiDepthHandle, thermalBuffer, ssgiTexture);

		const gfx::RGTextureHandle taaResolved = addTaaResolvePass(graph, resources, ctx, params, prepass, hdrLit);

		const gfx::RGTextureHandle hdrResolve = addOverlayPass(graph, resources, ctx, params,
			taaResolved, prepass.depthTarget, ddgiIrradianceHandle, ddgiDepthHandle, ddgiRayBuffer);

		gfx::RGTextureHandle bloomTexture{};
		if (features.bloom)
			bloomTexture = addBloomPasses(graph, resources, ctx, hdrResolve);

		addTonemapPass(graph, resources, hdrResolve, bloomTexture, ctx.gfx.backBuffer(), "Tonemap");

		for (const Impl::Output& output : i.outputs)
			addTonemapPass(graph, resources, hdrResolve, bloomTexture, *output.target, output.passName.c_str());

		if (!graph.compile())
		{
			LOG_ERROR("Engine", "RenderGraph::compile() failed");
			return;
		}

#ifndef NDEBUG
		static constexpr int s_framesToDump = 3;
		static int framesDumped = 0;
		if (framesDumped < s_framesToDump)
		{
			LOG_DEBUG("Engine", "{}", graph.debugDump());
			framesDumped++;
		}
#endif

		graph.execute(cmd);
	}

	void DeferredRenderer::addOutput(gfx::IRenderTarget* target)
	{
		if (!target)
			return;

		Impl& i = *m_impl;
		const auto existing = std::ranges::find(i.outputs, target, &Impl::Output::target);
		if (existing != i.outputs.end())
			return;

		i.outputs.push_back({ target, "Tonemap Output " + std::to_string(i.nextOutputId++) });
	}

	void DeferredRenderer::removeOutput(gfx::IRenderTarget* target)
	{
		auto& outputs = m_impl->outputs;
		std::erase_if(outputs, [target](const Impl::Output& output) { return output.target == target; });
	}

	void DeferredRenderer::clearOutputs()
	{
		m_impl->outputs.clear();
	}

	RenderFeatures& DeferredRenderer::features() { return m_impl->desc.features; }
	const RenderFeatures& DeferredRenderer::features() const { return m_impl->desc.features; }

	gfx::CascadeConfig& DeferredRenderer::cascadeConfig() { return m_impl->scene.cascadeConfig(); }
	const gfx::CascadeConfig& DeferredRenderer::cascadeConfig() const { return m_impl->scene.cascadeConfig(); }

	gfx::TextureFormat DeferredRenderer::hdrColourFormat() const { return m_impl->resources.hdrColourFormat(); }
	gfx::TextureFormat DeferredRenderer::hdrDepthFormat() const { return m_impl->resources.hdrDepthFormat(); }

	bool DeferredRenderer::initialised() const { return m_impl->initialised; }
}
