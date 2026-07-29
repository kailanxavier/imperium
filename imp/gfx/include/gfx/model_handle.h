#pragma once
#include <core/types/handle.h>

namespace imp::gfx
{
	struct ModelTag;
	using ModelHandle = core::Handle<ModelTag>;

	inline constexpr ModelHandle kInvalidModelHandle{};
}
