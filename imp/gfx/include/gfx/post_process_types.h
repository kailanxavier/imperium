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

	struct GBufferDebugPushConstants
	{
		u32 mode = 0;
	};

	struct DeferredLightingPushConstants
	{
		math::Mat4f invViewProj;
	};

	struct TaaResolvePushConstants
	{
		math::Mat4f currToPrevClip;
		math::Vec4f resolutionAndInv; // width, height, 1 / width and 1 / height
		float feedbackMin = 0.88f;
		float feedbackMax = 0.97f;
		float varianceGamma = 1.25f;
		u32 historyValid = 0;
	};

	static_assert(sizeof(TaaResolvePushConstants) <= 128 &&
		"TaaResolvePushConstants must stay within the guaranteed 128-byte push constant range");
}
