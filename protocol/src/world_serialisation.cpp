#include <protocol/world_snapshot.h>
#include "world_generated.h"
#include <flatbuffers/flatbuffers.h>

namespace imp::protocol
{
	namespace
	{
		world::Vec3 toFbVec3(const math::Vec3f& v) { return world::Vec3(v.x, v.y, v.z); }
		world::Quat toFbQuat(const math::Quaternionf& q) { return world::Quat(q.x, q.y, q.z, q.w); }

		math::Vec3f fromFbVec3(const world::Vec3* v)
		{
			return v ? math::Vec3f(v->x(), v->y(), v->z()) : math::Vec3f::zero();
		}

		math::Quaternionf fromFbQuat(const world::Quat* q)
		{
			return q ? math::Quaternionf(q->x(), q->y(), q->z(), q->w()) : math::Quaternionf::identity();
		}
	}

	std::vector<u8> serialiseWorldSnapshot(const std::vector<EntitySnapshotPayload>& entities)
	{
		flatbuffers::FlatBufferBuilder builder;
		std::vector<flatbuffers::Offset<world::EntitySnapshot>> entityOffsets;
		entityOffsets.reserve(entities.size());

		for (const auto& e : entities)
		{
			const auto nameOffset = builder.CreateString(e.name);

			flatbuffers::Offset<world::TransformComponent> transformOffset;
			if (e.transform)
			{
				const auto pos = toFbVec3(e.transform->localPosition);
				const auto rot = toFbQuat(e.transform->localRotation);
				const auto scale = toFbVec3(e.transform->localScale);

				world::TransformComponentBuilder tb(builder);
				tb.add_local_position(&pos);
				tb.add_local_rotation(&rot);
				tb.add_local_scale(&scale);
				transformOffset = tb.Finish();
			}

			flatbuffers::Offset<world::RenderableComponent> renderableOffset;
			if (e.renderable)
			{
				world::RenderableComponentBuilder rb(builder);
				rb.add_has_model(e.renderable->modelIndex != 0xFFFFFFFFu);
				rb.add_model_index(e.renderable->modelIndex);
				rb.add_model_generation(e.renderable->modelGeneration);
				rb.add_visible(e.renderable->visible);
				renderableOffset = rb.Finish();
			}

			flatbuffers::Offset<world::LightComponent> lightOffset;
			if (e.light)
			{
				const auto colour = toFbVec3(e.light->colour);

				world::LightComponentBuilder lb(builder);
				lb.add_kind(static_cast<world::LightKind>( e.light->kind ));
				lb.add_colour(&colour);
				lb.add_intensity(e.light->intensity);
				lightOffset = lb.Finish();
			}

			flatbuffers::Offset<world::ScriptComponent> scriptOffset;
			if (e.script)
			{
				const auto pathOffset = builder.CreateString(e.script->path);

				world::ScriptComponentBuilder sb(builder);
				sb.add_path(pathOffset);
				sb.add_wants_tick(e.script->wantsTick);
				scriptOffset = sb.Finish();
			}

			world::EntitySnapshotBuilder esb(builder);
			esb.add_id_index(e.index);
			esb.add_id_generation(e.generation);
			esb.add_parent_index(e.parentIndex);
			esb.add_parent_generation(e.parentGeneration);
			esb.add_name(nameOffset);
			if (e.transform) esb.add_transform(transformOffset);
			if (e.renderable) esb.add_renderable(renderableOffset);
			if (e.light) esb.add_light(lightOffset);
			if (e.script) esb.add_script(scriptOffset);

			entityOffsets.push_back(esb.Finish());
		}

		const auto entitiesVec = builder.CreateVector(entityOffsets);
		world::WorldSnapshotBuilder wsb(builder);
		wsb.add_entities(entitiesVec);
		builder.Finish(wsb.Finish());

		return { builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize() };
	}

	std::optional<std::vector<EntitySnapshotPayload>> deserialiseWorldSnapshot(std::span<const u8> payload)
	{
		flatbuffers::Verifier verifier(payload.data(), payload.size());
		if (!world::VerifyWorldSnapshotBuffer(verifier))
			return std::nullopt;

		const auto* snapshot = world::GetWorldSnapshot(payload.data());
		if (!snapshot || !snapshot->entities())
			return std::nullopt;

		std::vector<EntitySnapshotPayload> out;
		out.reserve(snapshot->entities()->size());

		for (const auto* e : *snapshot->entities())
		{
			if (!e) continue;

			EntitySnapshotPayload p;
			p.index = e->id_index();
			p.generation = e->id_generation();
			p.parentIndex = e->parent_index();
			p.parentGeneration = e->parent_generation();
			if (e->name()) p.name = e->name()->str();

			if (const auto* t = e->transform())
			{
				TransformComponentPayload tp;
				tp.localPosition = fromFbVec3(t->local_position());
				tp.localRotation = fromFbQuat(t->local_rotation());
				tp.localScale = fromFbVec3(t->local_scale());
				p.transform = tp;
			}

			if (const auto* r = e->renderable())
			{
				RenderableComponentPayload rp;
				rp.modelIndex = r->has_model() ? r->model_index() : 0xFFFFFFFFu;
				rp.modelGeneration = r->model_generation();
				rp.visible = r->visible();
				p.renderable = rp;
			}

			if (const auto* l = e->light())
			{
				LightComponentPayload lp;
				lp.kind = static_cast<LightKindPayload>( l->kind() );
				lp.colour = fromFbVec3(l->colour());
				lp.intensity = l->intensity();
				p.light = lp;
			}

			if (const auto* s = e->script())
			{
				ScriptComponentPayload sp;
				if (s->path()) sp.path = s->path()->str();
				sp.wantsTick = s->wants_tick();
				p.script = sp;
			}

			out.push_back(std::move(p));
		}

		return out;
	}
}
