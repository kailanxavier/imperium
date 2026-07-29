#pragma once

#include <core/math/math.h>

namespace imp::gfx
{
	struct MeshPushConstants
	{
		math::Mat4f mvp;
		math::Mat4f model;
	};

	static_assert( sizeof(MeshPushConstants) == 128 
		&& "MeshPushConstants must stay within the guaranteed 128-byte push constant range" );

	struct BlinnPhongLightUBO
	{
		math::Vec4f lightDirectionWS{ -0.4f, 1.f, -0.3f, 0.f };
		math::Vec4f lightColour{ 1.f, 1.f, 1.f, 0.f };
		math::Vec4f ambientColour{ 0.08f, 0.08f, 0.1f, 0.f };
		math::Vec4f cameraPositionWS{ 0.f, 0.f, 0.f, 0.f };
		float specularStrength = 0.5f;
		float shininess = 32.f;
		float _pad0 = 0.f;
		float _pad1 = 0.f;
	};
}
