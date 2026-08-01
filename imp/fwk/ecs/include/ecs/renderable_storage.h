#pragma once
#include <core/types/handle.h>
#include <core/types/int_types.h>
#include <ecs/entity.h>
#include <vector>

namespace imp::gfx { struct ModelTag; }
namespace imp::ecs
{
	using ModelHandle = core::Handle<gfx::ModelTag>;

	struct RenderableRange
	{
		ModelHandle model;
		u32 start = 0;
		u32 end = 0;
	};

	class RenderableStorage
	{
	public:
		static constexpr u32 kInvalidDense = 0xFFFFFFFFU;
		RenderableStorage() = default;

		void create(EntityId entity, ModelHandle model, bool visible = true);
		void destroy(EntityId entity);

		[[nodiscard]] bool contains(EntityId entity) const;

		void setModel(EntityId entity, ModelHandle model);
		void setVisible(EntityId entity, bool visible);

		[[nodiscard]] ModelHandle model(EntityId entity) const;
		[[nodiscard]] bool visible(EntityId entity) const;

		[[nodiscard]] size_t size() const noexcept { return m_owner.size(); }
		[[nodiscard]] const std::vector<u32>& order() const;
		[[nodiscard]] const std::vector<RenderableRange>& ranges() const;

		[[nodiscard]] const std::vector<EntityId>& owners() const noexcept { return m_owner; }
		[[nodiscard]] const std::vector<ModelHandle>& models() const noexcept { return m_model; }
		[[nodiscard]] const std::vector<u8>& visibility() const { return m_visible; }

	private:
		u32 denseIndexOf(EntityId entity) const;
		void swapRemoveDense(u32 idx);
		void rebuildOrder() const;

		std::vector<EntityId> m_owner;
		std::vector<ModelHandle> m_model;
		std::vector<u8> m_visible;
		std::vector<u32> m_sparse;

		mutable std::vector<u32> m_order;
		mutable std::vector<RenderableRange> m_ranges;
		mutable bool m_orderDirty = true;
	};
}
