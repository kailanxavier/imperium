#pragma once

#include <sky/sky_settings.h>
#include <sky/sky_state.h>

namespace imp::sky
{
	class SkySystem
	{
	public:
		explicit SkySystem(const SkySettings& settings = {});

		[[nodiscard]] SkySettings& settings() { return m_settings; }
		[[nodiscard]] const SkySettings& settings() const { return m_settings; }

		const SkyState& update(float deltaSeconds);

		[[nodiscard]] const SkyState& state() const { return m_state; }

		void setTimeOfDay(float hours);
		const SkyState& snap();

	private:
		void evaluate(float deltaSeconds);

		SkySettings m_settings;
		SkyState m_state;

		MainLight::Source m_activeSource = MainLight::Source::Sun;
		float m_swapFade = 1.f;
		bool m_needsSnap = true;
	};
}
