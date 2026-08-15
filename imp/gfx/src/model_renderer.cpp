#include <gfx/model_renderer.h>

#include <gfx/model.h>
#include <gfx/model_registry.h>
#include <gfx/commands.h>
#include <gfx/lighting.h>

#include <core/log/log.h>

namespace imp::gfx
{
	namespace
	{
		void drawNodeInstanced(const ModelRenderContext& ctx, const gfx::Model& model, u32 nodeIdx,
			const math::Mat4f parentNodeWorld, u32 firstInstance, u32 instanceCount, gfx::AlphaModePass passKind)
		{
			const gfx::ModelNode& node = model.nodes[nodeIdx];
			const math::Mat4f nodeWorld = parentNodeWorld * node.localTransform;
			
			if (node.meshIndex >= 0)
			{
				for (const gfx::MeshPrimitive& prim : model.meshes[node.meshIndex].primitives)
				{
					const gfx::Material* mat = (prim.materialIndex >= 0) ? &model.materials[prim.materialIndex] : nullptr;
					const gfx::AlphaMode resolvedAlphaMode = mat ? mat->alphaMode : gfx::AlphaMode::Opaque;
					//const bool primitiveIsBlend = (resolvedAlphaMode == gfx::AlphaMode::Blend);
					
					if (passKind == gfx::AlphaModePass::OpaqueAndMask)
					{
						if (resolvedAlphaMode == gfx::AlphaMode::Blend)
							continue;
					}
					else
					{
						if (resolvedAlphaMode != gfx::AlphaMode::Blend)
							continue;
					}

					gfx::MeshPushConstants pc;
					pc.viewProj = ctx.viewProj;
					pc.nodeWorld = nodeWorld;
					ctx.cmd->pushConstants(&pc, sizeof(pc), 0);

					if (ctx.lightBuffer)
					{
						ctx.cmd->bindUniformBuffer(*ctx.lightBuffer, 0);

						auto resolveTexture = [&](i32 materialTexIndex, i32 fallbackTexIndex) -> gfx::ITexture*
							{
								const i32 texIdx = (materialTexIndex >= 0) ? materialTexIndex : fallbackTexIndex;
								return (texIdx >= 0 && model.textures[texIdx].texture) ? model.textures[texIdx].texture.get() : nullptr;
							};

						if (gfx::ITexture* albedo = resolveTexture(mat ? mat->baseColourTextureIndex : -1, model.fallbackAlbedoTextureIndex))
							ctx.cmd->bindTexture(*albedo, *ctx.sampler, 1);
						if (gfx::ITexture* mr = resolveTexture(mat ? mat->metallicRoughnessTextureIndex : -1, model.fallbackMetallicRoughnessTextureIndex))
							ctx.cmd->bindTexture(*mr, *ctx.sampler, 2);
						if (gfx::ITexture* normal = resolveTexture(mat ? mat->normalTextureIndex : -1, model.fallbackNormalTextureIndex))
							ctx.cmd->bindTexture(*normal, *ctx.sampler, 3);
						if (gfx::ITexture* occlusion = resolveTexture(mat ? mat->occlusionTextureIndex : -1, model.fallbackOcclusionTextureIndex))
							ctx.cmd->bindTexture(*occlusion, *ctx.sampler, 4);

						gfx::IBuffer* factors = (mat && mat->factorsBuffer) ? mat->factorsBuffer.get() : model.defaultMaterialFactorsBuffer.get();
						if (factors)
							ctx.cmd->bindUniformBuffer(*factors, 6);

						if (ctx.shadowArrayTexture)
							ctx.cmd->bindTexture(*ctx.shadowArrayTexture, *ctx.shadowSampler, 5);
						if (ctx.cascadeBuffer)
							ctx.cmd->bindUniformBuffer(*ctx.cascadeBuffer, 7);
					}

					ctx.cmd->bindVertexBuffer(*prim.vertexBuffer, 0);
					ctx.cmd->bindIndexBuffer(*prim.indexBuffer);
					ctx.cmd->drawIndexed(prim.indexCount, instanceCount, firstInstance);
				}
			}

			for (u32 child : node.children)
				drawNodeInstanced(ctx, model, child, nodeWorld, firstInstance, instanceCount, passKind);
		}
	}

	void drawModelBatches(const ModelRenderContext& ctx, const RenderExtraction& extraction)
	{
		if (!ctx.cmd || !ctx.modelRegistry || !ctx.instanceBuffer)
			return;

		ctx.cmd->bindVertexBuffer(*ctx.instanceBuffer, 1);

		for (const ModelBatch& batch : extraction.batches)
		{
			const gfx::Model* model = ctx.modelRegistry->tryGet(batch.model);
			if (!model)
			{
				LOG_ERROR("Application", "Model registry did not return a model");
				continue;
			}

			for (u32 root : model->rootNodes)
				drawNodeInstanced(ctx, *model, root, math::Mat4f::identity(), batch.firstInstance, batch.instanceCount, gfx::AlphaModePass::OpaqueAndMask);
		}
	}

	void drawBlendInstances(const ModelRenderContext& ctx, const RenderExtraction& extraction)
	{
		if (!ctx.cmd || !ctx.modelRegistry || !ctx.instanceBuffer)
			return;
		ctx.cmd->bindVertexBuffer(*ctx.instanceBuffer, 1);
		for (const BlendInstance& blend : extraction.blendInstances)
		{
			const gfx::Model* model = ctx.modelRegistry->tryGet(blend.model);
			if (!model)
				continue;
			for (u32 root : model->rootNodes)
				drawNodeInstanced(ctx, *model, root, math::Mat4f::identity(),
					blend.instanceOffset, 1, gfx::AlphaModePass::Blend);
		}
	}
}
