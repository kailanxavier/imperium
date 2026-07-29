#include <ecs/entity.h>

namespace imp::ecs
{
	EntityId EntityRegistry::create()
	{
		u32 index;

		if (!m_freeList.empty())
		{
			index = m_freeList.back();
			m_freeList.pop_back();
		}
		else
		{
			index = static_cast<u32>( m_generations.size() );
			m_generations.push_back(0);
		}

		++m_aliveCount;
		return EntityId{ index, m_generations[index] };
	}

	void EntityRegistry::destroy(EntityId entity)
	{
		if (!isAlive(entity))
			return;

		++m_generations[entity.index];
		m_freeList.push_back(entity.index);
		--m_aliveCount;
	}

	bool EntityRegistry::isAlive(EntityId entity) const noexcept
	{
		return entity.isValid()
			&& entity.index < m_generations.size()
			&& m_generations[entity.index] == entity.generation;
	}
}
