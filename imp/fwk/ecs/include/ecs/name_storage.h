#pragma once
#include <core/types/int_types.h>
#include <ecs/entity.h>
#include <string>
#include <vector>

namespace imp::ecs
{
	class NameStorage
	{
	public:
		static constexpr u32 kInvalidDense = 0xFFFFFFFFu;
		NameStorage() = default;

		void create(EntityId entity, std::string name);
		void destroy(EntityId entity);

		[[nodiscard]] bool contains(EntityId entity) const;

		void setName(EntityId entity, std::string name);
		[[nodiscard]] const std::string& name(EntityId entity) const;

		[[nodiscard]] size_t size() const noexcept { return m_owner.size(); }
		[[nodiscard]] const std::vector<EntityId>& owners() const noexcept { return m_owner; }
		[[nodiscard]] const std::vector<std::string>& names() const noexcept { return m_name; }
	private:
		u32 denseIndexOf(EntityId entity) const;
		void swapRemoveDense(u32 idx);

		std::vector<EntityId> m_owner;
		std::vector<std::string> m_name;
		std::vector<u32> m_sparse;
	};
}
