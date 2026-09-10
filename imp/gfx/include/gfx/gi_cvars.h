#pragma once
#include <core/config/cvar.h>

namespace imp::gfx::gi
{
	inline CVarBool cvarEnabled{ "gi.enabled", true };

	inline CVarFloat cvarProbeSpacing{ "gi.probe_spacing", 2.f };
	inline CVarInt cvarRaysPerProbe{ "gi.rays_per_probe", 128 };
	inline CVarFloat cvarHysteresis{ "gi.hysteresis", 0.97f };
	inline CVarFloat cvarMaxRayDistance{ "gi.max_ray_distance", 100.f };
	inline CVarFloat cvarDepthSharpness{ "gi.depth_sharpness", 50.f };
	inline CVarFloat cvarRayRotationMaxDegrees{ "gi.ray_rotation_max_degrees", 8.f };

	inline CVarFloat cvarVolumeOriginX{ "gi.volume_origin_x", 0.f };
	inline CVarFloat cvarVolumeOriginY{ "gi.volume_origin_y", 2.f };
	inline CVarFloat cvarVolumeOriginZ{ "gi.volume_origin_z", 0.f };
	inline CVarFloat cvarVolumeExtentX{ "gi.volume_extent_x", 20.f };
	inline CVarFloat cvarVolumeExtentY{ "gi.volume_extent_y", 10.f };
	inline CVarFloat cvarVolumeExtentZ{ "gi.volume_extent_Z", 20.f };

	inline CVarFloat cvarNormalBias{ "gi.normal_bias", 0.25f };
	inline CVarFloat cvarViewBias{ "gi.view_bias", 0.1f };

	inline CVarInt cvarProbesPerFrame{ "gi.probes_per_frame", 512 };

	inline CVarBool cvarShowProbes{ "gi.debug_show_probes", false };
	inline CVarFloat cvarDebugProbeRadius{ "gi.debug_probe_radius", 0.15f };
	inline CVarBool cvarDebugShowInactiveProbes{ "gi.debug_show_inactive_probes", true };

	inline CVarBool cvarDebugShowRays{ "gi.debug_show_rays", false };
	inline CVarInt cvarDebugRayProbeIndex{ "gi.debug_ray_probe_index", -1 };

	inline CVarFloat cvarRelocationBackfaceThreshold{ "gi.relocation_backface_threshold", 0.25f };
	inline CVarFloat cvarRelocationMaxOffset{ "gi.relocation_max_offset", 0.45f };
	inline CVarFloat cvarRelocationStep{ "gi.relocation_step", 0.25f };
	inline CVarFloat cvarClassifyBackfaceRatioHigh{ "gi.classify_backface_ratio_high", 0.6f };
	inline CVarFloat cvarClassifyBackfaceRatioLow{ "gi.classify_backface_ratio_low", 0.4f };
}
