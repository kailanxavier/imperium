#pragma once

#include <app/iapp.h>
#include <fwk/layer.h>

#include <sky/sky_cvars.h>
#include <sky/sky_settings.h>
#include <sky/sky_state.h>
#include <sky/sky_system.h>

namespace imp::app
{
	class SkyLayer final : public fwk::ILayer
	{
	public:
		explicit SkyLayer(const sky::SkySettings& initial = {});

		void onAttach(AppContext& ctx) override;
		void onDetach(AppContext& ctx) override;
		void onUpdate(AppContext& ctx, float deltaSeconds) override;

		[[nodiscard]] sky::SkySystem& system() { return m_system; }
		[[nodiscard]] const sky::SkySystem& system() const { return m_system; }

	private:
		sky::SkySystem m_system;
		sky::SkyCVars m_cvars;
		sky::SkyState m_published;
	};
}
