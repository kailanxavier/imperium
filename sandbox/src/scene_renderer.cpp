#include <sandbox/scene_renderer.h>
#include <sandbox/render_resources.h>
#include <sandbox/sandbox_scene.h>
#include <gfx/render_extraction.h>
#include <gfx/lighting.h>
#include <cstdio>

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

	gfx::RGTextureHandle addHdrPass(gfx::RenderGraph &graph, RenderResources &resources, SandboxScene &scene, AppContext &ctx, const SceneRenderParams &params, const ShadowCascadePasses &shadowPasses)
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

				d.hdrColour = b.writeColour(d.hdrColour, gfx::RGLoadOp::Clear, { 0.023153f, 0.000911f, 0.004391f, 1.f });
				d.hdrDepth = b.writeDepth(d.hdrDepth, gfx::RGLoadOp::Clear);
				d.hdrResolve = b.writeResolve(d.hdrResolve, d.hdrColour);

				d.lightUBO = b.readBuffer(b.importBuffer("LightUBO", &resources.lightUBO(params.currentFrame)));
				d.cascadeUBO = b.readBuffer(b.importBuffer("CascadeUBO", &resources.cascadeUBO(params.currentFrame)));

				d.shadowArray = b.readTexture(b.importTexture("ShadowArray", resources.shadowArrayTexture()));
				for (const auto& cascadeTarget : shadowPasses.cascadeDepthTargets)
					b.readTexture(cascadeTarget);

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


	void renderShadowCascades(gfx::ICommandList& cmd, RenderResources& resources, SandboxScene& scene, const SceneRenderParams& params)
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

		for (u32 i = 0; i < gfx::kCascadeCount; ++i)
		{
			gfx::RenderPassDesc shadowPassDesc{};
			shadowPassDesc.colourTarget = nullptr;
			shadowPassDesc.depthTarget = &resources.shadowCascadeTarget(i);
			shadowPassDesc.clearDepthValue = 1.f;

			char label[32];
			snprintf(label, sizeof(label), "Shadow Cascade %u", i);
			shadowPassDesc.debugName = label;

			cmd.beginRenderPass(shadowPassDesc);

			gfx::CullVolume cullVolume{ cascades[i].lightView, cascades[i].boxMin, cascades[i].boxMax };

			gfx::ModelRenderContext shadowRenderCtx{};
			shadowRenderCtx.cmd = &cmd;
			shadowRenderCtx.modelRegistry = &scene.modelRegistry();
			shadowRenderCtx.sampler = &resources.sampler();
			shadowRenderCtx.lightBuffer = nullptr;
			shadowRenderCtx.instanceBuffer = &resources.instanceBuffer(params.currentFrame);
			shadowRenderCtx.viewProj = cascades[i].viewProj;
			shadowRenderCtx.cullVolume = &cullVolume;

			cmd.bindPipeline(resources.shadowPipeline());
			drawModelBatches(shadowRenderCtx, scene.extraction());
			cmd.endRenderPass();
		}
	}

	void renderHdrPass(gfx::ICommandList& cmd, RenderResources& resources, SandboxScene& scene, AppContext& ctx, const SceneRenderParams& params)
	{
		gfx::RenderPassDesc hdrPassDesc{};
		hdrPassDesc.colourTarget = &resources.hdrTarget();
		hdrPassDesc.resolveTarget = &resources.hdrResolveTarget();
		hdrPassDesc.depthTarget = &resources.hdrDepthTarget();
		hdrPassDesc.clearColourValue = { 0.023153f, 0.000911f, 0.004391f, 1.f };
		hdrPassDesc.clearDepthValue = 1.f;
		hdrPassDesc.debugName = "HDR";
		cmd.beginRenderPass(hdrPassDesc);

		gfx::CullVolume mainCullVolume{};

		gfx::ModelRenderContext renderCtx{};
		renderCtx.cmd = &cmd;
		renderCtx.modelRegistry = &scene.modelRegistry();
		renderCtx.sampler = &resources.sampler();
		renderCtx.lightBuffer = &resources.lightUBO(params.currentFrame);
		renderCtx.instanceBuffer = &resources.instanceBuffer(params.currentFrame);
		renderCtx.viewProj = params.camera->projection(params.aspect) * params.camera->view();
		renderCtx.shadowArrayTexture = resources.shadowArrayTexture();
		renderCtx.cascadeBuffer = &resources.cascadeUBO(params.currentFrame);
		renderCtx.shadowSampler = &resources.shadowSampler();
		if (params.enableFrustumCulling)
		{
			mainCullVolume.useFrustum = true;
			mainCullVolume.frustumPlanes = gfx::extractFrustumPlanes(renderCtx.viewProj);
			renderCtx.cullVolume = &mainCullVolume;
		}

		cmd.bindPipeline(resources.meshPipeline());
		drawModelBatches(renderCtx, scene.extraction());

		gfx::SkyPushConstants skyPC{};
		skyPC.invViewProj = math::inverse(renderCtx.viewProj);
		skyPC.cameraPositionWS = math::Vec4f{ params.camera->position(), 1.f };
		skyPC.sunDirAndIntensity = math::Vec4f{ -scene.sunDirection(), 10.f };

		cmd.bindPipeline(resources.skyPipeline());
		cmd.pushConstants(&skyPC, sizeof(skyPC));
		cmd.draw(3, 1);

		if (!scene.extraction().blendInstances.empty())
		{
			cmd.bindPipeline(resources.blendPipeline());
			drawBlendInstances(renderCtx, scene.extraction());
		}

		ctx.layers.renderAll(cmd);

		cmd.endRenderPass();
	}

	void renderTonemapPass(gfx::ICommandList& cmd, RenderResources& resources, AppContext& ctx, gfx::IRenderTarget& target)
	{
		gfx::RenderPassDesc tonemapPassDesc{};
		tonemapPassDesc.colourTarget = &target;
		tonemapPassDesc.depthTarget = nullptr;
		tonemapPassDesc.clearColour = false;
		tonemapPassDesc.debugName = "Tonemap";
		cmd.beginRenderPass(tonemapPassDesc);

		cmd.bindPipeline(resources.tonemapPipeline());
		cmd.bindTexture(*resources.hdrResolveTarget().asTexture(), resources.sampler(), 1);
		cmd.draw(3, 1);

		cmd.endRenderPass();
	}
}
