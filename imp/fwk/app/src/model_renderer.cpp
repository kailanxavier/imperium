#include <app/model_renderer.h>

#include <gfx/model.h>
#include <gfx/model_registry.h>
#include <gfx/commands.h>
#include <gfx/lighting.h>

#include <core/log/log.h>

namespace imp::app
{
	namespace
	{
		void drawNodeInstanced(const ModelRenderContext& ctx, const gfx::Model& model, u32 nodeIdx,
			const math::Mat4f parentNodeWorld, u32 firstInstance, u32 instanceCount)
		{
			const gfx::ModelNode& node = model.nodes[nodeIdx];
			const math::Mat4f nodeWorld = parentNodeWorld * node.localTransform;
			
			if (node.meshIndex >= 0)
			{
				for (const gfx::MeshPrimitive& prim : model.meshes[node.meshIndex].primitives)
				{
					gfx::MeshPushConstants pc;
					pc.viewProj = ctx.viewProj;
					pc.nodeWorld = nodeWorld;
					ctx.cmd->pushConstants(&pc, sizeof(pc), 0);

					ctx.cmd->bindUniformBuffer(*ctx.lightBuffer, 0);
					if (prim.materialIndex >= 0)
					{
						const i32 texIdx = model.materials[prim.materialIndex].baseColourTextureIndex;
						if (texIdx >= 0)
							ctx.cmd->bindTexture(*model.textures[texIdx].texture, *ctx.sampler, 1);
					}

					ctx.cmd->bindVertexBuffer(*prim.vertexBuffer, 0);
					ctx.cmd->bindIndexBuffer(*prim.indexBuffer);
					ctx.cmd->drawIndexed(prim.indexCount, instanceCount, firstInstance);
				}
			}

			for (u32 child : node.children)
				drawNodeInstanced(ctx, model, child, nodeWorld, firstInstance, instanceCount);
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
				drawNodeInstanced(ctx, *model, root, math::Mat4f::identity(), batch.firstInstance, batch.instanceCount);
		}
	}
}
