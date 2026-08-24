#pragma once
#include <core/math/math.h>
#include <core/types/int_types.h>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace imp::protocol
{
	enum class LightKindPayload : u8 { Directional = 0, Point };

	struct TransformComponentPayload
	{
		math::Vec3f localPosition;
		math::Quaternionf localRotation;
		math::Vec3f localScale;
	};

	struct RenderableComponentPayload
	{
		u32 modelIndex = 0xFFFFFFFFu;
		u32 modelGeneration = 0;
		bool visible = true;
	};

	struct LightComponentPayload
	{
		LightKindPayload kind = LightKindPayload::Directional;
		math::Vec3f colour;
		float intensity = 0.f;
	};

	struct EntitySnapshotPayload
	{
		u32 index = 0;
		u32 generation = 0;

		u32 parentIndex = 0xFFFFFFFFu;
		u32 parentGeneration = 0;

		std::string name;

		std::optional<TransformComponentPayload> transform;
		std::optional<RenderableComponentPayload> renderable;
		std::optional<LightComponentPayload> light;
	};

	std::vector<u8> serialiseWorldSnapshot(const std::vector<EntitySnapshotPayload>& entities);
	std::optional<std::vector<EntitySnapshotPayload>> deserialiseWorldSnapshot(std::span<const u8> payload);
}
