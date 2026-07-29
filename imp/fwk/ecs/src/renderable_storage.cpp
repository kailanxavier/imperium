#include <ecs/renderable_storage.h>
#include <algorithm>
#include <cassert>

namespace imp::ecs
{
	void RenderableStorage::create(EntityId entity, ModelHandle model, bool visible)
	{
		assert(!contains(entity) && "RenderableStorage::create: entity already present");
		
		const u32 newDense = static_cast<u32>( m_owner.size() );

		m_owner.push_back(entity);
		m_model.push_back(model);
		m_visible.push_back(visible ? 1 : 0);

		if (entity.index >= m_sparse.size())
			m_sparse.resize(static_cast<size_t>( entity.index ) + 1, kInvalidDense);

		m_sparse[entity.index] = newDense;
		m_orderDirty = true;
	}

	void RenderableStorage::destroy(EntityId entity)
	{
		const u32 idx = denseIndexOf(entity);
		if (idx == kInvalidDense)
			return;

		swapRemoveDense(idx);
		m_sparse[entity.index] = kInvalidDense;
		m_orderDirty = true;
	}

	bool RenderableStorage::contains(EntityId entity) const
	{
		if (!entity.isValid() || entity.index >= m_sparse.size())
			return false;

		const u32 dense = m_sparse[entity.index];
		if (dense == kInvalidDense)
			return false;

		return m_owner[dense] == entity;
	}

	void RenderableStorage::setModel(EntityId entity, ModelHandle model)
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "RenderableStorage::setModel: entity not present");
		if (idx == kInvalidDense)
			return;

		if (!( m_model[idx] == model ))
		{
			m_model[idx] = model;
			m_orderDirty = true;
		}
	}

	void RenderableStorage::setVisible(EntityId entity, bool visible)
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "RenderableStorage::setVisible: entity not present");
		if (idx == kInvalidDense)
			return;

		m_visible[idx] = visible ? 1 : 0;
	}

	ModelHandle RenderableStorage::model(EntityId entity) const
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "RenderableStorage::model: entity not present");
		return ( idx == kInvalidDense ) ? ModelHandle{} : m_model[idx];
	}

	bool RenderableStorage::visible(EntityId entity) const
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "RenderableStorage::visible: entity not present");
		return ( idx == kInvalidDense ) ? false : ( m_visible[idx] != 0 );
	}

	const std::vector<u32>& RenderableStorage::order() const
	{
		rebuildOrder();
		return m_order;
	}

	const std::vector<RenderableRange>& RenderableStorage::ranges() const
	{
		rebuildOrder();
		return m_ranges;
	}



	u32 RenderableStorage::denseIndexOf(EntityId entity) const
	{
		if (!contains(entity))
			return kInvalidDense;

		return m_sparse[entity.index];
	}

	void RenderableStorage::swapRemoveDense(u32 idx)
	{
		const u32 last = static_cast<u32>( m_owner.size() ) - 1;
		if (idx != last)
		{
			m_owner[idx] = m_owner[last];
			m_model[idx] = m_model[last];
			m_visible[idx] = m_visible[last];
			m_sparse[m_owner[idx].index] = idx;
		}

		m_owner.pop_back();
		m_model.pop_back();
		m_visible.pop_back();
	}

	void RenderableStorage::rebuildOrder() const
	{
		if (!m_orderDirty)
			return;

		const u32 n = static_cast<u32>( m_owner.size() );

		m_order.resize(n);
		for (u32 i = 0; i < n; ++i)
			m_order[i] = i;

		std::stable_sort(m_order.begin(), m_order.end(),
			[this](u32 a, u32 b) { return m_model[a] < m_model[b]; });

		m_ranges.clear();
		if (n == 0)
		{
			m_orderDirty = false;
			return;
		}

		u32 rangeStart = 0;
		ModelHandle currentModel = m_model[m_order[0]];

		for (u32 pos = 1; pos < n; ++pos)
		{
			const ModelHandle h = m_model[m_order[pos]];
			if (!( h == currentModel ))
			{
				m_ranges.push_back(RenderableRange{ currentModel, rangeStart, pos });
				rangeStart = pos;
				currentModel = h;
			}
		}

		m_ranges.push_back(RenderableRange{ currentModel, rangeStart, n });
		m_orderDirty = false;
	}
}
