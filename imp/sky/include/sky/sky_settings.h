#pragma once

#include <core/math/vec3.h>

namespace imp::sky
{
	struct ElevationFade
	{
		float startDegrees = 0.f;
		float endDegrees = 1.f;
	};

	struct SkyScattering
	{
		math::Vec3f rayleighCoefficients{ 12.f, 8.f, 22.f };
		float mieCoefficient = 55.f;
		float mieAnisotropy = 0.82f;
		float scatterScale = 0.0008f;

		float sunAngularRadiusDegrees = 2.13f;
		float moonAngularRadiusDegrees = 1.8f;

		math::Vec3f groundColour{ 0.055f, 0.004f, 0.003f };
	};

	struct SkySettings
	{
		bool cycleEnabled = true;
		float timeOfDayHours = 16.f; // 00 to 24
		float dayLengthSeconds = 1200.f; // Default 20 irl mins per in game day
		float timeScale = 1.f;

		float latitude = 45.f;
		float axialTilt = 23.44f;
		float dayOfYear = 212.f;

		math::Vec3f sunColourHorizon{ 1.f, 0.42f, 0.16f };
		math::Vec3f sunColourZenith{ 1.f, 0.82f, 0.55f };
		float sunColourBlendDegrees = 30.f;
		float sunIntensity = 50.f;
		ElevationFade sunLightFade{ 0.f, 8.f };

		math::Vec3f moonColour{ 0.55f, 0.68f, 1.f };
		float moonIntensity = 2.f;
		float moonPhase = 0.5f;
		float moonOrbitOffsetDegrees = 0.f;
		float moonInclinationDegrees = 5.f;
		ElevationFade moonLightFade{ 0.f, 10.f };

		float mainLightHysteresis = 0.25f;
		float mainLightSwapSeconds = 1.5f;

		float skyIntensityScale = 0.2f;
		float moonSkyIntensityScale = 0.25f;
		ElevationFade twilightFade{ -18.f, 2.f };
		ElevationFade starFade{ -14.f, -4.f };
		float starIntensity = 0.3f;
		SkyScattering scattering;
	};
}
