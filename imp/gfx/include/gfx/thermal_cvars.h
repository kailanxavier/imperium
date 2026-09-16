#pragma once
#include <core/config/cvar.h>
namespace imp::gfx::thermal
{
	inline CVarBool cvarEnabled{ "thermal.enabled", true };
	inline CVarFloat cvarHysteresis{ "thermal.hysteresis", 0.98f };
	inline CVarFloat cvarInputScale{ "thermal.input_scale", 1.f };
	inline CVarFloat cvarGlowIntensity{ "thermal.glow_intensity", 0.3f };
	inline CVarFloat cvarIgnitionThreshold{ "thermal.ignition_threshold", 0.05f };
	inline CVarFloat cvarMaxHeat{ "thermal.max_heat", 3.f };
	inline CVarFloat cvarFallbackProbeSpacing{ "thermal.fallback_probe_spacing", 2.f };
}
