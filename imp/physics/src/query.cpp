#include <physics/query.h>
#include <physics/collider/collider.h>
#include <ecs/world.h>

namespace imp::physics
{
	std::optional<RaycastHit> Raycaster::raycast(const Ray& ray, float maxDistance) const
	{
		std::optional<RaycastHit> best;
		float bestT = maxDistance;

		const ecs::ColliderStorage& colliders = m_world.colliders;
		for (size_t i = 0; i < colliders.size(); ++i)
		{
			const ecs::EntityId entity = colliders.ownerAt(i);
			if (!m_world.transforms.contains(entity))
				continue;

			const math::Mat4f world = m_world.transforms.worldMatrix(entity);
			const physics::Collider& collider = colliders.colliderAt(i);

			float t;
			if (!collider.rayIntersect(ray.origin, ray.direction, world, bestT, t))
				continue;

			bestT = t;
			best = RaycastHit{ entity, t, ray.origin + ray.direction * t };
		}

		return best;
	}
}
