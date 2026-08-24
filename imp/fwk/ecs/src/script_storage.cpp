#include <ecs/script_storage.h>
#include <cassert>

namespace imp::ecs
{
	void ScriptStorage::create(EntityId entity, std::string scriptPath, bool wantsTick)
	{
		assert(!contains(entity) && "ScriptStorage::create: entity already present");

		const u32 newDense = static_cast<u32>( m_owner.size() );
		m_owner.push_back(entity);
		m_component.push_back(ScriptComponent{ std::move(scriptPath), wantsTick });

		if (entity.index >= m_sparse.size())
			m_sparse.resize(static_cast<size_t>( entity.index ) + 1, kInvalidDense);
		m_sparse[entity.index] = newDense;
	}

	void ScriptStorage::destroy(EntityId entity)
	{
		const u32 idx = denseIndexOf(entity);
		if (idx == kInvalidDense)
			return;

		swapRemoveDense(idx);
		m_sparse[entity.index] = kInvalidDense;
	}

	bool ScriptStorage::contains(EntityId entity) const
	{
		if (!entity.isValid() || entity.index >= m_sparse.size())
			return false;

		const u32 dense = m_sparse[entity.index];
		if (dense == kInvalidDense)
			return false;

		return m_owner[dense] == entity;
	}

	void ScriptStorage::setScriptPath(EntityId entity, std::string scriptPath)
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "ScriptStorage::setScriptPath: entity not present");
		if (idx != kInvalidDense) m_component[idx].scriptPath = std::move(scriptPath);
	}

	void ScriptStorage::setWantsTick(EntityId entity, bool wantsTick)
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "ScriptStorage::setWantsTick: entity not present");
		if (idx != kInvalidDense) m_component[idx].wantsTick = wantsTick;
	}

	const std::string& ScriptStorage::scriptPath(EntityId entity) const
	{
		static const std::string kEmpty;
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "ScriptStorage::scriptPath: entity not present");
		return ( idx == kInvalidDense ) ? kEmpty : m_component[idx].scriptPath;
	}

	bool ScriptStorage::wantsTick(EntityId entity) const
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "ScriptStorage::wantsTick: entity not present");
		return ( idx != kInvalidDense ) && m_component[idx].wantsTick;
	}

	u32 ScriptStorage::denseIndexOf(EntityId entity) const
	{
		if (!contains(entity))
			return kInvalidDense;

		return m_sparse[entity.index];
	}

	void ScriptStorage::swapRemoveDense(u32 idx)
	{
		const u32 last = static_cast<u32>( m_owner.size() ) - 1;
		if (idx != last)
		{
			m_owner[idx] = m_owner[last];
			m_component[idx] = std::move(m_component[last]);
			m_sparse[m_owner[idx].index] = idx;
		}
		m_owner.pop_back();
		m_component.pop_back();
	}
}
