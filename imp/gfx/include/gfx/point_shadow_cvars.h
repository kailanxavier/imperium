#pragma once

#include <core/config/cvar.h>

namespace imp::gfx::pointshadow
{
	inline CVarBool cvarEnabled { "pointshadow.enabled", true };
	inline CVarInt cvarResolution { "pointshadow.resolution", 1024 };
	inline CVarFloat cvarNearPlane { "pointshadow.near_plane", 0.05f };
	inline CVarFloat cvarFarPlane { "pointshadow.far_plane", 50.f };

	inline CVarFloat cvarDepthBias { "pointshadow.depth_bias", 0.02f };
	inline CVarFloat cvarNormalOffset { "pointshadow.normal_offset", 1.5f };
	inline CVarFloat cvarFilterRadius { "pointshadow.filter_radius", 1.5f };
}
