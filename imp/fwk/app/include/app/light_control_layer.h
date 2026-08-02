#pragma once

#include <fwk/layer.h>
#include <core/math/math.h>
#include <ecs/transform.h>

namespace imp::app
{
	class LightControlLayer final : public fwk::ILayer
	{
	public:
		explicit LightControlLayer(math::Vec3f& sunDirection, ecs::Transform& t);
		void onUpdate(float deltaSeconds) override;

	private:
		math::Vec3f& m_sunDirection;
		ecs::Transform& m_pointLightT;
	};
}
