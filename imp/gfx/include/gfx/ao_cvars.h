#pragma once
#include <core/config/cvar.h>

namespace imp::gfx::ao
{
    inline CVarBool cvarEnabled { "ao.enabled", true };
    inline CVarFloat cvarRadius { "ao.radius", 0.5f };
    inline CVarFloat cvarIntensity { "ao.intensity", 1.f };
    inline CVarFloat cvarPower { "ao.power", 1.f };
    inline CVarInt cvarSliceCount { "ao.slice_count", 2 };
    inline CVarInt cvarStepCount { "ao.step_count", 4 };
    inline CVarFloat cvarThickness { "ao.thickness", 0.2f };
    inline CVarBool cvarBlurEnabled { "ao.blur_enabled", true };
    inline CVarFloat cvarBlurDepthSigma { "ao.blur_depth_sigma", 1.f };
    inline CVarFloat cvarBlurNormalSigma { "ao.blur_normal_sigma", 8.f };
}
