#include <ecs/collider_storage.h>
#include <cassert>
#include <utility>

namespace imp::ecs
{
	void ColliderStorage::create(EntityId entity, std::unique_ptr<physics::Collider> collider)
	{
		assert(!contains(entity) && "ColliderStorage::create: entity already present");

		const u32 newDense = static_cast<u32>( m_owner.size() );
		m_owner.push_back(entity);
		m_collider.push_back(std::move(collider));

		if (entity.index >= m_sparse.size())
			m_sparse.resize(static_cast<size_t>( entity.index ) + 1, kInvalidDense);

		m_sparse[entity.index] = newDense;
	}

	void ColliderStorage::createAABB(EntityId entity, const math::Vec3f& localMin, const math::Vec3f& localMax)
	{
		create(entity, std::make_unique<physics::AABBCollider>(localMin, localMax));
	}

	void ColliderStorage::destroy(EntityId entity)
	{
		const u32 idx = denseIndexOf(entity);
		if (idx == kInvalidDense)
			return;

		swapRemoveDense(idx);
		m_sparse[entity.index] = kInvalidDense;
	}

	bool ColliderStorage::contains(EntityId entity) const
	{
		if (!entity.isValid() || entity.index >= m_sparse.size())
			return false;

		const u32 dense = m_sparse[entity.index];
		if (dense == kInvalidDense)
			return false;

		return m_owner[dense] == entity;
	}

	physics::Collider& ColliderStorage::get(EntityId entity)
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "ColliderStorage::get: entity not present");
		return *m_collider[idx];
	}

	const physics::Collider& ColliderStorage::get(EntityId entity) const
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "ColliderStorage::get: entity not present");
		return *m_collider[idx];
	}

	u32 ColliderStorage::denseIndexOf(EntityId entity) const
	{
		if (!contains(entity))
			return kInvalidDense;
		return m_sparse[entity.index];
	}

	void ColliderStorage::swapRemoveDense(u32 idx)
	{
		const u32 last = static_cast<u32>( m_owner.size() ) - 1;
		if (idx != last)
		{
			m_owner[idx] = m_owner[last];
			m_collider[idx] = std::move(m_collider[last]);
			m_sparse[m_owner[idx].index] = idx;
		}

		m_owner.pop_back();
		m_collider.pop_back();
	}
}
