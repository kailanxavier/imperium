#include <physics/collider/aabb_collider.h>

namespace imp::physics
{
	AABB AABBCollider::worldBounds(const math::Mat4f& transform) const
	{
		return m_local.transformed(transform);
	}

	bool AABBCollider::rayIntersect(const math::Vec3f& rayOrigin, const math::Vec3f& rayDir, 
		const math::Mat4f& transform, float maxDistance, float& outT) const
	{
		return m_local.transformed(transform).rayIntersect(rayOrigin, rayDir, maxDistance, outT);
	}


}
