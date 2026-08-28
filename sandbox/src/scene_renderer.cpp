#include <sandbox/scene_renderer.h>
#include <sandbox/render_resources.h>
#include <sandbox/sandbox_scene.h>
#include <gfx/render_extraction.h>
#include <gfx/lighting.h>
#include <cstdio>

namespace imp::app
{
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
