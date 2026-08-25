#pragma once

#include <ecs/entity.h>
#include <string>
#include <memory>

namespace imp::fs { class VirtualFileSystem; }
namespace imp::ecs { class World; }

namespace imp::script
{
	class ScriptSystem
	{
	public:
		explicit ScriptSystem(fs::VirtualFileSystem& vfs);
		~ScriptSystem();

		ScriptSystem(const ScriptSystem&) = delete;
		ScriptSystem& operator=(const ScriptSystem&) = delete;
		ScriptSystem(ScriptSystem&&) noexcept;
		ScriptSystem& operator=(ScriptSystem&&) noexcept;

		void update(ecs::World& world, float deltaSeconds);
		void onEntityDestroyed(ecs::EntityId entity);
		void reloadScript(const std::string& virtualPath);

		[[nodiscard]] size_t loadedScriptCount() const noexcept;
		[[nodiscard]] size_t liveInstanceCount() const noexcept;

	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
