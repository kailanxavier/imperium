#pragma once

#include <core/math/math.h>
#include <core/types/int_types.h>
#include <array>

namespace imp::gfx
{
	constexpr u32 kPointShadowFaceCount = 6;
	struct PointShadowFaceBasis
	{
		math::Vec3f forward;
		math::Vec3f up;
	};

	inline const std::array<PointShadowFaceBasis, kPointShadowFaceCount>& pointShadowFaceBases()
	{
		static const std::array<PointShadowFaceBasis, kPointShadowFaceCount> bases = { {
			{ math::Vec3f{  1.f,  0.f,  0.f }, math::Vec3f{ 0.f, 1.f,  0.f } }, // +X
			{ math::Vec3f{ -1.f,  0.f,  0.f }, math::Vec3f{ 0.f, 1.f,  0.f } }, // -X
			{ math::Vec3f{  0.f,  1.f,  0.f }, math::Vec3f{ 0.f, 0.f, -1.f } }, // +Y
			{ math::Vec3f{  0.f, -1.f,  0.f }, math::Vec3f{ 0.f, 0.f,  1.f } }, // -Y
			{ math::Vec3f{  0.f,  0.f,  1.f }, math::Vec3f{ 0.f, 1.f,  0.f } }, // +Z
			{ math::Vec3f{  0.f,  0.f, -1.f }, math::Vec3f{ 0.f, 1.f,  0.f } }, // -Z
		} };
		return bases;
	}

	inline std::array<math::Mat4f, kPointShadowFaceCount> computePointShadowViewProj(
		const math::Vec3f& lightPosition, float nearPlane, float farPlane)
	{
		const math::Mat4f proj = math::makePerspectiveLH(math::kPif * 0.5f, 1.f, nearPlane, farPlane);

		std::array<math::Mat4f, kPointShadowFaceCount> out{};
		const auto& bases = pointShadowFaceBases();
		for (u32 face = 0; face < kPointShadowFaceCount; ++face)
		{
			const math::Mat4f view = 
				math::makeLookAtLH(lightPosition, lightPosition + bases[face].forward, bases[face].up);
			out[face] = proj * view;
		}
		return out;
	}

}
