#pragma once

#include <ecs/entity.h>
#include <sol/sol.hpp>

#include <string>
#include <unordered_map>

namespace imp::fs { class VirtualFileSystem; }
namespace imp::ecs { class World; }

namespace imp::script
{
	class ScriptSystem
	{
	public:
		explicit ScriptSystem(fs::VirtualFileSystem& vfs);

		ScriptSystem(const ScriptSystem&) = delete;
		ScriptSystem& operator=(const ScriptSystem&) = delete;
		ScriptSystem(ScriptSystem&&) = delete;
		ScriptSystem& operator=(ScriptSystem&&) = delete;

		void update(ecs::World& world, float deltaSeconds);
		void onEntityDestroyed(ecs::EntityId entity);

		void reloadScript(const std::string& virtualPath);

		[[nodiscard]] size_t loadedScriptCount() const noexcept { return m_loadedScripts.size(); }
		[[nodiscard]] size_t liveInstanceCount() const noexcept { return m_selfTables.size(); }

	private:
		[[nodiscard]] sol::table loadModule(const std::string& virtualPath);
		void ensureInstance(ecs::World& world, ecs::EntityId entity);

		fs::VirtualFileSystem& m_vfs;

		ecs::World* m_world = nullptr;

		sol::state m_lua;
		std::unordered_map<ecs::EntityId, sol::table> m_selfTables;
		std::unordered_map<std::string, sol::table> m_loadedScripts;
	};
}
