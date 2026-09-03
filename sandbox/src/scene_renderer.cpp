#include <sandbox/scene_renderer.h>
#include <sandbox/render_resources.h>
#include <sandbox/sandbox_scene.h>
#include <gfx/render_extraction.h>
#include <gfx/lighting.h>
#include <cstdio>
#include <gfx/ao.h>
#include <gfx/ao_cvars.h>

namespace imp::app
{
	namespace
	{
		struct ShadowCascadePassData
		{
			gfx::RGTextureHandle target;

			gfx::IBuffer* instanceBuffer = nullptr;
			math::Mat4f viewProj;
			gfx::CullVolume cullVolume;

			RenderResources* resources = nullptr;
			SandboxScene* scene = nullptr;
		};

		struct HdrPassData
		{
			gfx::RGTextureHandle hdrColour;
			gfx::RGTextureHandle hdrDepth;
			gfx::RGTextureHandle hdrResolve;
			gfx::RGBufferHandle lightUBO;
			gfx::RGBufferHandle cascadeUBO;
			gfx::RGTextureHandle shadowArray;
			gfx::RGTextureHandle aoTexture;
			gfx::RGBufferHandle screenParamsUBO;

			RenderResources* resources = nullptr;
			SandboxScene* scene = nullptr;
			AppContext* ctx = nullptr;
			SceneRenderParams params{};
		};

		struct TonemapPassData
		{
			gfx::RGTextureHandle hdrResolve;
			gfx::RGTextureHandle output;
			RenderResources* resources = nullptr;
		};

		struct PrepassData
		{
			gfx::RGTextureHandle normalTarget;
			gfx::RGTextureHandle albedoRoughnessTarget;
			gfx::RGTextureHandle depthTarget;

			gfx::IBuffer* instanceBuffer = nullptr;
			math::Mat4f viewProj;

			RenderResources* resources = nullptr;
			SandboxScene* scene = nullptr;
		};

		struct GTAOPassData
		{
			gfx::RGTextureHandle depthIn;
			gfx::RGTextureHandle normalIn;
			gfx::RGTextureHandle aoOut;
			gfx::RGBufferHandle paramsUBO;

			RenderResources* resources = nullptr;
		};

		struct BlurPassData
		{
			gfx::RGTextureHandle aoIn;
			gfx::RGTextureHandle depthIn;
			gfx::RGTextureHandle normalIn;
			gfx::RGTextureHandle blurredOut;
			gfx::RGBufferHandle paramsUBO;

			float texelSizeX = 0.f, texelSizeY = 0.f;
			RenderResources* resources = nullptr;
		};
	}

	ShadowCascadePasses addShadowCascadePasses(gfx::RenderGraph &graph, RenderResources &resources, SandboxScene &scene, const SceneRenderParams &params)
	{
		const auto& cascades = scene.cascades();

		gfx::CascadeUBO cascadeData{};
		for (u32 i = 0; i < gfx::kCascadeCount; ++i)
		{
			cascadeData.viewProj[i] = cascades[i].viewProj;
			cascadeData.splitDepths[i] = cascades[i].splitDepth;
		}
		cascadeData.blendParams = math::Vec4f{ params.camera->nearPlane, scene.cascadeConfig().blendFraction, 0.f, 0.f };
		resources.cascadeUBO(params.currentFrame).update(&cascadeData, sizeof(cascadeData), 0);
		resources.lightUBO(params.currentFrame).update(&scene.extraction().lightData, sizeof(gfx::LightUBO), 0);

		ShadowCascadePasses out{};

		for (u32 i = 0; i < gfx::kCascadeCount; ++i)
		{
			char name[32];
			std::snprintf(name, sizeof(name), "Shadow Cascade %u", i);

			const auto& data = graph.addPass<ShadowCascadePassData>(name,
				[&, i](gfx::RenderGraphBuilder& b, ShadowCascadePassData& d)
				{
					d.target = b.importTexture(name, &resources.shadowCascadeTarget(i));
					d.target = b.writeDepth(d.target, gfx::RGLoadOp::Clear, 1.f);

					d.instanceBuffer = &resources.instanceBuffer(params.currentFrame);
					d.viewProj = cascades[i].viewProj;
					d.cullVolume = gfx::CullVolume{ cascades[i].lightView, cascades[i].boxMin, cascades[i].boxMax };
					d.resources = &resources;
					d.scene = &scene;
				},
				[](const ShadowCascadePassData& d, gfx::RenderGraphContext& rgCtx)
				{
					gfx::ModelRenderContext shadowRenderCtx{};
					shadowRenderCtx.cmd = &rgCtx.cmd();
					shadowRenderCtx.modelRegistry = &d.scene->modelRegistry();
					shadowRenderCtx.sampler = &d.resources->sampler();
					shadowRenderCtx.lightBuffer = nullptr;
					shadowRenderCtx.instanceBuffer = d.instanceBuffer;
					shadowRenderCtx.viewProj = d.viewProj;
					shadowRenderCtx.cullVolume = &d.cullVolume;

					rgCtx.cmd().bindPipeline(d.resources->shadowPipeline());
					drawModelBatches(shadowRenderCtx, d.scene->extraction());
				});

			out.cascadeDepthTargets[i] = data.target;
		}

		return out;
	}

	gfx::RGTextureHandle addHdrPass(gfx::RenderGraph &graph, RenderResources &resources, SandboxScene &scene, AppContext &ctx, const SceneRenderParams &params, const ShadowCascadePasses &shadowPasses, gfx::RGTextureHandle aoTexture)
	{
		const auto& data = graph.addPass<HdrPassData>("HDR",
			[&](gfx::RenderGraphBuilder& b, HdrPassData& d)
			{
				const u32 w = ctx.gfx.backBuffer().width();
				const u32 h = ctx.gfx.backBuffer().height();

				gfx::TextureDesc colourDesc{};
				colourDesc.width = w; colourDesc.height = h;
				colourDesc.format = resources.hdrColourFormat();
				colourDesc.sampleCount = RenderResources::kMsaaSampleCount;
				colourDesc.usage = gfx::TextureUsage::RenderTarget;
				d.hdrColour = b.createTexture("HdrColour", colourDesc);

				gfx::TextureDesc depthDesc = colourDesc;
				depthDesc.format = resources.hdrDepthFormat();
				depthDesc.usage = gfx::TextureUsage::DepthStencil;
				d.hdrDepth = b.createTexture("HdrDepth", depthDesc);

				gfx::TextureDesc resolveDesc = colourDesc;
				resolveDesc.sampleCount = gfx::SampleCount::One;
				resolveDesc.usage = gfx::TextureUsage::RenderTarget | gfx::TextureUsage::Sampled;
				d.hdrResolve = b.createTexture("HdrResolve", resolveDesc);

				d.hdrColour = b.writeColour(d.hdrColour, gfx::RGLoadOp::Clear, { /* default ClearColour */ });
				d.hdrDepth = b.writeDepth(d.hdrDepth, gfx::RGLoadOp::Clear);
				d.hdrResolve = b.writeResolve(d.hdrResolve, d.hdrColour);

				d.lightUBO = b.readBuffer(b.importBuffer("LightUBO", &resources.lightUBO(params.currentFrame)));
				d.cascadeUBO = b.readBuffer(b.importBuffer("CascadeUBO", &resources.cascadeUBO(params.currentFrame)));

				d.shadowArray = b.readTexture(b.importTexture("ShadowArray", resources.shadowArrayTexture()));
				for (const auto& cascadeTarget : shadowPasses.cascadeDepthTargets)
					b.readTexture(cascadeTarget);

				d.aoTexture = b.readTexture(aoTexture);
				d.screenParamsUBO = b.readBuffer(b.importBuffer("ScreenParamsUBO", &resources.screenParamsUBO(params.currentFrame)));

				gfx::ScreenParamsUBO screenParams{};
				screenParams.resolutionAndInv = { static_cast<float>(w), static_cast<float>(h),
					1.f / static_cast<float>(w), 1.f / static_cast<float>(h) };
				screenParams.flags = { gfx::ao::cvarEnabled ? 1.f : 0.f, 0.f, 0.f, 0.f };
				resources.screenParamsUBO(params.currentFrame).update(&screenParams, sizeof(screenParams), 0);

				d.resources = &resources;
				d.scene = &scene;
				d.ctx = &ctx;
				d.params = params;
			},
			[](const HdrPassData& d, gfx::RenderGraphContext& rgCtx)
			{
				gfx::CullVolume mainCullVolume{};

				gfx::ModelRenderContext renderCtx{};
				renderCtx.cmd = &rgCtx.cmd();
				renderCtx.modelRegistry = &d.scene->modelRegistry();
				renderCtx.sampler = &d.resources->sampler();
				renderCtx.lightBuffer = &rgCtx.buffer(d.lightUBO);
				renderCtx.instanceBuffer = &d.resources->instanceBuffer(d.params.currentFrame);
				renderCtx.viewProj = d.params.camera->projection(d.params.aspect) * d.params.camera->view();
				renderCtx.shadowArrayTexture = &rgCtx.texture(d.shadowArray);
				renderCtx.cascadeBuffer = &rgCtx.buffer(d.cascadeUBO);
				renderCtx.shadowSampler = &d.resources->shadowSampler();
				renderCtx.aoTexture = &rgCtx.texture(d.aoTexture);
				renderCtx.screenParamsBuffer = &rgCtx.buffer(d.screenParamsUBO);

				if (d.params.enableFrustumCulling)
				{
					mainCullVolume.useFrustum = true;
					mainCullVolume.frustumPlanes = gfx::extractFrustumPlanes(renderCtx.viewProj);
					renderCtx.cullVolume = &mainCullVolume;
				}

				rgCtx.cmd().bindPipeline(d.resources->meshPipeline());
				drawModelBatches(renderCtx, d.scene->extraction());

				gfx::SkyPushConstants skyPC{};
				skyPC.invViewProj = math::inverse(renderCtx.viewProj);
				skyPC.cameraPositionWS = math::Vec4f{ d.params.camera->position(), 1.f };
				skyPC.sunDirAndIntensity = math::Vec4f{ -d.scene->sunDirection(), 10.f };

				rgCtx.cmd().bindPipeline(d.resources->skyPipeline());
				rgCtx.cmd().pushConstants(&skyPC, sizeof(skyPC));
				rgCtx.cmd().draw(3, 1);

				if (!d.scene->extraction().blendInstances.empty())
				{
					rgCtx.cmd().bindPipeline(d.resources->blendPipeline());
					drawBlendInstances(renderCtx, d.scene->extraction());
				}

				d.ctx->layers.renderAll(rgCtx.cmd());
			});

		return data.hdrResolve;
	}

	void addTonemapPass(gfx::RenderGraph &graph, RenderResources &resources, gfx::RGTextureHandle hdrResolve, gfx::IRenderTarget &target, const char *passName)
	{
		graph.addPass<TonemapPassData>(passName,
			[&](gfx::RenderGraphBuilder& b, TonemapPassData& d)
			{
				d.hdrResolve = b.readTexture(hdrResolve);

				d.output = b.importTexture(passName, &target);
				d.output = b.writeColour(d.output, gfx::RGLoadOp::DontCare);

				d.resources = &resources;

				b.hasSideEffect();
			},
			[](const TonemapPassData& d, gfx::RenderGraphContext& rgCtx)
			{
				rgCtx.cmd().bindPipeline(d.resources->tonemapPipeline());
				rgCtx.cmd().bindTexture(rgCtx.texture(d.hdrResolve), d.resources->sampler(), 1);
				rgCtx.cmd().draw(3, 1);
			});
	}

	PrepassOutputs addDepthNormalPrepass(gfx::RenderGraph &graph, RenderResources &resources, SandboxScene &scene, AppContext &ctx, const SceneRenderParams &params)
	{
		const auto& data = graph.addPass<PrepassData>("DepthNormalPrepass",
			[&](gfx::RenderGraphBuilder& b, PrepassData& d)
			{
				const u32 w = ctx.gfx.backBuffer().width();
			const u32 h = ctx.gfx.backBuffer().height();

			gfx::TextureDesc normalDesc{};
			normalDesc.width = w; normalDesc.height = h;
			normalDesc.format = gfx::TextureFormat::RGBA16Float;
			normalDesc.sampleCount = gfx::SampleCount::One;
			normalDesc.usage = gfx::TextureUsage::RenderTarget | gfx::TextureUsage::Sampled;
			d.normalTarget = b.createTexture("PrepassNormal", normalDesc);

			gfx::TextureDesc albedoDesc = normalDesc;
			albedoDesc.format = gfx::TextureFormat::RGBA8Unorm;
			d.albedoRoughnessTarget = b.createTexture("PrepassAlbedoRoughness", albedoDesc);

			gfx::TextureDesc depthDesc{};
			depthDesc.width = w; depthDesc.height = h;
			depthDesc.format = gfx::TextureFormat::Depth32Float;
			depthDesc.sampleCount = gfx::SampleCount::One;
			depthDesc.usage = gfx::TextureUsage::DepthStencil | gfx::TextureUsage::Sampled;
			d.depthTarget = b.createTexture("PrepassDepth", depthDesc);

			d.normalTarget = b.writeColour(d.normalTarget, gfx::RGLoadOp::Clear, { 1.f, 1.f, 1.f, 1.f });
			d.albedoRoughnessTarget = b.writeColour(d.albedoRoughnessTarget, gfx::RGLoadOp::Clear, { 0.f, 0.f, 0.f, 0.f });
			d.depthTarget = b.writeDepth(d.depthTarget, gfx::RGLoadOp::Clear, 1.f);

			d.instanceBuffer = &resources.instanceBuffer(params.currentFrame);
			d.viewProj = params.camera->projection(params.aspect) * params.camera->view();
			d.resources = &resources;
			d.scene = &scene;
			},
			[](const PrepassData& d, gfx::RenderGraphContext& rgCtx)
			{
				gfx::ModelRenderContext prepassCtx{};
				prepassCtx.cmd = &rgCtx.cmd();
				prepassCtx.modelRegistry = &d.scene->modelRegistry();
				prepassCtx.instanceBuffer = d.instanceBuffer;
				prepassCtx.viewProj = d.viewProj;
				prepassCtx.sampler = &d.resources->sampler();
				prepassCtx.alphaTestOnly = true;
				// no light or shadow bindings needed, this is a normals only pass

				rgCtx.cmd().bindPipeline(d.resources->prepassPipeline());
				drawModelBatches(prepassCtx, d.scene->extraction());
			});

		return { data.normalTarget, data.depthTarget, data.albedoRoughnessTarget };
	}

	gfx::RGTextureHandle addGTAOPass(gfx::RenderGraph &graph, RenderResources &resources, AppContext& ctx, const PrepassOutputs &prepass, const SceneRenderParams &params)
	{
		gfx::AOParamsUBO cpuParams{};
		cpuParams.invProj = math::inverse(params.camera->projection(params.aspect));
		cpuParams.invView = math::inverse(params.camera->view());
		cpuParams.view = params.camera->view();
		cpuParams.params = { gfx::ao::cvarRadius, gfx::ao::cvarIntensity,
			static_cast<float>(gfx::ao::cvarSliceCount.operator i32()), static_cast<float>(gfx::ao::cvarStepCount.operator i32()) };
		cpuParams.params2 = { gfx::ao::cvarThickness, gfx::ao::cvarPower,
			static_cast<float>(ctx.gfx.backBuffer().width()), static_cast<float>(ctx.gfx.backBuffer().height()) };
		resources.aoParamsUBO(params.currentFrame).update(&cpuParams, sizeof(cpuParams), 0);

		const auto& data = graph.addPass<GTAOPassData>("GTAO",
			[&](gfx::RenderGraphBuilder& b, GTAOPassData& d)
			{
				d.depthIn = b.readTexture(prepass.depthTarget);
				d.normalIn = b.readTexture(prepass.normalTarget);
				d.paramsUBO = b.readBuffer(b.importBuffer("AOParamsUBO", &resources.aoParamsUBO(params.currentFrame)));

				gfx::TextureDesc aoDesc{};
				aoDesc.width = ctx.gfx.backBuffer().width();
				aoDesc.height = ctx.gfx.backBuffer().height();
				aoDesc.format = gfx::TextureFormat::RGBA8Unorm;
				aoDesc.usage = gfx::TextureUsage::RenderTarget | gfx::TextureUsage::Sampled;
				d.aoOut = b.createTexture("AORaw", aoDesc);
				d.aoOut = b.writeColour(d.aoOut, gfx::RGLoadOp::DontCare);

				d.resources = &resources;
			},
			[](const GTAOPassData& d, gfx::RenderGraphContext& rgCtx)
			{
				rgCtx.cmd().bindPipeline(d.resources->gtaoPipeline());
				rgCtx.cmd().bindTexture(rgCtx.texture(d.depthIn), d.resources->sampler(), 0);
				rgCtx.cmd().bindTexture(rgCtx.texture(d.normalIn), d.resources->sampler(), 1);
				rgCtx.cmd().bindUniformBuffer(rgCtx.buffer(d.paramsUBO), 2);
				rgCtx.cmd().draw(3, 1);
			});

		return data.aoOut;
	}

	gfx::RGTextureHandle addBilateralBlurPass(gfx::RenderGraph &graph, RenderResources &resources, AppContext &ctx, const PrepassOutputs &prepass, gfx::RGTextureHandle rawAO)
	{
		const auto& data = graph.addPass<BlurPassData>("AOBilateralBlur",
		[&](gfx::RenderGraphBuilder& b, BlurPassData& d)
		{
			d.aoIn = b.readTexture(rawAO);
			d.depthIn = b.readTexture(prepass.depthTarget);
			d.normalIn = b.readTexture(prepass.normalTarget);

			const u32 w = ctx.gfx.backBuffer().width();
			const u32 h = ctx.gfx.backBuffer().height();

			gfx::TextureDesc blurDesc{};
			blurDesc.width = w; blurDesc.height = h;
			blurDesc.format = gfx::TextureFormat::RGBA8Unorm;
			blurDesc.usage = gfx::TextureUsage::RenderTarget | gfx::TextureUsage::Sampled;
			d.blurredOut = b.createTexture("AOBlurred", blurDesc);
			d.blurredOut = b.writeColour(d.blurredOut, gfx::RGLoadOp::DontCare);

			gfx::BlurParamsUBO cpuParams{};
			cpuParams.texelSizeAndSigmas = { 1.f / static_cast<float>(w), 1.f / static_cast<float>(h),
				gfx::ao::cvarBlurDepthSigma, gfx::ao::cvarBlurNormalSigma };
			resources.blurParamsUBO(ctx.gfx.currentFrameIndex()).update(&cpuParams, sizeof(cpuParams), 0);

			d.paramsUBO = b.readBuffer(b.importBuffer("BlurParamsUBO", &resources.blurParamsUBO(ctx.gfx.currentFrameIndex())));
			d.resources = &resources;
		},
		[](const BlurPassData& d, gfx::RenderGraphContext& rgCtx)
		{
			rgCtx.cmd().bindPipeline(d.resources->blurPipeline());
			rgCtx.cmd().bindTexture(rgCtx.texture(d.aoIn), d.resources->sampler(), 0);
			rgCtx.cmd().bindTexture(rgCtx.texture(d.depthIn), d.resources->sampler(), 1);
			rgCtx.cmd().bindTexture(rgCtx.texture(d.normalIn), d.resources->sampler(), 2);
			rgCtx.cmd().bindUniformBuffer(rgCtx.buffer(d.paramsUBO), 3);
			rgCtx.cmd().draw(3, 1);
		});

		return data.blurredOut;
	}
}
