#pragma once
#include <ecs/entity.h>
#include <core/math/math.h>
#include <optional>

static_assert( __cplusplus >= 202302L );

namespace imp::ecs { class World; }
namespace imp::physics 
{
	struct Ray
	{
		math::Vec3f origin{};
		math::Vec3f direction{};
	};

	struct RaycastHit
	{
		ecs::EntityId entity;
		float distance = 0.f;
		math::Vec3f point{};
	};

	class Raycaster
	{
	public:
		explicit Raycaster(ecs::World& world) : m_world(world) {}
		std::optional<RaycastHit> raycast(const Ray& ray, float maxDistance = 1000.f) const;

	private:
		ecs::World& m_world;
	};

}
