#pragma once
#include <gfx/render_extraction.h>
#include <core/math/math.h>

namespace imp::gfx
{
	class ICommandList;
	class IBuffer;
	class ITexture;
	class ISampler;
	class ModelRegistry;

	struct CullVolume
	{
		math::Mat4f lightView;
		math::Vec3f boxMin;
		math::Vec3f boxMax;
	};

	struct ModelRenderContext
	{
		gfx::ICommandList* cmd = nullptr;
		gfx::ModelRegistry* modelRegistry = nullptr;
		gfx::ISampler* sampler = nullptr;

		gfx::IBuffer* lightBuffer = nullptr;
		gfx::IBuffer* instanceBuffer = nullptr;

		gfx::ITexture* shadowArrayTexture = nullptr;
		gfx::IBuffer* cascadeBuffer = nullptr;
		gfx::ISampler* shadowSampler = nullptr;

		math::Mat4f viewProj;

		const CullVolume* cullVolume = nullptr;
	};

	void drawModelBatches(const ModelRenderContext& ctx, const RenderExtraction& extraction);
	void drawBlendInstances(const ModelRenderContext& ctx, const RenderExtraction& extraction);
}
