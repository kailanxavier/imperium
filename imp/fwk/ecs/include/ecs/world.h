#pragma once

#include <ecs/entity.h>
#include <ecs/transform_storage.h>
#include <ecs/renderable_storage.h>
#include <ecs/light_storage.h>
#include <ecs/collider_storage.h>
#include <ecs/name_storage.h>
#include <ecs/script_storage.h>

namespace imp::ecs
{
	struct EntitySpawnDesc
	{
		std::string name;
		Transform transform;
		EntityId parent{};
		ModelHandle model{};
		bool addCollider = true;
		math::Vec3f colliderHalfExtents = math::Vec3f{ 0.5f, 0.5f, 0.5f };
	};

	class World
	{
	public:
		EntityRegistry registry;
		TransformStorage transforms;
		RenderableStorage renderables;
		LightStorage lights;
		ColliderStorage colliders;
		NameStorage names;
		ScriptStorage scripts;

		EntityId createEntity() { return registry.create(); }

		EntityId spawnEntity(const EntitySpawnDesc& desc)
		{
			const EntityId id = createEntity();
			transforms.create(id, desc.transform, desc.parent);

			if (!desc.name.empty())
				names.create(id, desc.name);

			if (desc.model.isValid())
			{
				renderables.create(id, desc.model);
				if (desc.addCollider)
					colliders.createAABB(id, -desc.colliderHalfExtents, desc.colliderHalfExtents);
			}

			return id;
		}

		void destroyEntity(EntityId entity)
		{
			if (transforms.contains(entity))
				transforms.destroy(entity);
			if (renderables.contains(entity))
				renderables.destroy(entity);
			if (lights.contains(entity))
				lights.destroy(entity);
			if (colliders.contains(entity))
				colliders.destroy(entity);
			if (names.contains(entity))
				names.destroy(entity);
			if (scripts.contains(entity))
				scripts.destroy(entity);

			registry.destroy(entity);
		}
	};
}
