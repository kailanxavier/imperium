#include <protocol/scene_command.h>
#include "scene_command_generated.h"
#include <flatbuffers/flatbuffers.h>

namespace imp::protocol
{
	std::vector<u8> serialiseSceneCommand(const SceneCommandPayload& cmd)
	{
		flatbuffers::FlatBufferBuilder builder;

		const auto pathOffset = builder.CreateString(cmd.path);

		scene::SceneCommandBuilder cb(builder);
		cb.add_op(static_cast<scene::SceneCommandOp>( cmd.op ));
		cb.add_path(pathOffset);
		builder.Finish(cb.Finish());

		return { builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize() };
	}

	std::optional<SceneCommandPayload> deserialiseSceneCommand(std::span<const u8> payload)
	{
		flatbuffers::Verifier verifier(payload.data(), payload.size());
		if (!scene::VerifySceneCommandBuffer(verifier))
			return std::nullopt;

		const auto* cmd = scene::GetSceneCommand(payload.data());
		if (!cmd)
			return std::nullopt;

		SceneCommandPayload p;
		p.op = static_cast<SceneCommandOp>( cmd->op() );
		if (cmd->path()) p.path = cmd->path()->str();

		return p;
	}

	std::vector<u8> serialiseSceneCommandResult(const SceneCommandResultPayload& result)
	{
		flatbuffers::FlatBufferBuilder builder;

		const auto pathOffset = builder.CreateString(result.path);
		const auto errorOffset = builder.CreateString(result.error);

		scene::SceneCommandResultBuilder rb(builder);
		rb.add_op(static_cast<scene::SceneCommandOp>( result.op ));
		rb.add_path(pathOffset);
		rb.add_success(result.success);
		rb.add_error(errorOffset);
		builder.Finish(rb.Finish());

		return { builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize() };
	}

	std::optional<SceneCommandResultPayload> deserialiseSceneCommandResult(std::span<const u8> payload)
	{
		flatbuffers::Verifier verifier(payload.data(), payload.size());
		if (!verifier.VerifyBuffer<scene::SceneCommandResult>(nullptr))
			return std::nullopt;

		const auto* result = flatbuffers::GetRoot<scene::SceneCommandResult>(payload.data());
		if (!result)
			return std::nullopt;

		SceneCommandResultPayload p;
		p.op = static_cast<SceneCommandOp>( result->op() );
		if (result->path()) p.path = result->path()->str();
		p.success = result->success();
		if (result->error()) p.error = result->error()->str();

		return p;
	}

}
