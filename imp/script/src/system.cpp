#include "script/system.h"
#include "script/binding.h"

#include <sol/sol.hpp>
#include <unordered_map>

#include <core/fs/vfs.h>
#include <core/log/log.h>
#include <ecs/world.h>
#include <protocol/script_status.h>
#include <protocol/tool_server.h>

#include <chrono>
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

	struct ScriptSystem::Impl
	{
		struct LiveInstance
		{
			sol::table self;
			std::string scriptPath;
		};

		fs::VirtualFileSystem& vfs;
		ecs::World* world = nullptr;
		sol::state lua;
		std::unordered_map<ecs::EntityId, LiveInstance> instances;
		std::unordered_map<std::string, sol::table> loadedScripts;

		explicit Impl(fs::VirtualFileSystem& v) : vfs(v)
		{
			lua.open_libraries(
				sol::lib::base,
				sol::lib::string,
				sol::lib::math,
				sol::lib::table
			);

			registerEntityBindings(lua);
			registerLogBindings(lua);
		}

		[[nodiscard]] sol::table loadModule(const std::string& virtualPath)
		{
			if (const auto it = loadedScripts.find(virtualPath); it != loadedScripts.end())
				return it->second;

			std::string source;
			if (!vfs.readEntireFileText(virtualPath, source))
			{
				LOG_ERROR("Script", "Could not read script '{}'", virtualPath.c_str());
				return lua.create_table();
			}

			const sol::protected_function_result result = lua.script(source, sol::script_pass_on_error);
			if (!result.valid())
			{
				const sol::error err = result;
				LOG_ERROR("Script", "Failed to compile '{}': {}", virtualPath.c_str(), err.what());
				return lua.create_table();
			}

			if (result.get_type() != sol::type::table)
			{
				LOG_ERROR("Script", "'{}' must return a table of hooks", virtualPath.c_str());
				return lua.create_table();
			}

			const sol::table module = result;
			loadedScripts.emplace(virtualPath, module);
			return module;
		}

		void ensureInstance(ecs::World& w, ecs::EntityId entity)
		{
			if (instances.contains(entity))
				return;

			const std::string scriptPath = w.scripts.scriptPath(entity);
			const sol::table module = loadModule(scriptPath);
			sol::table self = lua.create_table();
			sol::table metatable = lua.create_table();
			metatable[sol::meta_function::index] = module;
			self[sol::metatable_key] = metatable;

			callHook(self, "OnInit", ScriptEntityHandle{ entity, &w });

			instances.emplace(entity, LiveInstance{ std::move(self), scriptPath });
		}

		void destroyInstance(ecs::EntityId entity)
		{
			const auto it = instances.find(entity);
			if (it == instances.end())
				return;

			sol::table self = it->second.self;
			callHook(self, "OnDestroy", ScriptEntityHandle{ entity, world });

			instances.erase(it);
		}

		void reconcileAgainstEcs(ecs::World& w)
		{
			std::vector<ecs::EntityId> stale;
			for (const auto& [entity, instance] : instances)
			{
				if (!w.scripts.contains(entity) || w.scripts.scriptPath(entity) != instance.scriptPath)
					stale.push_back(entity);
			}

			for (const ecs::EntityId entity : stale)
				destroyInstance(entity);
		}
	};

	ScriptSystem::ScriptSystem(fs::VirtualFileSystem& vfs) : m_impl(std::make_unique<Impl>(vfs)) {}

	ScriptSystem::~ScriptSystem() = default;
	ScriptSystem::ScriptSystem(ScriptSystem&&) noexcept = default;
	ScriptSystem& ScriptSystem::operator=(ScriptSystem&&) noexcept = default;

	size_t ScriptSystem::loadedScriptCount() const noexcept { return m_impl->loadedScripts.size(); }
	size_t ScriptSystem::liveInstanceCount() const noexcept { return m_impl->instances.size(); }

	void ScriptSystem::update(ecs::World& world, float deltaSeconds)
	{
		m_impl->world = &world;
		m_impl->reconcileAgainstEcs(world);

		for (const ecs::EntityId entity : world.scripts.owners())
			m_impl->ensureInstance(world, entity);

		for (auto& [entity, instance] : m_impl->instances)
		{
			if (!world.scripts.contains(entity) || !world.scripts.wantsTick(entity))
				continue;
			callHook(instance.self, "OnUpdate", ScriptEntityHandle{ entity, &world }, deltaSeconds);
		}
	}

	void ScriptSystem::onEntityDestroyed(ecs::EntityId entity)
	{
		m_impl->destroyInstance(entity);
	}

	void ScriptSystem::reloadScript(const std::string& virtualPath)
	{
		auto& toolServer = protocol::ToolServer::instance();
		const auto report = [&](bool success, const std::string& error)
			{
				if (!toolServer.hasSubscribers(protocol::MessageType::ScriptStatus))
					return;

				protocol::ScriptStatusPayload status;
				status.path = virtualPath;
				status.success = success;
				status.error = error;
				status.reloadedAtMs = static_cast<u64>( std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::system_clock::now().time_since_epoch() ).count() );

				toolServer.publish(protocol::MessageType::ScriptStatus, protocol::serialiseScriptStatus(status));
			};

		std::string source;
		if (!m_impl->vfs.readEntireFileText(virtualPath, source))
		{
			const std::string error = "Could not read file";
			LOG_ERROR("Script", "Hot reload of '{}' failed: could not read file", virtualPath.c_str());
			report(false, error);
			return;
		}

		const sol::protected_function_result result = m_impl->lua.script(source, sol::script_pass_on_error);
		if (!result.valid())
		{
			const sol::error err = result;
			LOG_ERROR("Script", "Hot reload of '{}' failed: {}", virtualPath.c_str(), err.what());
			report(false, err.what()); // we don't update the script, just run the latest working version

			return;
		}

		if (result.get_type() != sol::type::table)
		{
			const std::string error = "Script must return a table of hooks";
			LOG_ERROR("Script", "Hot reload of '{}' failed: script must return a table of hooks", virtualPath.c_str());
			report(false, error);
			return;
		}

		const sol::table freshModule = result;

		const auto it = m_impl->loadedScripts.find(virtualPath);
		if (it == m_impl->loadedScripts.end())
		{
			m_impl->loadedScripts.emplace(virtualPath, freshModule);
			report(true, {});
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

		report(true, {});
	}
}
