#pragma once
#include <core/config/cvar.h>
namespace imp::gfx::bloom
{
	inline CVarBool cvarEnabled{ "bloom.enabled", false };
	inline CVarFloat cvarThreshold{ "bloom.threshold", 1.2f };
	inline CVarFloat cvarSoftKnee{ "bloom.soft_knee", 0.5f };
	inline CVarFloat cvarIntensity{ "bloom.intensity", 0.18f };
	inline CVarInt cvarMipCount{ "bloom.mip_count", 4 };
}
