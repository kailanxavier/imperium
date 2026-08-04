#pragma once
#include <core/math/math.h>

namespace imp::physics
{
	struct AABB
	{
		math::Vec3f min{};
		math::Vec3f max{};

		static AABB fromCentreExtents(const math::Vec3f& centre, const math::Vec3f& extents)
		{
			return AABB{ centre - extents, centre + extents };
		}

		math::Vec3f centre() const { return ( min + max ) * 0.5f; }
		math::Vec3f extents() const { return ( max - min ) * 0.5f; }

		AABB transformed(const math::Mat4f& m) const;

		bool rayIntersect(const math::Vec3f& rayOrigin, const math::Vec3f& rayDir, float maxDistance, float& outT) const;
	};
}
