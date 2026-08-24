#include "script/system.h"
#include "script/binding.h"

#include <core/fs/vfs.h>
#include <core/log/log.h>
#include <ecs/world.h>

#include <utility>
#include <vector>

namespace imp::script
{
	namespace
	{
		template <typename... Args>
		void callHook(sol::table& self, const char* hookName, Args&&... args)
		{
			const sol::object candidate = self[hookName];
			if (candidate.get_type() != sol::type::function)
				return;

			const sol::protected_function fn = candidate;
			const sol::protected_function_result result = fn(self, std::forward<Args>(args)...);
			if (result.valid())
				return;

			const sol::error err = result;
			LOG_ERROR("Script", "{}() failed: {}", hookName, err.what());
		}
	}

	ScriptSystem::ScriptSystem(fs::VirtualFileSystem& vfs) : m_vfs(vfs)
	{
		m_lua.open_libraries(
			sol::lib::base,
			sol::lib::string,
			sol::lib::math,
			sol::lib::table
		);

		registerEntityBindings(m_lua);
	}

	sol::table ScriptSystem::loadModule(const std::string& virtualPath)
	{
		if (const auto it = m_loadedScripts.find(virtualPath); it != m_loadedScripts.end())
			return it->second;

		std::string source;
		if (!m_vfs.readEntireFileText(virtualPath, source))
		{
			LOG_ERROR("Script", "Could not read script '{}'", virtualPath.c_str());
			return m_lua.create_table();
		}

		const sol::protected_function_result result = m_lua.script(source, sol::script_pass_on_error);
		if (!result.valid())
		{
			const sol::error err = result;
			LOG_ERROR("Script", "Failed to compile '{}': {}", virtualPath.c_str(), err.what());
			return m_lua.create_table();
		}

		if (result.get_type() != sol::type::table)
		{
			LOG_ERROR("Script", "'{}' must return a table of hooks", virtualPath.c_str());
			return m_lua.create_table();
		}

		const sol::table module = result;
		m_loadedScripts.emplace(virtualPath, module);
		return module;
	}

	void ScriptSystem::ensureInstance(ecs::World& world, ecs::EntityId entity)
	{
		if (m_selfTables.contains(entity))
			return;

		const sol::table module = loadModule(world.scripts.scriptPath(entity));
		sol::table self = m_lua.create_table();
		sol::table metatable = m_lua.create_table();
		metatable[sol::meta_function::index] = module;
		self[sol::metatable_key] = metatable;

		callHook(self, "OnInit", ScriptEntityHandle{ entity, &world });

		m_selfTables.emplace(entity, std::move(self));
	}

	void ScriptSystem::update(ecs::World& world, float deltaSeconds)
	{
		m_world = &world;
		for (const ecs::EntityId entity : world.scripts.owners())
			ensureInstance(world, entity);

		for (auto& [entity, self] : m_selfTables)
		{
			if (!world.scripts.contains(entity) || !world.scripts.wantsTick(entity))
				continue;

			callHook(self, "OnUpdate", ScriptEntityHandle{ entity, &world }, deltaSeconds);
		}
	}

	void ScriptSystem::onEntityDestroyed(ecs::EntityId entity)
	{
		const auto it = m_selfTables.find(entity);
		if (it == m_selfTables.end())
			return;

		sol::table self = it->second;
		callHook(self, "OnDestroy", ScriptEntityHandle{ entity, m_world });

		m_selfTables.erase(it);
	}

	void ScriptSystem::reloadScript(const std::string& virtualPath)
	{
		std::string source;
		if (!m_vfs.readEntireFileText(virtualPath, source))
		{
			LOG_ERROR("Script", "Hot reload of '{}' failed: could not read file", virtualPath.c_str());
			return;
		}

		const sol::protected_function_result result = m_lua.script(source, sol::script_pass_on_error);
		if (!result.valid())
		{
			const sol::error err = result;
			LOG_ERROR("Script", "Hot reload of '{}' failed: {}", virtualPath.c_str(), err.what());
			return; // we don't update the script, just run the latest working version
		}

		if (result.get_type() != sol::type::table)
		{
			LOG_ERROR("Script", "Hot reload of '{}' failed: script must return a table of hooks", virtualPath.c_str());
			return;
		}

		const sol::table freshModule = result;

		const auto it = m_loadedScripts.find(virtualPath);
		if (it == m_loadedScripts.end())
		{
			m_loadedScripts.emplace(virtualPath, freshModule);
			return;
		}

		sol::table liveModule = it->second;

		std::vector<std::string> staleKeys;
		for (const auto& [key, value] : liveModule)
			staleKeys.push_back(key.as<std::string>());

		for (const std::string& key : staleKeys)
			liveModule[key] = sol::lua_nil;

		for (const auto& [key, value] : freshModule)
			liveModule[key] = value;
	}
}
