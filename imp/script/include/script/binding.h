#pragma once
#include <ecs/entity.h>

namespace sol { class state; }
namespace imp::ecs { class World; }

namespace imp::script 
{
	struct ScriptEntityHandle
	{
		ecs::EntityId id;
		ecs::World* world = nullptr;

		[[nodiscard]] bool isAlive() const noexcept;
	};

	void registerEntityBindings(sol::state& lua);
}