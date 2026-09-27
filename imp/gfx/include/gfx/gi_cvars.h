#pragma once
#include <core/config/cvar.h>

namespace imp::gfx::gi
{

	// DDGI
	inline CVarBool cvarDDGIEnabled{ "gi.ddgi.enabled", true };
	inline CVarFloat cvarProbeSpacing{ "gi.ddgi.probe_spacing", 2.f };
	inline CVarInt cvarRaysPerProbe{ "gi.ddgi.rays_per_probe", 128 };
	inline CVarFloat cvarHysteresis{ "gi.ddgi.hysteresis", 0.97f };
	inline CVarFloat cvarMaxRayDistance{ "gi.ddgi.max_ray_distance", 100.f };
	inline CVarFloat cvarDepthSharpness{ "gi.ddgi.depth_sharpness", 50.f };
	inline CVarFloat cvarRayRotationMaxDegrees{ "gi.ddgi.ray_rotation_max_degrees", 0.f }; // defaulting this to 0 because it causes
																						   // too much flickering for not much visual improvement
																						   // indoors, which is the whole purpose of the DDGI anyway.
	inline CVarFloat cvarVolumeOriginX{ "gi.ddgi.volume_origin_x", 0.f };
	inline CVarFloat cvarVolumeOriginY{ "gi.ddgi.volume_origin_y", 5.f };
	inline CVarFloat cvarVolumeOriginZ{ "gi.ddgi.volume_origin_z", 0.f };
	inline CVarFloat cvarVolumeExtentX{ "gi.ddgi.volume_extent_x", 16.f };
	inline CVarFloat cvarVolumeExtentY{ "gi.ddgi.volume_extent_y", 7.f };
	inline CVarFloat cvarVolumeExtentZ{ "gi.ddgi.volume_extent_Z", 8.f };

	inline CVarFloat cvarNormalBias{ "gi.ddgi.normal_bias", 0.25f };
	inline CVarFloat cvarViewBias{ "gi.ddgi.view_bias", 0.1f };

	inline CVarInt cvarProbesPerFrame{ "gi.ddgi.probes_per_frame", 512 };

	inline CVarBool cvarShowProbes{ "gi.ddgi.debug_show_probes", true };
	inline CVarFloat cvarDebugProbeRadius{ "gi.ddgi.debug_probe_radius", 0.15f };
	inline CVarBool cvarDebugShowInactiveProbes{ "gi.ddgi.debug_show_inactive_probes", true };

	inline CVarBool cvarDebugShowRays{ "gi.ddgi.debug_show_rays", false };
	inline CVarInt cvarDebugRayProbeIndex{ "gi.ddgi.debug_ray_probe_index", -1 };

	inline CVarFloat cvarRelocationBackfaceThreshold{ "gi.ddgi.relocation_backface_threshold", 0.25f };
	inline CVarFloat cvarRelocationMaxOffset{ "gi.ddgi.relocation_max_offset", 100.f };
	inline CVarFloat cvarRelocationStep{ "gi.ddgi.relocation_step", 10.f };
	inline CVarFloat cvarClassifyBackfaceRatioHigh{ "gi.ddgi.classify_backface_ratio_high", 0.6f };
	inline CVarFloat cvarClassifyBackfaceRatioLow{ "gi.ddgi.classify_backface_ratio_low", 0.4f };

	// SSGI
	inline CVarBool cvarSSGIEnabled{ "gi.ssgi.enabled", false };
	inline CVarFloat cvarRadius{ "gi.ssgi.radius", 1.5f };
	inline CVarFloat cvarIntensity{ "gi.ssgi.intensity", 1.f };
	inline CVarInt cvarSliceCount{ "gi.ssgi.slice_count", 2 };
	inline CVarInt cvarStepCount{ "gi.ssgi.step_count", 6 };
	inline CVarFloat cvarThickness{ "gi.ssgi.thickness", 0.2f };
	inline CVarFloat cvarMaxRadiance{ "gi.ssgi.max_radiance", 4.f };

	inline CVarBool cvarBlurEnabled{ "gi.ssgi.blur_enabled", true };
	inline CVarFloat cvarBlurDepthSigma{ "gi.ssgi.blur_depth_sigma", 1.f };
	inline CVarFloat cvarBlurNormalSigma{ "gi.ssgi.blur_normal_sigma", 8.f };
}
