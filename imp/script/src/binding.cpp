#include "script/binding.h"

#include <ecs/world.h>
#include <sol/sol.hpp>

#include <format>

#include <core/log/log.h>

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
			"GetRotation", [](ScriptEntityHandle& h) -> sol::optional<math::Quaternionf>
			{
				if (!h.isAlive() || !h.world->transforms.contains(h.id))
					return sol::nullopt;
				return h.world->transforms.localTransform(h.id).rotation;
			},
			"SetRotation", sol::overload(
				[](ScriptEntityHandle& h, float x, float y, float z)
				{
					if (!h.isAlive() || !h.world->transforms.contains(h.id))
						return;

					ecs::Transform local = h.world->transforms.localTransform(h.id);
					local.rotation = math::Quaternionf::fromEuler(
						math::toRadians(x), 
						math::toRadians(y),
						math::toRadians(z)
					);
					h.world->transforms.setLocalTransform(h.id, local);
				},
				[](ScriptEntityHandle& h, const math::Quaternionf& rotation)
				{
					if (!h.isAlive() || !h.world->transforms.contains(h.id))
						return;

					ecs::Transform local = h.world->transforms.localTransform(h.id);
					local.rotation = rotation;
					h.world->transforms.setLocalTransform(h.id, local);
				}
			),
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
			"z", &math::Vec3f::z,

			sol::meta_function::to_string, [](const math::Vec3f& v)
			{
				return std::format("vec3f({}, {}, {})", v.x, v.y, v.z);
			},
			sol::meta_function::equal_to, [](const math::Vec3f& a, const math::Vec3f& b)
			{
				return a == b;
			},
			sol::meta_function::multiplication, [](const math::Vec3f& a, const math::Vec3f& b)
			{
				return a * b;
			}
		);

		lua.new_usertype<math::Quaternionf>("quatf",
			sol::constructors<math::Quaternionf(), math::Quaternionf(float, float, float, float)>(),
			"x", &math::Quaternionf::x,
			"y", &math::Quaternionf::y,
			"z", &math::Quaternionf::z,
			"w", &math::Quaternionf::w,

			sol::meta_function::to_string, [](const math::Quaternionf& q)
			{
				return std::format("quatf({}, {}, {}, {})", q.x, q.y, q.z, q.w);
			},
			sol::meta_function::equal_to, [](const math::Quaternionf& a, const math::Quaternionf& b)
			{
				return a == b;
			},
			sol::meta_function::multiplication, [](const math::Quaternionf& a, const math::Quaternionf& b)
			{
				return a * b;
			}
		);
	}

	void registerLogBindings(sol::state& lua)
	{
		lua.create_named_table("Log",
			"Info", [](const std::string& message)
			{
				imp::log::Logger::get().log(
					imp::log::LogLevel::Info,
					"Lua",
					message,
					"<Lua>",
					0
				);
			},
			"Warning", [](const std::string& message)
			{
				imp::log::Logger::get().log(
					imp::log::LogLevel::Warning,
					"Lua",
					message,
					"<Lua>",
					0
				);
			},
			"Error", [](const std::string& message)
			{
				imp::log::Logger::get().log(
					imp::log::LogLevel::Error,
					"Lua",
					message,
					"<Lua>",
					0
				);
			},
			"Fatal", [](const std::string& message)
			{
				imp::log::Logger::get().log(
					imp::log::LogLevel::Fatal,
					"Lua",
					message,
					"<Lua>",
					0
				);
			}
		);
	}
}
