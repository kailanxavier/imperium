#pragma once
#include <core/math/math.h>
#include <core/types/int_types.h>
namespace imp::gfx
{
	struct TonemapPushConstants
	{
		float exposure = 0.f;
		float saturation = 1.f;
		float bloomIntensity = 0.f;
		u32 bloomEnabled = 0;
	};

	struct BloomDownsamplePushConstants
	{
		math::Vec2f texelSize{ 0.f, 0.f };
		float threshold = 0.f;
		float softKnee = 0.f;
		u32 applyThreshold = 0;
	};

	struct BloomUpsamplePushConstants
	{
		math::Vec2f texelSize{ 0.f, 0.f };
	};
}
