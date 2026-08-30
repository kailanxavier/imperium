#pragma once
#include <core/types/int_types.h>

namespace imp::gfx
{
	enum class RGResourceType : u8 { Texture, Buffer };
	struct RGHandle
	{
		u32 index = ~0u;
		u32 version = 0;
		RGResourceType type = RGResourceType::Texture;

		[[nodiscard]] bool isValid() const { return index != ~0u; }
		friend bool operator==(const RGHandle&, const RGHandle&) = default;
	};

	struct RGTextureHandle : RGHandle { RGTextureHandle() { type = RGResourceType::Texture; } };
	struct RGBufferHandle : RGHandle { RGBufferHandle() { type = RGResourceType::Buffer; } };

	enum class RGLoadOp { Load, Clear, DontCare };
}
