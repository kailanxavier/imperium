#pragma once

#include <core/memory/int_types.h>

namespace imp::gfx::vulkan
{
	// Number of frames the CPU is allowed to have recorded
	// and submitted but not yet known to have finished on the GPU
	// before it must stall waiting for one to complete.
	// Both the swapchain's per-frame sync objects and
	// the command module's per-frame command buffer
	// are sized off this, therefore they must agree.
	inline constexpr u32 kMaxFramesInFlight = 2;
}