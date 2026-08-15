#include <gfx/cascade_shadow.h>

namespace imp::gfx
{
	namespace
	{
		std::array<float, kCascadeCount + 1> computeSplits(float near, float far, float lambda)
		{
			std::array<float, kCascadeCount + 1> splits{};
			splits[0] = near;
			for (u32 i = 1; i <= kCascadeCount; ++i)
			{
				const float p = static_cast<float>( i ) / static_cast<float>( kCascadeCount );
				const float logSplit = near * std::pow(far / near, p);
				const float uniformSplit = near + ( far - near ) * p;
				splits[i] = lambda * logSplit + ( 1.f - lambda ) * uniformSplit;
			}
			return splits;
		}
	}

	std::array<CascadeData,kCascadeCount> computeCascades(
		const fwk::Camera& camera, 
		float aspect, 
		const math::Vec3f& sunDirection, 
		const CascadeConfig& config)
	{
		std::array<CascadeData, kCascadeCount> out{};

		const auto splits = computeSplits(camera.nearPlane, camera.farPlane, config.splitLambda);
		const math::Vec3f lightDir = math::normalise(sunDirection);

		math::Vec3f upHint = math::Vec3f::up();
		if (std::abs(math::dot(lightDir, upHint)) > 0.99f)
			upHint = math::Vec3f::forward();

		for (u32 i = 0; i < kCascadeCount; ++i)
		{
			const float sliceNear = splits[i];
			const float sliceFar = splits[i + 1];
			const auto corners = camera.frustumCornersWorldSpace(aspect, sliceNear, sliceFar);

			math::Vec3f centre = math::Vec3f::zero();
			for (const auto& c : corners) 
				centre += c;

			centre /= static_cast<float>(corners.size());

			float radius = 0.f;
			for (const auto& c : corners)
				radius = std::max(radius, math::length(c - centre));
			radius = std::ceil(radius * 16.f) / 16.f;
			radius *= config.radiusMultiplier[i];

			const float worldUnitsPerTexel = ( radius * 2.f ) / static_cast<float>( config.shadowMapResolution );
			const math::Mat4f lookAtNoSnap = math::makeLookAtLH(centre - lightDir * radius, centre, upHint);

			math::Vec4f originLS = lookAtNoSnap * math::Vec4f{ centre, 1.f };
			originLS.x = std::floor(originLS.x / worldUnitsPerTexel) * worldUnitsPerTexel;
			originLS.y = std::floor(originLS.y / worldUnitsPerTexel) * worldUnitsPerTexel;

			const math::Mat4f invLookAt = math::inverse(lookAtNoSnap);
			const math::Vec4f snappedCentreWS = invLookAt * math::Vec4f{ originLS.xyz(), 1.f };
			const math::Vec3f snappedCentre{ snappedCentreWS.xyz() };

			const math::Vec3f eye = snappedCentre - lightDir * ( radius + config.zPadding );
			const math::Mat4f lightView = math::makeLookAtLH(eye, snappedCentre, upHint);

			const math::Mat4f lightProj = math::makeOrthographicOffcentreLH(
				-radius, radius, -radius, radius,
				0.f, 2.f * radius + 2.f * config.zPadding);

			out[i].viewProj = lightProj * lightView;
			out[i].lightView = lightView;
			out[i].boxMin = { -radius, -radius, 0.f };
			out[i].boxMax = math::Vec3f{ radius, radius, 2.f * radius + 2.f * config.zPadding };
			out[i].splitDepth = sliceFar;
			out[i].worldUnitsPerTexel = worldUnitsPerTexel;
		}

		return out;
	}
}
