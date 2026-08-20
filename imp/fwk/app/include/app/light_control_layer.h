#pragma once

#include <fwk/layer.h>
#include <core/math/math.h>
#include <ecs/transform.h>
#include <gfx/cascade_shadow.h>

namespace imp::app
{
	class LightControlLayer final : public fwk::ILayer
	{
	public:
		explicit LightControlLayer(math::Vec3f& sunDirection, ecs::Transform& t, gfx::CascadeConfig& shadowConfig);
		void onUpdate(float deltaSeconds) override;

	private:
		math::Vec3f& m_sunDirection;
		gfx::CascadeConfig& m_shadowConfig;
		ecs::Transform& m_pointLightT;
	};
}
