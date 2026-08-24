#include "script/binding.h"

#include <ecs/world.h>
#include <sol/sol.hpp>

namespace imp::script
{
	bool ScriptEntityHandle::isAlive() const noexcept
	{
		return world != nullptr && world->registry.isAlive(id);
	}

	void registerEntityBindings(sol::state& lua)
	{
		lua.new_usertype<ScriptEntityHandle>("Entity",
			"GetPosition", [](ScriptEntityHandle& h) -> sol::optional<math::Vec3f>
			{
				if (!h.isAlive() || !h.world->transforms.contains(h.id))
					return sol::nullopt;
				return h.world->transforms.localTransform(h.id).position;
			},
			"SetPosition", [](ScriptEntityHandle& h, float x, float y, float z)
			{
				if (!h.isAlive() || !h.world->transforms.contains(h.id))
					return;

				ecs::Transform local = h.world->transforms.localTransform(h.id);
				local.position = math::Vec3f{ x, y, z };
				h.world->transforms.setLocalTransform(h.id, local);
			},
			"SetRenderableVisible", [](ScriptEntityHandle& h, bool visible)
			{
				if (!h.isAlive() || !h.world->renderables.contains(h.id))
					return;

				h.world->renderables.setVisible(h.id, visible);
			}
		);

		lua.new_usertype<math::Vec3f>("vec3f",
			sol::constructors<math::Vec3f(), math::Vec3f(float, float, float)>(),
			"x", &math::Vec3f::x,
			"y", &math::Vec3f::y,
			"z", &math::Vec3f::z
		);
	}
}
