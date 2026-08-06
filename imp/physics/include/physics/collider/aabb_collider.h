#pragma once
#include <physics/collider/collider.h>

namespace imp::physics
{
	class AABBCollider final : public Collider
	{
	public:
		AABBCollider(const math::Vec3f& localMin, const math::Vec3f& localMax) 
			: m_local{ localMin, localMax } {}

		ColliderType type() const override { return ColliderType::AABB; }

		AABB worldBounds(const math::Mat4f& transform) const override;

		bool rayIntersect(const math::Vec3f& rayOrigin, const math::Vec3f& rayDir,
			const math::Mat4f& transform, float maxDistance, float& outT) const override;
		
		const AABB& localBounds() const { return m_local; }
		void setLocalBounds(const math::Vec3f& localMin, const math::Vec3f& localMax) { m_local = AABB{ localMin, localMax }; }

	private:
		AABB m_local;
	};
}
