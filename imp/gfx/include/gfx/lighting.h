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

	struct SkyPushConstants
	{
		math::Mat4f invViewProj;
		math::Vec4f cameraPositionWS;
	};

	static_assert( sizeof(SkyPushConstants) <= 128
		&& "SkyPushConstants must stay within the guaranteed 128-byte push constant range" );

	struct SkyUBO
	{
		math::Vec4f sunDirAndIntensity{ 0.f, 1.f, 0.f, 0.f };
		math::Vec4f sunColour{ 1.f, 1.f, 1.f, 0.f };
		math::Vec4f moonDirAndIntensity{ 0.f, -1.f, 0.f, 0.f };
		math::Vec4f moonColour{ 1.f, 1.f, 1.f, 0.f };
		math::Vec4f rayleighAndMieG{ 12.f, 8.f, 22.f, 0.82f };
		math::Vec4f scatterParams{ 55.f, 0.0008f, 0.037f, 0.031f };
		math::Vec4f groundAndStars{ 0.055f, 0.004f, 0.003f, 0.f }; // ground colour (3) and star visibility (1)
	};
	static_assert( sizeof(SkyUBO) % 16 == 0 && "SkyUBO layout must stay std140 friendly" );

	constexpr u32 kMaxLights = 16;
	constexpr u32 kMainLightSlot = 0;

	struct GPULight
	{
		math::Vec4f positionOrDirWS{ 0.f, 0.f, 0.f, 0.f };
		math::Vec4f colourIntensity{ 1.f, 1.f, 1.f, 1.f };
	};
	static_assert( sizeof(GPULight) == 32 && "GPULight must stay std140 friendly" );

	struct LightUBO
	{
		math::Vec4f cameraPositionWS{ 0.f, 0.f, 0.f, 0.f };
		math::Vec4f ambientColour{ 0.1f, 0.1f, 0.15f, 0.f };
		float specularStrength = 0.5f;
		float shininess = 32.f;
		u32 lightCount = 0;
		u32 _pad0 = 0;
		math::Mat4f sunViewProj = math::Mat4f::identity();

		math::Vec3f sunDirection = math::Vec3f::zero();
		float mainShadowStrength = 1.f; // 0..1

		GPULight lights[kMaxLights];
	};
	static_assert( sizeof(LightUBO) % 16 == 0 && "LightUBO layout must stay std140 consistent" );

	struct CascadeUBO
	{
		math::Mat4f viewProj[4];
		math::Vec4f splitDepths;
		math::Vec4f blendParams;
		math::Vec4f shadowMapSizes;
	};
	static_assert( sizeof(CascadeUBO) % 16 == 0 && "CascadeUBO layout must stay std140 consistent" );

	inline void setMainLight(LightUBO& ubo, const math::Vec3f& direction, const math::Vec3f& colour,
		float intensity, float shadowStrength)
	{
		ubo.sunDirection = direction;
		ubo.mainShadowStrength = shadowStrength;

		GPULight& light = ubo.lights[kMainLightSlot];
		light.positionOrDirWS = math::Vec4f{ direction, 0.f };
		light.colourIntensity = math::Vec4f{ colour, intensity };
	}
}
