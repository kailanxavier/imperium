#include <ecs/transform_storage.h>
#include <algorithm>
#include <cassert>

namespace imp::ecs
{
	EntityId TransformStorage::create(EntityId entity, const Transform& local, EntityId parent)
	{
		assert(!contains(entity) && "TransformStorage::create: entity already present");

		u32 parentDense = kInvalidDense;
		u16 depth = 0;

		if (parent.isValid())
		{
			assert(contains(parent) && "TransformStorage::create: parent not present in this storage");
			parentDense = denseIndexOf(parent);
			depth = static_cast<u16>( m_depth[parentDense] + 1 );
		}

		const u32 newDense = static_cast<u32>( m_owner.size() );

		m_owner.push_back(entity);
		m_localPos.push_back(local.position);
		m_localRot.push_back(local.rotation);
		m_localScale.push_back(local.scale);
		m_localMatrix.push_back(makeTRS(local.position, local.rotation, local.scale));
		m_worldMatrix.push_back(Mat4f::identity());
		m_parentDense.push_back(parentDense);
		m_depth.push_back(depth);
		m_dirty.push_back(1);
		m_children.emplace_back();

		if (parentDense != kInvalidDense)
			m_children[parentDense].push_back(newDense);

		if (entity.index >= m_sparse.size())
			m_sparse.resize(static_cast<size_t>( entity.index ) + 1, kInvalidDense);

		m_sparse[entity.index] = newDense;
		m_updateOrderDirty = true;

		return entity;
	}

	void TransformStorage::destroy(EntityId entity)
	{
		if (!contains(entity)) return;

		std::vector<EntityId> subtree;
		collectSubtree(denseIndexOf(entity), subtree);

		// subtree is parent before child so we destroy in reverse,
		// meaning all entity's children are already gone by the time we get to them
		for (auto it = subtree.rbegin(); it != subtree.rend(); ++it)
			destroySingle(*it);
	}

	void TransformStorage::collectSubtree(u32 dense, std::vector<EntityId>& out) const
	{
		out.push_back(m_owner[dense]);
		for (u32 child : m_children[dense])
			collectSubtree(child, out);
	}

	void TransformStorage::destroySingle(EntityId entity)
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "TransformStorage::destroySingle: entity not present");
		assert(m_children[idx].empty() && "TransformStorage::destroySingle: entity still has live children");

		swapRemoveDense(idx);
		m_sparse[entity.index] = kInvalidDense;
		m_updateOrderDirty = true;
	}

	void TransformStorage::swapRemoveDense(u32 idx)
	{
		const u32 last = static_cast<u32>( m_owner.size() ) - 1;
		const u32 parentDense = m_parentDense[idx];
		if (parentDense != kInvalidDense)
		{
			auto& siblings = m_children[parentDense];
			auto it = std::find(siblings.begin(), siblings.end(), idx);
			// someone lost their kid lol
			assert(it != siblings.end() && "swapRemoveDense: idx missing from parent's child list");
			*it = siblings.back();
			siblings.pop_back();
		}

		if (idx != last)
		{
			m_owner[idx] = m_owner[last];
			m_localPos[idx] = m_localPos[last];
			m_localRot[idx] = m_localRot[last];
			m_localScale[idx] = m_localScale[last];
			m_localMatrix[idx] = m_localMatrix[last];
			m_worldMatrix[idx] = m_worldMatrix[last];
			m_parentDense[idx] = m_parentDense[last];
			m_depth[idx] = m_depth[last];
			m_dirty[idx] = m_dirty[last];
			m_children[idx] = std::move(m_children[last]);

			// The moved entity had its slot changed,
			// so we point its sparse entry at idx
			m_sparse[m_owner[idx].index] = idx;

			// New parent slot
			for (u32 child : m_children[idx])
				m_parentDense[child] = idx;

			const u32 movedParentDense = m_parentDense[idx];
			if (movedParentDense != kInvalidDense)
			{
				auto& siblings = m_children[movedParentDense];
				auto it = std::find(siblings.begin(), siblings.end(), last);
				assert(it != siblings.end() && "swapRemoveDense: last missing from its parent's child list");
				*it = idx;
			}
		}

		m_owner.pop_back();
		m_localPos.pop_back();
		m_localRot.pop_back();
		m_localScale.pop_back();
		m_localMatrix.pop_back();
		m_worldMatrix.pop_back();
		m_parentDense.pop_back();
		m_depth.pop_back();
		m_dirty.pop_back();
		m_children.pop_back();
	}

	bool TransformStorage::contains(EntityId entity) const
	{
		if (!entity.isValid() || entity.index >= m_sparse.size())
			return false;

		const u32 dense = m_sparse[entity.index];
		if (dense == kInvalidDense)
			return false;

		return m_owner[dense] == entity;
	}

	u32 TransformStorage::denseIndexOf(EntityId entity) const
	{
		if (!contains(entity))
			return kInvalidDense;

		return m_sparse[entity.index];
	}

	void TransformStorage::setLocalTransform(EntityId entity, const Transform& local)
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "TransformStorage::setLocalTransform: entity not present");

		if (idx == kInvalidDense)
			return;

		m_localPos[idx] = local.position;
		m_localRot[idx] = local.rotation;
		m_localScale[idx] = local.scale;
		m_localMatrix[idx] = makeTRS(local.position, local.rotation, local.scale);
		m_dirty[idx] = 1;
	}

	Transform TransformStorage::localTransform(EntityId entity) const
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "TransformStorage::localTransform: entity not present");
		if (idx == kInvalidDense)
			return Transform{};

		return Transform{ m_localPos[idx], m_localRot[idx], m_localScale[idx] };
	}

	const Mat4f& TransformStorage::worldMatrix(EntityId entity) const
	{
		static const Mat4f kFallback = Mat4f::identity();
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "TransformStorage::worldMatrix: entity not present");

		if (idx == kInvalidDense)
			return kFallback;

		return m_worldMatrix[idx];
	}

	EntityId TransformStorage::parentOf(EntityId entity) const
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "TransformStorage::parentOf: entity not present");

		if (idx == kInvalidDense)
			return kInvalidEntity;

		const u32 parentDense = m_parentDense[idx];
		return ( parentDense == kInvalidDense ) ? kInvalidEntity : m_owner[parentDense];
	}

	u16 TransformStorage::depthOf(EntityId entity) const
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "TransformStorage::depthOf: entity not present");
		return ( idx == kInvalidDense ) ? 0 : m_depth[idx];
	}

	void TransformStorage::updateWorldMatrices()
	{
		const size_t n = m_owner.size();
		if (n == 0) return;

		if (m_updateOrderDirty)
			rebuildUpdateOrder();

		m_worldUpdated.assign(n, false);

		for (const auto& [rangeStart, rangeEnd] : m_depthRanges)
		{
			for (u32 pos = rangeStart; pos < rangeEnd; ++pos)
			{
				const u32 i = m_updateOrder[pos];
				const u32 parentDense = m_parentDense[i];
				const bool parentRecomputed = ( parentDense != kInvalidDense ) && m_worldUpdated[parentDense];
				const bool needsUpdate = m_dirty[i] || parentRecomputed;

				if (!needsUpdate)
				{
					m_worldUpdated[i] = 0;
					continue;
				}

				m_worldMatrix[i] = ( parentDense == kInvalidDense )
					? m_localMatrix[i]
					: ( m_worldMatrix[parentDense] * m_localMatrix[i] );

				m_worldUpdated[i] = true;
				m_dirty[i] = 0;
			}
		}
	}

	void TransformStorage::updateWorldMatricesParallel(jobs::JobSystem& jobSystem, u32 minChunkSize,
		const std::function<void(size_t levelIndex, u32 startRange, u32 endRange)>& onLevelComplete)
	{
		const size_t n = m_owner.size();
		if (n == 0) return;

		if (m_updateOrderDirty)
			rebuildUpdateOrder();

		m_worldUpdated.assign(n, 0);

		for (size_t levelIndex = 0; levelIndex < m_depthRanges.size(); ++levelIndex)
		{
			const auto [rangeStart, rangeEnd] = m_depthRanges[levelIndex];
			const u32 rangeSize = rangeEnd - rangeStart;

			jobSystem.parallelFor(rangeSize, minChunkSize, [this, rangeStart](u32 chunkStart, u32 chunkEnd)
				{
					for (u32 offset = chunkStart; offset < chunkEnd; ++offset)
					{
						const u32 i = m_updateOrder[rangeStart + offset];
						const u32 parentDense = m_parentDense[i];

						// Safe to read m_worldUpdated[parentDense] here even
						// though it was written by a possible different worker thread:
						// parentDense is always in an earlier depth range, which already
						// went through parallelFor's blocking wait()
						const bool parentRecomputed = ( parentDense != kInvalidDense )
							&& ( m_worldUpdated[parentDense] != 0 );

						const bool needsUpdate = m_dirty[i] || parentRecomputed;

						if (!needsUpdate)
						{
							m_worldUpdated[i] = 0;
							continue;
						}

						m_worldMatrix[i] = ( parentDense == kInvalidDense )
							? m_localMatrix[i]
							: ( m_worldMatrix[parentDense] * m_localMatrix[i] );

						m_worldUpdated[i] = 1;
						m_dirty[i] = 0;
					}
				});

			if (onLevelComplete)
				onLevelComplete(levelIndex, rangeStart, rangeEnd);
		}
	}

	void TransformStorage::rebuildUpdateOrder()
	{
		if (!m_updateOrderDirty)
			return;

		const u32 n = static_cast<u32>( m_owner.size() );

		m_updateOrder.resize(n);
		for (u32 i = 0; i < n; ++i)
			m_updateOrder[i] = i;

		std::stable_sort(m_updateOrder.begin(), m_updateOrder.end(),
			[this](u32 a, u32 b) { return m_depth[a] < m_depth[b]; });
		
		m_depthRanges.clear();

		if (n == 0)
		{
			m_updateOrderDirty = false;
			return;
		}

		u32 rangeStart = 0;
		u16 currentDepth = m_depth[m_updateOrder[0]];

		for (u32 pos = 1; pos < n; ++pos)
		{
			const u16 d = m_depth[m_updateOrder[pos]];
			if (d != currentDepth)
			{
				m_depthRanges.emplace_back(rangeStart, pos);
				rangeStart = pos;
				currentDepth = d;
			}
		}

		m_depthRanges.emplace_back(rangeStart, n);
		m_updateOrderDirty = false;
	}

	bool TransformStorage::reparent(EntityId entity, EntityId newParent)
	{
		const u32 idx = denseIndexOf(entity);
		if (idx == kInvalidDense)
			return false;

		u32 newParentDense = denseIndexOf(newParent);

		if (newParentDense == kInvalidDense)
			return false;
		if (newParentDense == idx)
			return false;
		if (isAncestorOfDense(idx, newParentDense))
			return false;

		if (m_parentDense[idx] == newParentDense)
			return true;

		const u32 oldParentDense = m_parentDense[idx];
		if (oldParentDense != kInvalidDense)
		{
			auto& siblings = m_children[oldParentDense];
			auto it = std::find(siblings.begin(), siblings.end(), idx);
			if (it != siblings.end())
			{
				*it = siblings.back();
				siblings.pop_back();
			}
		}

		u16 newDepth = 0;
		if (newParentDense != kInvalidDense)
		{
			newDepth = static_cast<u16>( m_depth[newParentDense] + 1 );
			m_children[newParentDense].push_back(idx);
		}

		m_parentDense[idx] = newParentDense;
		applyDepth(idx, newDepth);

		m_dirty[idx] = 1;
		m_updateOrderDirty = true;

		return true;
	}

	std::optional<Transform> TransformStorage::tryGet(EntityId entity) const
	{
		const u32 idx = denseIndexOf(entity);
		if (idx == kInvalidDense)
			return std::nullopt;

		return Transform{
			m_localPos[idx],
			m_localRot[idx],
			m_localScale[idx]
		};
	}

	void TransformStorage::applyDepth(u32 dense, u16 newDepth)
	{
		m_depth[dense] = newDepth;
		for (u32 child : m_children[dense])
			applyDepth(child, static_cast<u16>( newDepth + 1 ));
	}

	bool TransformStorage::isAncestorOfDense(u32 ancestorDense, u32 dense) const
	{
		u32 current = m_parentDense[dense];
		while (current != kInvalidDense)
		{
			if (current == ancestorDense)
				return true;
			current = m_parentDense[current];
		}
		return false;
	}
}
