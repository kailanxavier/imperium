#pragma once

#include <core/math/math.h>
#include <core/memory/int_types.h>

#include <ecs/entity.h>
#include <ecs/transform.h>
#include <jobs/job_system.h>

#include <utility>
#include <vector>

namespace imp::ecs
{
	using namespace math;

	class TransformStorage
	{
	public:
		static constexpr u32 kInvalidDense = 0xFFFFFFFFU;

		TransformStorage() = default;

		EntityId create(EntityId entity, const Transform& local, EntityId parent = {});
		void destroy(EntityId entity);

		[[nodiscard]] bool contains(EntityId entity) const;
		void setLocalTransform(EntityId entity, const Transform& local);

		[[nodiscard]] Transform localTransform(EntityId entity) const;
		[[nodiscard]] const Mat4f& worldMatrix(EntityId entity) const;
		[[nodiscard]] EntityId parentOf(EntityId entity) const;
		[[nodiscard]] u16 depthOf(EntityId entity) const;
		[[nodiscard]] const std::vector<std::pair<u32, u32>>& depthRanges() const noexcept { return m_depthRanges; };

		[[nodiscard]] size_t size() const noexcept { return m_owner.size(); }

		void updateWorldMatrices();
		void updateWorldMatricesParallel(
			jobs::JobSystem& jobSystem, 
			u32 minChunkSize = 64,
			const std::function<void(size_t levelIndex, u32 startRange, u32 endRange)>& onLevelComplete = {});

		void rebuildDepthRanges();

		std::vector<EntityId> m_owner;
		std::vector<Vec3f> m_localPos;
		std::vector<Quaternionf> m_localRot;
		std::vector<Vec3f> m_localScale;
		std::vector<Mat4f> m_worldMatrix;
		std::vector<Mat4f> m_localMatrix;
		std::vector<u32> m_parentDense;
		std::vector<u16> m_depth;
		std::vector<u8> m_dirty;

	private:
		u32 denseIndexOf(EntityId entity) const;

		std::vector<u32> m_sparse;

		// Scratch buffer reused across updateWorldMatrices() calls to avoid one
		// allocation per frame, not persistent state, just sized to match m_owner each call.
		std::vector<u8> m_recomputedThisPass;

		std::vector<std::pair<u32, u32>> m_depthRanges;
		bool m_depthRangesDirty = true;
	};
}
