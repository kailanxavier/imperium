#pragma once
#include <gfx/render_extraction.h>
#include <core/math/math.h>
#include <array>

namespace imp::gfx
{
	class ICommandList;
	class IBuffer;
	class ITexture;
	class ISampler;
	class ModelRegistry;

	struct Plane
	{
		math::Vec3f normal;
		float distance = 0.f;
	};

	struct CullVolume
	{
		math::Mat4f lightView;
		math::Vec3f boxMin;
		math::Vec3f boxMax;

		std::array<Plane, 6> frustumPlanes{};
		bool useFrustum = false;
	};

	std::array<Plane, 6> extractFrustumPlanes(const math::Mat4f& viewProj);
	bool sphereIntersectsFrustum(const math::Vec3f& centre, float radius, const std::array<Plane, 6>& planes);

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

		gfx::ITexture* aoTexture = nullptr;
		gfx::IBuffer* screenParamsBuffer = nullptr;
		bool alphaTestOnly = false;

		math::Mat4f viewProj;

		const CullVolume* cullVolume = nullptr;
	};

	void drawModelBatches(const ModelRenderContext& ctx, const RenderExtraction& extraction);
	void drawBlendInstances(const ModelRenderContext& ctx, const RenderExtraction& extraction);
}
