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

		const auto insertIt = std::upper_bound(m_depth.begin(), m_depth.end(), depth);
		const u32 insertPos = static_cast<u32>( insertIt - m_depth.begin() );

		for (u32 i = insertPos; i < m_owner.size(); ++i)
			m_sparse[m_owner[i].index] = i + 1;

		for (auto& pd : m_parentDense)
			if (pd != kInvalidDense && pd >= insertPos)
				++pd;

		m_owner.insert(m_owner.begin() + insertPos, entity);
		m_localPos.insert(m_localPos.begin() + insertPos, local.position);
		m_localRot.insert(m_localRot.begin() + insertPos, local.rotation);
		m_localScale.insert(m_localScale.begin() + insertPos, local.scale);
		m_worldMatrix.insert(m_worldMatrix.begin() + insertPos, Mat4f::identity());
		m_parentDense.insert(m_parentDense.begin() + insertPos, parentDense);
		m_depth.insert(m_depth.begin() + insertPos, depth);
		m_dirty.insert(m_dirty.begin() + insertPos, true);

		if (entity.index >= m_sparse.size())
			m_sparse.resize(static_cast<size_t>( entity.index ) + 1, kInvalidDense);

		m_sparse[entity.index] = insertPos;

		return entity;
	}

	void TransformStorage::destroy(EntityId entity)
	{
		if (!contains(entity)) return;

		const u32 idx = denseIndexOf(entity);
		bool hasChildren = false;

		for (u32 pd : m_parentDense)
		{
			if (pd == idx)
			{
				hasChildren = true;
				break;
			}
		}

		assert(!hasChildren && "TransformStorage::destroy: entity has live children. Cascading destroy not implemented yet so they must be destroyed first.");

		if (hasChildren)
			return;

		m_owner.erase(m_owner.begin() + idx);
		m_localPos.erase(m_localPos.begin() + idx);
		m_localRot.erase(m_localRot.begin() + idx);
		m_localScale.erase(m_localScale.begin() + idx);
		m_worldMatrix.erase(m_worldMatrix.begin() + idx);
		m_parentDense.erase(m_parentDense.begin() + idx);
		m_depth.erase(m_depth.begin() + idx);
		m_dirty.erase(m_dirty.begin() + idx);

		for (u32 i = idx; i < m_owner.size(); ++i)
			m_sparse[m_owner[i].index] = i;

		for (auto& pd : m_parentDense)
			if (pd != kInvalidDense && pd > idx)
				--pd;

		m_sparse[entity.index] = kInvalidDense;
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
		m_dirty[idx] = true;
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
		m_recomputedThisPass.assign(n, false);
		
		for (size_t i = 0; i < n; ++i)
		{
			const u32 parentDense = m_parentDense[i];
			const bool parentRecomputed = ( parentDense != kInvalidDense ) && m_recomputedThisPass[parentDense];
			const bool needsUpdate = m_dirty[i] || parentRecomputed;
			if (!needsUpdate)
				continue;

			const Mat4f local = makeTRS(m_localPos[i], m_localRot[i], m_localScale[i]);
			m_worldMatrix[i] = ( parentDense == kInvalidDense ) ? local : ( m_worldMatrix[parentDense] * local );

			m_recomputedThisPass[i] = true;
			m_dirty[i] = false;
		}
	}

	void TransformStorage::updateWorldMatricesParallel(jobs::JobSystem& jobSystem, u32 minChunkSize,
		const std::function<void(size_t levelIndex, u32 startRange, u32 endRange)>& onLevelComplete)
	{
		const size_t n = m_owner.size();
		if (n == 0)
			return;

		m_recomputedThisPass.assign(n, 0);
		const auto ranges = computeDepthRanges();

		for (size_t levelIndex = 0; levelIndex < ranges.size(); ++levelIndex)
		{
			const auto [rangeStart, rangeEnd] = ranges[levelIndex];
			const u32 rangeSize = rangeEnd - rangeStart;

			jobSystem.parallelFor(rangeSize, minChunkSize, [this, rangeStart](u32 chunkStart, u32 chunkEnd)
				{
					for (u32 offset = chunkStart; offset < chunkEnd; ++offset)
					{
						const u32 i = rangeStart + offset;
						const u32 parentDense = m_parentDense[i];

						// Safe to read m_recomputedThisPass[parentDense] here even
						// though it was written by a possible different worker thread:
						// parentDense is always in an earlier depth range, which already
						// went through parallelFor's blocking wait()
						const bool parentRecomputed = ( parentDense != kInvalidDense ) 
							&& ( m_recomputedThisPass[parentDense] != 0);

						const bool needsUpdate = m_dirty[i] || parentRecomputed;

						if (needsUpdate)
						{
							const Mat4f local = makeTRS(m_localPos[i], m_localRot[i], m_localScale[i]);
							m_worldMatrix[i] = ( parentDense == kInvalidDense ) 
								? local 
								: ( m_worldMatrix[parentDense] * local );

							m_recomputedThisPass[i] = 1;
						}
						else
						{
							m_recomputedThisPass[i] = 0;
						}

						// NOTE: Deliberatly not touching m_dirty[i] here.
						// Reason: m_dirty is a bool std::vector, meaning it
						// is bit packed. Clearing it from inside parallel_for
						// would race across threads on logically disjoint
						// indices that happen to share a byte.
					}
				});
			if (onLevelComplete)
				onLevelComplete(levelIndex, rangeStart, rangeEnd);
		}

		// Now we clear it here in one single threaded pass now
		// that all parallel work is done, using u8 scratch buffer
		// that **WAS** safe to write concurrently
		for (size_t i = 0; i < n; ++i)
			if (m_recomputedThisPass[i])
				m_dirty[i] = false;
	}

	std::vector<std::pair<u32, u32>> TransformStorage::computeDepthRanges() const
	{
		std::vector<std::pair<u32, u32>> ranges;
		if (m_depth.empty())
			return ranges;

		u32 rangeStart = 0;
		u16 currentDepth = m_depth[0];

		for (u32 i = 1; i < m_depth.size(); ++i)
		{
			if (m_depth[i] != currentDepth)
			{
				ranges.emplace_back(rangeStart, i);
				rangeStart = i;
				currentDepth = m_depth[i];
			}
		}

		ranges.emplace_back(rangeStart, static_cast<u32>(m_depth.size()));
		return ranges;
	}
}
