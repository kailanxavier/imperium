#include <sky/sky_system.h>

#include <algorithm>
#include <cmath>

namespace imp::sky
{
	namespace
	{
		constexpr float kPif = 3.14159265358979323846f;
		constexpr float kTwoPif = 2.f * kPif;
		constexpr float kDegToRad = kPif / 180.f;
		constexpr float kRadToDeg = 180.f / kPif;

		constexpr float kIntensityEpsilon = 1e-4f;

		float finiteOr(float v, float fallback)
		{
			return std::isfinite(v) ? v : fallback;
		}

		float clampf(float v, float lo, float hi)
		{
			return std::min(std::max(v, lo), hi);
		}

		float smoothstep(float edge0, float edge1, float x)
		{
			if (!( edge1 > edge0 ))
				return x >= edge1 ? 1.f : 0.f;

			const float t = clampf(( x - edge0 ) / ( edge1 - edge0 ), 0.f, 1.f);
			return t * t * ( 3.f - 2.f * t );
		}

		// This is just lerp, but to make it look more like shaders
		// I'm just gonna define it here. This also made me realise
		// the lerp in the math library is wrong, won't fix for now
		// because it's funny that it's wrong
		float mix(float a, float b, float t)
		{
			return a + ( b - a ) * t;
		}

		math::Vec3f mix(const math::Vec3f& a, const math::Vec3f& b, float t)
		{
			return a + ( b - a ) * t;
		}

		float fade(const ElevationFade& f, float elevationDegrees)
		{
			return smoothstep(f.startDegrees, f.endDegrees, elevationDegrees);
		}

		float wrapHours(float hours)
		{
			hours = std::fmod(hours, 24.f);
			if (hours < 0.f)
				hours += 24.f;
			if (hours >= 24.f)
				hours = 0.f;
			return hours;
		}

		math::Vec3f sanitiseColour(const math::Vec3f& c)
		{
			return {
				std::max(finiteOr(c.x, 0.f), 0.f),
				std::max(finiteOr(c.y, 0.f), 0.f),
				std::max(finiteOr(c.z, 0.f), 0.f)
			};
		}

		ElevationFade sanitiseFade(const ElevationFade& f, const ElevationFade& fallback)
		{
			return { finiteOr(f.startDegrees, fallback.startDegrees),
					 finiteOr(f.endDegrees,   fallback.endDegrees) };
		}

		ElevationFade sanitiseLightFade(const ElevationFade& f, const ElevationFade& fallback)
		{
			ElevationFade out = sanitiseFade(f, fallback);
			out.startDegrees = std::max(out.startDegrees, 0.f);
			out.endDegrees = std::max(out.endDegrees, 0.f);
			return out;
		}

		SkySettings& sanitise(SkySettings& s)
		{
			const SkySettings d{};

			s.timeOfDayHours = wrapHours(finiteOr(s.timeOfDayHours, d.timeOfDayHours));
			s.dayLengthSeconds = std::max(finiteOr(s.dayLengthSeconds, d.dayLengthSeconds), 0.f);
			s.timeScale = finiteOr(s.timeScale, d.timeScale);

			s.latitude = clampf(finiteOr(s.latitude, d.latitude), -90.f, 90.f);
			s.axialTilt = clampf(finiteOr(s.axialTilt, d.axialTilt), 0.f, 90.f);
			s.dayOfYear = clampf(finiteOr(s.dayOfYear, d.dayOfYear), 0.f, 365.f);

			s.sunColourHorizon = sanitiseColour(s.sunColourHorizon);
			s.sunColourZenith = sanitiseColour(s.sunColourZenith);
			s.sunColourBlendDegrees = std::max(finiteOr(s.sunColourBlendDegrees, d.sunColourBlendDegrees), 0.f);
			s.sunIntensity = std::max(finiteOr(s.sunIntensity, d.sunIntensity), 0.f);
			s.sunLightFade = sanitiseLightFade(s.sunLightFade, d.sunLightFade);

			s.moonColour = sanitiseColour(s.moonColour);
			s.moonIntensity = std::max(finiteOr(s.moonIntensity, d.moonIntensity), 0.f);
			{
				const float phase = finiteOr(s.moonPhase, d.moonPhase);
				s.moonPhase = phase - std::floor(phase);
			}

			s.moonOrbitOffsetDegrees = finiteOr(s.moonOrbitOffsetDegrees, d.moonOrbitOffsetDegrees);
			s.moonInclinationDegrees = clampf(finiteOr(s.moonInclinationDegrees, d.moonInclinationDegrees), -90.f, 90.f);
			s.moonLightFade = sanitiseLightFade(s.moonLightFade, d.moonLightFade);

			s.mainLightHysteresis = clampf(finiteOr(s.mainLightHysteresis, d.mainLightHysteresis), 0.f, 4.f);
			s.mainLightSwapSeconds = std::max(finiteOr(s.mainLightSwapSeconds, d.mainLightSwapSeconds), 0.f);

			s.skyIntensityScale = std::max(finiteOr(s.skyIntensityScale, d.skyIntensityScale), 0.f);
			s.moonSkyIntensityScale = std::max(finiteOr(s.moonSkyIntensityScale, d.moonSkyIntensityScale), 0.f);
			s.twilightFade = sanitiseFade(s.twilightFade, d.twilightFade);
			s.starFade = sanitiseFade(s.starFade, d.starFade);
			s.starIntensity = std::max(finiteOr(s.starIntensity, d.starIntensity), 0.f);

			SkyScattering& sc = s.scattering;
			sc.rayleighCoefficients = sanitiseColour(sc.rayleighCoefficients);
			sc.mieCoefficient = std::max(finiteOr(sc.mieCoefficient, d.scattering.mieCoefficient), 0.f);
			sc.mieAnisotropy = clampf(finiteOr(sc.mieAnisotropy, d.scattering.mieAnisotropy), -0.99f, 0.99f);
			sc.scatterScale = std::max(finiteOr(sc.scatterScale, d.scattering.scatterScale), 0.f);
			sc.sunAngularRadiusDegrees = clampf(finiteOr(sc.sunAngularRadiusDegrees, d.scattering.sunAngularRadiusDegrees), 0.01f, 30.f);
			sc.moonAngularRadiusDegrees = clampf(finiteOr(sc.moonAngularRadiusDegrees, d.scattering.moonAngularRadiusDegrees), 0.01f, 30.f);
			sc.groundColour = sanitiseColour(sc.groundColour);

			return s;
		}

		math::Vec3f towardBody(float latitudeRad, float declinationRad, float hourAngleRad)
		{
			const float sinLat = std::sin(latitudeRad);
			const float cosLat = std::cos(latitudeRad);
			const float sinDec = std::sin(declinationRad);
			const float cosDec = std::cos(declinationRad);
			const float sinH = std::sin(hourAngleRad);
			const float cosH = std::cos(hourAngleRad);

			const float east = -cosDec * sinH;
			const float up = sinDec * sinLat + cosDec * cosLat * cosH;
			const float north = sinDec * cosLat - cosDec * cosH * sinLat;

			return { east, up, north };
		}

		float elevationDegrees(const math::Vec3f& toward)
		{
			return std::asin(clampf(toward.y, -1.f, 1.f)) * kRadToDeg;
		}
	}

	SkySystem::SkySystem(const SkySettings& settings) : m_settings(settings)
	{
		evaluate(0.f);
	}

	const SkyState& SkySystem::update(float deltaSeconds)
	{
		deltaSeconds = std::max(finiteOr(deltaSeconds, 0.f), 0.f);

		if (m_settings.cycleEnabled)
		{
			const float dayLength = finiteOr(m_settings.dayLengthSeconds, 0.f);
			if (dayLength > 0.f)
			{
				const float hoursPerSecond = 24.f / dayLength * finiteOr(m_settings.timeScale, 1.f);
				const float now = finiteOr(m_settings.timeOfDayHours, SkySettings{}.timeOfDayHours);
				m_settings.timeOfDayHours = wrapHours(now + deltaSeconds * hoursPerSecond);
			}
		}

		evaluate(deltaSeconds);
		return m_state;
	}

	void SkySystem::setTimeOfDay(float hours)
	{
		m_settings.timeOfDayHours = wrapHours(finiteOr(hours, SkySettings{}.timeOfDayHours));
		snap();
	}

	const SkyState& SkySystem::snap()
	{
		m_needsSnap = true;
		evaluate(0.f);
		return m_state;
	}

	void SkySystem::evaluate(float deltaSeconds)
	{
		const SkySettings s = sanitise(m_settings);

		const float latitudeRad = s.latitude * kDegToRad;
		const float declinationRad = -s.axialTilt * kDegToRad * std::cos(kTwoPif * ( s.dayOfYear + 10.f ) / 365.f);
		const float hourAngleRad = ( s.timeOfDayHours - 12.f ) / 24.f * kTwoPif;

		const math::Vec3f sunToward = towardBody(latitudeRad, declinationRad, hourAngleRad);
		const float moonHourAngleRad = hourAngleRad + kTwoPif * s.moonPhase + s.moonOrbitOffsetDegrees * kDegToRad;
		const math::Vec3f moonToward = towardBody(latitudeRad, s.moonInclinationDegrees * kDegToRad, moonHourAngleRad);

		const float sunElevation = elevationDegrees(sunToward);
		const float moonElevation = elevationDegrees(moonToward);
		const float moonIllumination = 0.5f * ( 1.f - std::cos(kTwoPif * s.moonPhase) );

		SkyState next{};
		next.timeOfDayHours = s.timeOfDayHours;
		next.scattering = s.scattering;

		next.sun.direction = -sunToward;
		next.sun.colour = mix(s.sunColourHorizon, s.sunColourZenith, smoothstep(0.f, s.sunColourBlendDegrees, sunElevation));
		next.sun.intensity = s.sunIntensity * fade(s.sunLightFade, sunElevation);

		next.moon.direction = -moonToward;
		next.moon.colour = s.moonColour;
		next.moon.intensity = s.moonIntensity * moonIllumination * fade(s.moonLightFade, moonElevation);

		next.sunSkyIntensity = s.sunIntensity * s.skyIntensityScale * fade(s.twilightFade, sunElevation);
		next.moonSkyIntensity = s.moonIntensity * moonIllumination * s.moonSkyIntensityScale * fade(s.moonLightFade, moonElevation);
		next.starVisibility = s.starIntensity * ( 1.f - fade(s.starFade, sunElevation) );
		next.daylight = smoothstep(-6.f, 8.f, sunElevation);

		using Source = MainLight::Source;
		const float sunStrength = next.sun.intensity;
		const float moonStrength = next.moon.intensity;

		if (m_needsSnap)
		{
			if (sunStrength <= kIntensityEpsilon && moonStrength <= kIntensityEpsilon)
				m_activeSource = sunElevation >= moonElevation ? Source::Sun : Source::Moon;
			else
				m_activeSource = moonStrength > sunStrength ? Source::Moon : Source::Sun;

			m_swapFade = 1.f;
			m_needsSnap = false;
		}
		else
		{
			const bool sunActive = m_activeSource == Source::Sun;
			const float activeStrength = sunActive ? sunStrength : moonStrength;
			const float otherStrength = sunActive ? moonStrength : sunStrength;
			const Source other = sunActive ? Source::Moon : Source::Sun;

			const bool wantsSwap = otherStrength > activeStrength * ( 1.f + s.mainLightHysteresis ) + kIntensityEpsilon;

			if (s.mainLightSwapSeconds <= 0.f)
			{
				if (wantsSwap)
					m_activeSource = other;
				m_swapFade = 1.f;
			}
			else if (wantsSwap)
			{
				m_swapFade = std::max(m_swapFade - deltaSeconds / s.mainLightSwapSeconds, 0.f);
				if (m_swapFade <= 0.f)
					m_activeSource = other;
			}
			else
			{
				m_swapFade = std::min(m_swapFade + deltaSeconds / s.mainLightSwapSeconds, 1.f);
			}
		}

		const CelestialBody& body = m_activeSource == Source::Sun ? next.sun : next.moon;
		next.main.direction = body.direction;
		next.main.colour = body.colour;
		next.main.source = m_activeSource;
		next.main.shadowStrength = m_swapFade;
		next.main.intensity = body.intensity * m_swapFade;

		m_state = next;
	}
}
