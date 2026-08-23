#pragma once
#include <core/types/int_types.h>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace imp::protocol
{
	enum class SceneCommandOp : u8
	{
		Save = 0,
		Load,
	};

	struct SceneCommandPayload
	{
		SceneCommandOp op = SceneCommandOp::Save;
		std::string path;
	};

	struct SceneCommandResultPayload
	{
		SceneCommandOp op = SceneCommandOp::Save;
		std::string path;
		bool success = false;
		std::string error;
	};

	std::vector<u8> serialiseSceneCommand(const SceneCommandPayload& cmd);
	std::optional<SceneCommandPayload> deserialiseSceneCommand(std::span<const u8> payload);

	std::vector<u8> serialiseSceneCommandResult(const SceneCommandResultPayload& result);
	std::optional<SceneCommandResultPayload> deserialiseSceneCommandResult(std::span<const u8> payload);

}
