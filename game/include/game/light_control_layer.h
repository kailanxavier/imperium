#pragma once

#include <app/iapp.h>
#include <fwk/layer.h>
#include <core/math/math.h>
#include <gfx/cascade_shadow.h>

namespace imp::game
{
	using app::AppContext;

	struct LightControlRefs
	{
		math::Vec3f* sunDirection = nullptr;
		gfx::CascadeConfig* cascade = nullptr;
	};

	// This class is temporary. We have already planned its deletion but
	// it's still necessary until we implement the sky driver.
	class LightControlLayer final : public fwk::ILayer
	{
	public:
		LightControlLayer();
		
		void onAttach(AppContext& ctx) override;
		void onUpdate(AppContext&, float deltaSeconds) override;

	private:
		LightControlRefs m_refs{};
	};
}
