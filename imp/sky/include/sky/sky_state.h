#pragma once

#include <core/math/vec3.h>
#include <core/types/int_types.h>
#include <sky/sky_settings.h>

namespace imp::sky
{
	struct CelestialBody
	{
		math::Vec3f direction{ 0.f, -1.f, 0.f };
		math::Vec3f colour{ 1.f, 1.f, 1.f };
		float intensity = 0.f;
	};

	struct MainLight
	{
		enum class Source : u8
		{
			Sun,
			Moon,
		};

		math::Vec3f direction{ 0.f, -1.f, 0.f };
		math::Vec3f colour{ 1.f, 1.f, 1.f };
		float intensity = 0.f;
		Source source = Source::Sun;
		float shadowStrength = 1.f;
	};

	struct SkyState
	{
		CelestialBody sun;
		CelestialBody moon;
		MainLight main;

		float sunSkyIntensity = 0.f;
		float moonSkyIntensity = 0.f;
		float starVisibility = 0.f;

		float daylight = 0.f;
		float timeOfDayHours = 0.f;

		SkyScattering scattering;
	};
}
