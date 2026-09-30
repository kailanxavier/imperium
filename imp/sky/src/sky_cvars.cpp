#include <sky/sky_cvars.h>
#include <core/config/cvar.h>

namespace imp::sky
{
	SkyCVars::SkyCVars()
	{
		CVarRegistry& registry = CVarRegistry::get();
		const SkySettings defaults{};

#define SKY_FLOAT(cvarName, expr) \
		do { \
			FloatBinding b; \
			b.field = [](SkySettings& s) -> float& { return s.expr; }; \
			b.last = defaults.expr; \
			b.cvar = &registry.registerFloat(cvarName, defaults.expr); \
			m_floats.push_back(b); \
		} while (false)

#define SKY_BOOL(cvarName, expr) \
		do { \
			BoolBinding b; \
			b.field = [](SkySettings& s) -> bool& { return s.expr; }; \
			b.last = defaults.expr; \
			b.cvar = &registry.registerBool(cvarName, defaults.expr); \
			m_bools.push_back(b); \
		} while (false)

		SKY_BOOL("sky.cycle_enabled", cycleEnabled);
		SKY_FLOAT("sky.time_of_day", timeOfDayHours);
		SKY_FLOAT("sky.day_length_seconds", dayLengthSeconds);
		SKY_FLOAT("sky.time_scale", timeScale);

		SKY_FLOAT("sky.latitude", latitude);
		SKY_FLOAT("sky.axial_tilt", axialTilt);
		SKY_FLOAT("sky.day_of_year", dayOfYear);

		SKY_FLOAT("sky.sun.colour_horizon.r", sunColourHorizon.x);
		SKY_FLOAT("sky.sun.colour_horizon.g", sunColourHorizon.y);
		SKY_FLOAT("sky.sun.colour_horizon.b", sunColourHorizon.z);
		SKY_FLOAT("sky.sun.colour_zenith.r", sunColourZenith.x);
		SKY_FLOAT("sky.sun.colour_zenith.g", sunColourZenith.y);
		SKY_FLOAT("sky.sun.colour_zenith.b", sunColourZenith.z);
		SKY_FLOAT("sky.sun.colour_blend_degrees", sunColourBlendDegrees);
		SKY_FLOAT("sky.sun.intensity", sunIntensity);
		SKY_FLOAT("sky.sun.fade_start_degrees", sunLightFade.startDegrees);
		SKY_FLOAT("sky.sun.fade_end_degrees", sunLightFade.endDegrees);

		SKY_FLOAT("sky.moon.colour.r", moonColour.x);
		SKY_FLOAT("sky.moon.colour.g", moonColour.y);
		SKY_FLOAT("sky.moon.colour.b", moonColour.z);
		SKY_FLOAT("sky.moon.intensity", moonIntensity);
		SKY_FLOAT("sky.moon.phase", moonPhase);
		SKY_FLOAT("sky.moon.orbit_offset_degrees", moonOrbitOffsetDegrees);
		SKY_FLOAT("sky.moon.inclination_degrees", moonInclinationDegrees);
		SKY_FLOAT("sky.moon.fade_start_degrees", moonLightFade.startDegrees);
		SKY_FLOAT("sky.moon.fade_end_degrees", moonLightFade.endDegrees);

		SKY_FLOAT("sky.main_light.hysteresis", mainLightHysteresis);
		SKY_FLOAT("sky.main_light.swap_seconds", mainLightSwapSeconds);

		SKY_FLOAT("sky.intensity_scale", skyIntensityScale);
		SKY_FLOAT("sky.moon_intensity_scale", moonSkyIntensityScale);
		SKY_FLOAT("sky.twilight.start_degrees", twilightFade.startDegrees);
		SKY_FLOAT("sky.twilight.end_degrees", twilightFade.endDegrees);
		SKY_FLOAT("sky.stars.intensity", starIntensity);
		SKY_FLOAT("sky.stars.fade_start_degrees", starFade.startDegrees);
		SKY_FLOAT("sky.stars.fade_end_degrees", starFade.endDegrees);

		SKY_FLOAT("sky.scatter.rayleigh.r", scattering.rayleighCoefficients.x);
		SKY_FLOAT("sky.scatter.rayleigh.g", scattering.rayleighCoefficients.y);
		SKY_FLOAT("sky.scatter.rayleigh.b", scattering.rayleighCoefficients.z);
		SKY_FLOAT("sky.scatter.mie", scattering.mieCoefficient);
		SKY_FLOAT("sky.scatter.mie_anisotropy", scattering.mieAnisotropy);
		SKY_FLOAT("sky.scatter.scale", scattering.scatterScale);
		SKY_FLOAT("sky.scatter.sun_radius_degrees", scattering.sunAngularRadiusDegrees);
		SKY_FLOAT("sky.scatter.moon_radius_degrees", scattering.moonAngularRadiusDegrees);
		SKY_FLOAT("sky.scatter.ground.r", scattering.groundColour.x);
		SKY_FLOAT("sky.scatter.ground.g", scattering.groundColour.y);
		SKY_FLOAT("sky.scatter.ground.b", scattering.groundColour.z);

#undef SKY_FLOAT
#undef SKY_BOOL
	}

	void SkyCVars::pull(SkySettings& settings)
	{
		for (FloatBinding& b : m_floats)
		{
			if (*b.cvar != b.last)
			{
				b.field(settings) = *b.cvar;
				b.last = *b.cvar;
			}
		}

		for (BoolBinding& b : m_bools)
		{
			if (*b.cvar != b.last)
			{
				b.field(settings) = *b.cvar;
				b.last = *b.cvar;
			}
		}
	}

	void SkyCVars::push(const SkySettings& settings)
	{
		SkySettings copy = settings;
		for (FloatBinding& b : m_floats)
		{
			*b.cvar = b.field(copy);
			b.last = *b.cvar;
		}

		for (BoolBinding& b : m_bools)
		{
			*b.cvar = b.field(copy);
			b.last = *b.cvar;
		}
	}
}
