#pragma once
#include <physics/collider/aabb.h>
#include <core/math/math.h>

namespace imp::physics
{
	enum class ColliderType { AABB, Sphere, Capsule, ConvexHull };
	class Collider
	{
	public:
		virtual ~Collider() = default;
		virtual ColliderType type() const = 0;

		virtual AABB worldBounds(const math::Mat4f& transform) const = 0;

		virtual bool rayIntersect(const math::Vec3f& rayOrigin, const math::Vec3f& rayDir,
			const math::Mat4f& transform, float maxDistance, float& outT) const = 0;
	};
}
