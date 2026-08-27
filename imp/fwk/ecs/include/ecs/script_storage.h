#pragma once
#include <core/types/int_types.h>
#include <ecs/entity.h>
#include <string>
#include <vector>

namespace imp::ecs
{
	struct ScriptComponent
	{
		std::string scriptPath;
		bool wantsTick = false;
	};

	class ScriptStorage
	{
	public:
		static constexpr u32 kInvalidDense = 0xFFFFFFFFu;
		ScriptStorage() = default;

		void create(EntityId entity, std::string scriptPath, bool wantsTick = false);
		void destroy(EntityId entity);

		[[nodiscard]] bool contains(EntityId entity) const;

		void setScriptPath(EntityId entity, std::string scriptPath);
		void setWantsTick(EntityId entity, bool wantsTick);

		[[nodiscard]] const std::string& scriptPath(EntityId entity) const;
		[[nodiscard]] bool wantsTick(EntityId entity) const;

		[[nodiscard]] size_t size() const noexcept { return m_owner.size(); }
		[[nodiscard]] const std::vector<EntityId>& owners() const noexcept { return m_owner; }
		[[nodiscard]] const std::vector<ScriptComponent>& components() const noexcept { return m_component; }

	private:
		u32 denseIndexOf(EntityId entity) const;
		void swapRemoveDense(u32 idx);

		std::vector<EntityId> m_owner;
		std::vector<ScriptComponent> m_component;
		std::vector<u32> m_sparse;
	};
}
