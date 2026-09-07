#pragma once
#include <core/config/cvar.h>
namespace imp::gfx::thermal
{
	inline CVarBool cvarEnabled{ "thermal.enabled", false };
	inline CVarFloat cvarHysteresis{ "thermal.hysteresis", 0.997f };
	inline CVarFloat cvarInputScale{ "thermal.input_scale", 1.f };
	inline CVarFloat cvarGlowIntensity{ "thermal.glow_intensity", 0.6f };
	inline CVarFloat cvarIgnitionThreshold{ "thermal.ignition_threshold", 0.05f };
	inline CVarFloat cvarMaxHeat{ "thermal.max_heat", 4.f };
	inline CVarFloat cvarFallbackProbeSpacing{ "thermal.fallback_probe_spacing", 3.f };
}
