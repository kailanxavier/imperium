#include <protocol/script_status.h>
#include "script_status_generated.h"
#include <flatbuffers/flatbuffers.h>

namespace imp::protocol
{
	std::vector<u8> serialiseScriptStatus(const ScriptStatusPayload& status)
	{
		flatbuffers::FlatBufferBuilder builder;

		const auto pathOffset = builder.CreateString(status.path);
		const auto errorOffset = builder.CreateString(status.error);

		script::ScriptStatusBuilder sb(builder);
		sb.add_path(pathOffset);
		sb.add_success(status.success);
		sb.add_error(errorOffset);
		sb.add_reloaded_at_ms(status.reloadedAtMs);
		builder.Finish(sb.Finish());

		return { builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize() };
	}

	std::optional<ScriptStatusPayload> deserialiseScriptStatus(std::span<const u8> payload)
	{
		flatbuffers::Verifier verifier(payload.data(), payload.size());
		if (!script::VerifyScriptStatusBuffer(verifier))
			return std::nullopt;

		const auto* status = script::GetScriptStatus(payload.data());
		if (!status)
			return std::nullopt;

		ScriptStatusPayload p;
		if (status->path()) p.path = status->path()->str();
		p.success = status->success();
		if (status->error()) p.error = status->error()->str();
		p.reloadedAtMs = status->reloaded_at_ms();

		return p;
	}
}
