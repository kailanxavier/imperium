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
		float splitDepth = 0.f;
	};

	struct CascadeConfig
	{
		u32 shadowMapResolution = 4096;
		float splitLambda = 0.99f;
		float zPadding = 25.f;
	};

	std::array<CascadeData, kCascadeCount> computeCascades(
		const fwk::Camera& camera,
		float aspect,
		const math::Vec3f& sunDirection,
		const CascadeConfig& config);
}
