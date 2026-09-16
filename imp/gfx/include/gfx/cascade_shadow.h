#pragma once

#include <core/math/math.h>
#include <core/types/int_types.h>
#include <camera/camera.h>
#include <array>

namespace imp::gfx
{
	constexpr u32 kCascadeCount = 4;

	struct CascadeData
	{
		math::Mat4f viewProj;
		math::Mat4f lightView;
		math::Vec3f boxMin;
		math::Vec3f boxMax;
		float splitDepth = 0.f;
		float worldUnitsPerTexel = 0.f;
		u32 shadowMapResolution = 0;
	};

	struct CascadeConfig
	{
		std::array<u32, kCascadeCount> resolution = { 4096, 2048, 2048, 1024 };
		float splitLambda = 0.75f;
		float zPadding = 25.f;
		float blendFraction = 0.15f;
		std::array<float, kCascadeCount> radiusMultiplier = { 1.f, 1.f, 1.f, 1.f };
	};

	std::array<CascadeData, kCascadeCount> computeCascades(
		const fwk::Camera& camera,
		float aspect,
		const math::Vec3f& sunDirection,
		const CascadeConfig& config);
}
