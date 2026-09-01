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
		bool sphereIntersectsBox(const math::Vec3f& centre, float radius, const math::Vec3f& boxMin, const math::Vec3f& boxMax)
		{
			return centre.x >= boxMin.x - radius && centre.x <= boxMax.x + radius
				&& centre.y >= boxMin.y - radius && centre.y <= boxMax.y + radius
				&& centre.z >= boxMin.z - radius && centre.z <= boxMax.z + radius;
		}

		void drawNodeInstanced(const ModelRenderContext& ctx, const gfx::Model& model, u32 nodeIdx,
			const math::Mat4f parentNodeWorld, u32 firstInstance, u32 instanceCount,
			gfx::AlphaModePass passKind, const math::Mat4f* singleInstanceWorld = nullptr)
		{
			const gfx::ModelNode& node = model.nodes[nodeIdx];
			const math::Mat4f nodeWorld = parentNodeWorld * node.localTransform;

			if (node.meshIndex >= 0)
			{
				math::Mat4f entityWorld;
				bool haveEntityWorld = false;
				float entityScale = 1.f;
				if (ctx.cullVolume && singleInstanceWorld)
				{
					entityWorld = ( *singleInstanceWorld ) * nodeWorld;
					haveEntityWorld = true;
					entityScale = std::max({
						math::length(math::Vec3f{ entityWorld(0,0), entityWorld(1,0), entityWorld(2,0) }),
						math::length(math::Vec3f{ entityWorld(0,1), entityWorld(1,1), entityWorld(2,1) }),
						math::length(math::Vec3f{ entityWorld(0,2), entityWorld(1,2), entityWorld(2,2) }) });
				}

				for (const gfx::MeshPrimitive& prim : model.meshes[node.meshIndex].primitives)
				{
					const gfx::Material* mat = ( prim.materialIndex >= 0 ) ? &model.materials[prim.materialIndex] : nullptr;
					const gfx::AlphaMode resolvedAlphaMode = mat ? mat->alphaMode : gfx::AlphaMode::Opaque;

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

					if (haveEntityWorld)
					{
						const math::Vec3f centreWS = math::transformPoint(entityWorld, prim.boundsCentreLocal);
						const float worldRadius = prim.boundsRadiusLocal * entityScale;

						bool visible = false;
						if (ctx.cullVolume->useFrustum)
						{
							visible = sphereIntersectsFrustum(centreWS, worldRadius, ctx.cullVolume->frustumPlanes);
						}
						else
						{
							const math::Vec3f centreLS = math::transformPoint(ctx.cullVolume->lightView, centreWS);
							visible = sphereIntersectsBox(centreLS, worldRadius, ctx.cullVolume->boxMin, ctx.cullVolume->boxMax);
						}

						if (!visible)
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
								const i32 texIdx = ( materialTexIndex >= 0 ) ? materialTexIndex : fallbackTexIndex;
								return ( texIdx >= 0 && model.textures[texIdx].texture ) ? model.textures[texIdx].texture.get() : nullptr;
							};

						if (gfx::ITexture* albedo = resolveTexture(mat ? mat->baseColourTextureIndex : -1, model.fallbackAlbedoTextureIndex))
							ctx.cmd->bindTexture(*albedo, *ctx.sampler, 1);
						if (gfx::ITexture* mr = resolveTexture(mat ? mat->metallicRoughnessTextureIndex : -1, model.fallbackMetallicRoughnessTextureIndex))
							ctx.cmd->bindTexture(*mr, *ctx.sampler, 2);
						if (gfx::ITexture* normal = resolveTexture(mat ? mat->normalTextureIndex : -1, model.fallbackNormalTextureIndex))
							ctx.cmd->bindTexture(*normal, *ctx.sampler, 3);
						if (gfx::ITexture* occlusion = resolveTexture(mat ? mat->occlusionTextureIndex : -1, model.fallbackOcclusionTextureIndex))
							ctx.cmd->bindTexture(*occlusion, *ctx.sampler, 4);

						gfx::IBuffer* factors = ( mat && mat->factorsBuffer ) ? mat->factorsBuffer.get() : model.defaultMaterialFactorsBuffer.get();
						if (factors)
							ctx.cmd->bindUniformBuffer(*factors, 6);

						if (ctx.shadowArrayTexture)
							ctx.cmd->bindTexture(*ctx.shadowArrayTexture, *ctx.shadowSampler, 5);
						if (ctx.cascadeBuffer)
							ctx.cmd->bindUniformBuffer(*ctx.cascadeBuffer, 7);

						if (ctx.aoTexture)
							ctx.cmd->bindTexture(*ctx.aoTexture, *ctx.sampler, 8);
						if (ctx.screenParamsBuffer)
							ctx.cmd->bindUniformBuffer(*ctx.screenParamsBuffer, 9);
					}

					ctx.cmd->bindVertexBuffer(*prim.vertexBuffer, 0);
					ctx.cmd->bindIndexBuffer(*prim.indexBuffer);
					ctx.cmd->drawIndexed(prim.indexCount, instanceCount, firstInstance);
				}
			}

			for (u32 child : node.children)
				drawNodeInstanced(ctx, model, child, nodeWorld, firstInstance, instanceCount, passKind, singleInstanceWorld);
		}
	}

	std::array<Plane, 6> extractFrustumPlanes(const math::Mat4f& m)
	{
		auto row = [&](int r) { return math::Vec4f{ m(r, 0), m(r, 1), m(r, 2), m(r, 3) }; };
		const math::Vec4f r0 = row(0);
		const math::Vec4f r1 = row(1);
		const math::Vec4f r2 = row(2);
		const math::Vec4f r3 = row(3);
		
		std::array<math::Vec4f, 6> raw = {
			r3 + r0,
			r3 - r0,
			r3 + r1,
			r3 - r1,
			r2,
			r3 - r2,
		};

		std::array<Plane, 6> planes;
		for (int i = 0; i < 6; ++i)
		{
			math::Vec3f n{ raw[i].x, raw[i].y, raw[i].z };
			const float len = math::length(n);
			planes[i].normal = ( len > 1e-8f ) ? ( n / len ) : n;
			planes[i].distance = ( len > 1e-8f ) ? ( raw[i].w / len ) : raw[i].w;
		}
		return planes;
	}

	bool sphereIntersectsFrustum(const math::Vec3f& centre, float radius, const std::array<Plane, 6>& planes)
	{
		for (const Plane& p : planes)
			if (math::dot(p.normal, centre) + p.distance < -radius)
				return false;
		return true;
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

			if (ctx.cullVolume && batch.instanceCount > 1)
			{
				for (u32 i = 0; i < batch.instanceCount; ++i)
				{
					const u32 instanceIdx = batch.firstInstance + i;
					if (instanceIdx >= extraction.instanceData.size())
						break;

					const math::Mat4f* instanceWorld = &extraction.instanceData[instanceIdx];
					for (u32 root : model->rootNodes)
						drawNodeInstanced(ctx, *model, root, math::Mat4f::identity(),
							instanceIdx, 1, gfx::AlphaModePass::OpaqueAndMask, instanceWorld);
				}
				continue;
			}

			const math::Mat4f* singleInstanceWorld = nullptr;
			if (batch.instanceCount == 1 && batch.firstInstance < extraction.instanceData.size())
				singleInstanceWorld = &extraction.instanceData[batch.firstInstance];

			for (u32 root : model->rootNodes)
				drawNodeInstanced(ctx, *model, root, math::Mat4f::identity(),
					batch.firstInstance, batch.instanceCount, gfx::AlphaModePass::OpaqueAndMask, singleInstanceWorld);
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

			const math::Mat4f* instanceWorld = ( blend.instanceOffset < extraction.instanceData.size() )
				? &extraction.instanceData[blend.instanceOffset] : nullptr;

			for (u32 root : model->rootNodes)
				drawNodeInstanced(ctx, *model, root, math::Mat4f::identity(),
					blend.instanceOffset, 1, gfx::AlphaModePass::Blend, instanceWorld);
		}
	}
}
