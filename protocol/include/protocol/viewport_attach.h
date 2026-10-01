#pragma once
#include <core/types/int_types.h>
#include <optional>
#include <span>
#include <vector>

namespace imp::protocol
{
	struct ViewportAttachPayload
	{
		u64 nativeHandle = 0;
		u32 width = 0;
		u32 height = 0;
		u32 processId = 0;
	};

	std::vector<u8> serialiseViewportAttach(const ViewportAttachPayload& payload);
	std::optional<ViewportAttachPayload> deserialiseViewportAttach(std::span<const u8> payload);
}
