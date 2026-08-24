#include <ecs/name_storage.h>
#include <cassert>

namespace imp::ecs
{
	void NameStorage::create(EntityId entity, std::string name)
	{
		assert(!contains(entity) && "NameStorage::create: entity already present");

		const u32 newDense = static_cast<u32>( m_owner.size() );
		m_owner.push_back(entity);
		m_name.push_back(std::move(name));

		if (entity.index >= m_sparse.size())
			m_sparse.resize(static_cast<size_t>( entity.index ) + 1, kInvalidDense);
		m_sparse[entity.index] = newDense;
	}

	void NameStorage::destroy(EntityId entity)
	{
		const u32 idx = denseIndexOf(entity);
		if (idx == kInvalidDense)
			return;

		swapRemoveDense(idx);
		m_sparse[entity.index] = kInvalidDense;
	}

	bool NameStorage::contains(EntityId entity) const
	{
		if (!entity.isValid() || entity.index >= m_sparse.size())
			return false;

		const u32 dense = m_sparse[entity.index];
		if (dense == kInvalidDense)
			return false;

		return m_owner[dense] == entity;
	}

	void NameStorage::setName(EntityId entity, std::string name)
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "NameStorage::setName: entity not present");
		if (idx != kInvalidDense) m_name[idx] = std::move(name);
	}

	const std::string& NameStorage::name(EntityId entity) const
	{
		static const std::string kEmpty;
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "NameStorage::name: entity not present");
		return ( idx == kInvalidDense ) ? kEmpty : m_name[idx];
	}

	u32 NameStorage::denseIndexOf(EntityId entity) const
	{
		if (!contains(entity))
			return kInvalidDense;

		return m_sparse[entity.index];
	}

	void NameStorage::swapRemoveDense(u32 idx)
	{
		const u32 last = static_cast<u32>( m_owner.size() ) - 1;
		if (idx != last)
		{
			m_owner[idx] = m_owner[last];
			m_name[idx] = std::move(m_name[last]);
			m_sparse[m_owner[idx].index] = idx;
		}
		m_owner.pop_back();
		m_name.pop_back();
	}
}
