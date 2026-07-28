#pragma once

#include <ecs/entity.h>
#include <ecs/transform_storage.h>

namespace imp::ecs
{
	class World
	{
	public:
		EntityRegistry registry;
		TransformStorage transforms;

		EntityId createEntity() { return registry.create(); }
		void destroyEntity(EntityId entity)
		{
			if (transforms.contains(entity))
				transforms.destroy(entity);

			registry.destroy(entity);
		}
	};
}
