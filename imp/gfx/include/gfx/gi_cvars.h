#pragma once
#include <core/config/cvar.h>

namespace imp::gfx::gi
{
	inline CVarBool cvarEnabled{ "gi.enabled", true };

	inline CVarFloat cvarProbeSpacing{ "gi.probe_spacing", 2.f };
	inline CVarInt cvarRaysPerProbe{ "gi.rays_per_probe", 128 };
	inline CVarFloat cvarHysteresis{ "gi.hysteresis", 0.95f };
	inline CVarFloat cvarMaxRayDistance{ "gi.max_ray_distance", 100.f };

	inline CVarFloat cvarVolumeOriginX{ "gi.volume_origin_x", 0.f };
	inline CVarFloat cvarVolumeOriginY{ "gi.volume_origin_y", 2.f };
	inline CVarFloat cvarVolumeOriginZ{ "gi.volume_origin_z", 0.f };
	inline CVarFloat cvarVolumeExtentX{ "gi.volume_extent_x", 20.f };
	inline CVarFloat cvarVolumeExtentY{ "gi.volume_extent_y", 5.f };
	inline CVarFloat cvarVolumeExtentZ{ "gi.volume_extent_Z", 20.f };

	inline CVarFloat cvarNormalBias{ "gi.normal_bias", 0.25f };
	inline CVarFloat cvarViewBias{ "gi.view_bias", 0.1f };

	inline CVarInt cvarProbesPerFrame{ "gi.probes_per_frame", 512 };

	inline CVarBool cvarShowProbes{ "gi.debug_show_probes", false };
}
