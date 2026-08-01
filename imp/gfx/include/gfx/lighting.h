#pragma once

#include <core/math/math.h>
#include <core/types/int_types.h>

namespace imp::gfx
{
	struct MeshPushConstants
	{
		math::Mat4f viewProj;
		math::Mat4f nodeWorld;
	};

	static_assert( sizeof(MeshPushConstants) == 128 
		&& "MeshPushConstants must stay within the guaranteed 128-byte push constant range" );

	constexpr u32 kMaxLights = 16;

	struct GPULight
	{
		math::Vec4f positionOrDirWS{ 0.f, 0.f, 0.f, 0.f };
		math::Vec4f colourIntensity{ 1.f, 1.f, 1.f, 1.f };
	};
	static_assert( sizeof(GPULight) == 32 && "GPULight must stay std140 friendly" );

	struct LightUBO
	{
		math::Vec4f cameraPositionWS{ 0.f, 0.f, 0.f, 0.f };
		math::Vec4f ambientColour{ 0.08f, 0.08f, 0.1f, 0.f };
		float specularStrength = 0.5f;
		float shininess = 32.f;
		u32 lightCount = 0;
		u32 _pad0 = 0;
		math::Mat4f sunViewProj = math::Mat4f::identity();

		math::Vec3f sunDirection = math::Vec3f::zero();
		float shadowMapSize = 0.f;

		GPULight lights[kMaxLights];
	};
	static_assert( sizeof(LightUBO) % 16 == 0 && "LightUBO layout must stay std140 consistent" );
}
