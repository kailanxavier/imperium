#pragma once
#include <core/types/int_types.h>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace imp::protocol
{
	struct ScriptStatusPayload
	{
		std::string path;
		bool success= false;
		std::string error;
		u64 reloadedAtMs = 0;
	};

	std::vector<u8> serialiseScriptStatus(const ScriptStatusPayload& status);
	std::optional<ScriptStatusPayload> deserialiseScriptStatus(std::span<const u8> payload);
}
