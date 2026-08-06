#pragma once
#include <ecs/entity.h>
#include <physics/collider/collider.h>
#include <physics/collider/aabb_collider.h>
#include <core/types/int_types.h>
#include <memory>
#include <vector>

namespace imp::ecs
{
	class ColliderStorage
	{
	public:
		static constexpr u32 kInvalidDense = 0xFFFFFFFFU;

		void create(EntityId entity, std::unique_ptr<physics::Collider> collider);

		// Convenience until other collider types exist as first class helpers too
		void createAABB(EntityId entity, const math::Vec3f& localMin, const math::Vec3f& localMax);

		void destroy(EntityId entity);
		bool contains(EntityId entity) const;

		physics::Collider& get(EntityId entity);
		const physics::Collider& get(EntityId entity) const;

		size_t size() const { return m_owner.size(); }
		EntityId ownerAt(size_t denseIndex) const { return m_owner[denseIndex]; }
		physics::Collider& colliderAt(size_t denseIndex) { return *m_collider[denseIndex]; }
		const physics::Collider& colliderAt(size_t denseIndex) const { return *m_collider[denseIndex]; }

	private:
		u32 denseIndexOf(EntityId entity) const;
		void swapRemoveDense(u32 idx);

		std::vector<EntityId> m_owner;
		std::vector<std::unique_ptr<physics::Collider>> m_collider;
		std::vector<u32> m_sparse;
	};
}
