#include <protocol/entity_command.h>

#include "entity_command_generated.h"
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

	std::vector<u8> serialiseEntityCommand(const EntityCommandPayload& cmd)
	{
		flatbuffers::FlatBufferBuilder builder;

		const auto stringOffset = builder.CreateString(cmd.stringA);
		const auto vec3A = toFbVec3(cmd.vec3A);
		const auto quatA = toFbQuat(cmd.quatA);
		const auto vec3B = toFbVec3(cmd.vec3B);

		world::EntityCommandBuilder cb(builder);
		cb.add_op(static_cast<world::EntityCommandOp>(cmd.op));
		cb.add_target_index(cmd.targetIndex);
		cb.add_target_generation(cmd.targetGeneration);
		cb.add_vec3_a(&vec3A);
		cb.add_quat_a(&quatA);
		cb.add_vec3_b(&vec3B);
		cb.add_float_a(cmd.floatA);
		cb.add_bool_a(cmd.boolA);
		cb.add_string_a(stringOffset);
		cb.add_ref_index(cmd.refIndex);
		cb.add_ref_generation(cmd.refGeneration);
		builder.Finish(cb.Finish());

		return { builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize() };
	}

	std::optional<EntityCommandPayload> deserialiseEntityCommand(std::span<const u8> payload)
	{
		flatbuffers::Verifier verifier(payload.data(), payload.size());
		if (!verifier.VerifyBuffer<world::EntityCommand>(nullptr))
			return std::nullopt;

		const auto* cmd = flatbuffers::GetRoot<world::EntityCommand>(payload.data());
		if (!cmd) return std::nullopt;

		EntityCommandPayload p;
		p.op = static_cast<EntityCommandOp>(cmd->op());
		p.targetIndex = cmd->target_index();
		p.targetGeneration = cmd->target_generation();
		p.vec3A = fromFbVec3(cmd->vec3_a());
		p.quatA = fromFbQuat(cmd->quat_a());
		p.vec3B = fromFbVec3(cmd->vec3_b());
		p.floatA = cmd->float_a();
		p.boolA = cmd->bool_a();
		if (cmd->string_a()) p.stringA = cmd->string_a()->str();
		p.refIndex = cmd->ref_index();
		p.refGeneration = cmd->ref_generation();

		return p;
	}

	std::vector<u8> serialiseEntityCommandResult(const EntityCommandResultPayload& result)
	{
		flatbuffers::FlatBufferBuilder builder;
		const auto errorOffset = builder.CreateString(result.error);

		world::EntityCommandResultBuilder rb(builder);
		rb.add_op(static_cast<world::EntityCommandOp>(result.op));
		rb.add_target_index(result.targetIndex);
		rb.add_target_generation(result.targetGeneration);
		rb.add_success(result.success);
		rb.add_error(errorOffset);
		builder.Finish(rb.Finish());

		return { builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize() };
	}

	std::optional<EntityCommandResultPayload> deserialiseEntityCommandResult(std::span<const u8> payload)
	{
		flatbuffers::Verifier verifier(payload.data(), payload.size());
		if (!verifier.VerifyBuffer<world::EntityCommandResult>(nullptr))
			return std::nullopt;

		const auto* result = flatbuffers::GetRoot<world::EntityCommandResult>(payload.data());
		if (!result) return std::nullopt;

		EntityCommandResultPayload p;
		p.op = static_cast<EntityCommandOp>(result->op());
		p.targetIndex = result->target_index();
		p.targetGeneration = result->target_generation();
		p.success = result->success();
		if (result->error()) p.error = result->error()->str();

		return p;
	}
}
