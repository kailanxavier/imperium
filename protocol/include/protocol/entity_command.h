#pragma once

#include <core/math/math.h>
#include <core/types/int_types.h>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace imp::protocol
{
	enum class EntityCommandOp : u8
	{
		SetLocalTransform = 0,
		SetName,
		SetRenderableVisible,
		SetLightColour,
		SetLightIntensity,
		Reparent,
		Destroy,
		AttachScript,
		Create,
	};

	struct EntityCommandPayload
	{
		EntityCommandOp op = EntityCommandOp::Destroy;
		u32 targetIndex = 0;
		u32 targetGeneration = 0;

		math::Vec3f vec3A;
		math::Quaternionf quatA;
		math::Vec3f vec3B;
		float floatA = 0.f;
		bool boolA = false;
		std::string stringA;

		u32 refIndex = 0xFFFFFFFFu;
		u32 refGeneration = 0;
	};

	struct EntityCommandResultPayload
	{
		EntityCommandOp op = EntityCommandOp::Destroy;
		u32 targetIndex = 0;
		u32 targetGeneration = 0;
		bool success = false;
		std::string error;
	};

	std::vector<u8> serialiseEntityCommand(const EntityCommandPayload& cmd);
	std::optional<EntityCommandPayload> deserialiseEntityCommand(std::span<const u8> payload);

	std::vector<u8> serialiseEntityCommandResult(const EntityCommandResultPayload& cmd);
	std::optional<EntityCommandResultPayload> deserialiseEntityCommandResult(std::span<const u8> payload);
}
