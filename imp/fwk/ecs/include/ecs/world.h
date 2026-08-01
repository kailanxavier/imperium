#pragma once

#include <ecs/entity.h>
#include <ecs/transform_storage.h>
#include <ecs/renderable_storage.h>
#include <ecs/light_storage.h>

namespace imp::ecs
{
	class World
	{
	public:
		EntityRegistry registry;
		TransformStorage transforms;
		RenderableStorage renderables;
		LightStorage lights;

		EntityId createEntity() { return registry.create(); }

		void destroyEntity(EntityId entity)
		{
			if (transforms.contains(entity))
				transforms.destroy(entity);
			if (renderables.contains(entity))
				renderables.destroy(entity);
			if (lights.contains(entity))
				lights.destroy(entity);

			registry.destroy(entity);
		}
	};
}
