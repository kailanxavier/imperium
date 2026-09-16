#pragma once
#include <gfx/resources.h>
#include <gfx/device.h>
#include <core/math/math.h>
#include <memory>

namespace imp::gfx
{
	struct DDGIVolumeDesc
	{
		math::Vec3f origin = math::Vec3f::zero();
		math::Vec3f extents = math::Vec3f(20.f, 5.f, 20.f);
		float probeSpacing{ 2.f };
	};

	class DDGIVolume
	{
	public:
		static constexpr u32 kIrradianceInteriorTexels{ 6 };
		static constexpr u32 kIrradianceTileTexels{ kIrradianceInteriorTexels + 2 };
		static constexpr u32 kDepthInteriorTexels{ 14 };
		static constexpr u32 kDepthTileTexels{ kDepthInteriorTexels + 2 };

		bool create(IDevice& device, const DDGIVolumeDesc& desc);

		[[nodiscard]] const DDGIVolumeDesc desc() const { return m_desc; }

		[[nodiscard]] u32 probeCountX() const { return m_probeCountX; }
		[[nodiscard]] u32 probeCountY() const { return m_probeCountY; }
		[[nodiscard]] u32 probeCountZ() const { return m_probeCountZ; }
		[[nodiscard]] u32 probeCount() const { return m_probeCountX * m_probeCountY * m_probeCountZ; }

		[[nodiscard]] math::Vec3f probePosition(u32 x, u32 y, u32 z) const;

		[[nodiscard]] u32 probeAtlasColumn(u32 x, u32 y) const { return x + y * m_probeCountX; }
		[[nodiscard]] u32 probeAtlasRow(u32 z) const { return z; }

		[[nodiscard]] ITexture* irradianceAtlas() const { return m_irradianceAtlas.get(); }
		[[nodiscard]] ITexture* depthAtlas() const { return m_depthAtlas.get(); }

		[[nodiscard]] IBuffer* rayBuffer() const { return m_rayBuffer.get(); }
		[[nodiscard]] u32 rayBufferCapacity() const { return m_rayBufferCapacity; }
		bool ensureRayBufferCapacity(IDevice& device, u32 requiredRayCount);

		[[nodiscard]] IBuffer* probeStateBuffer() const { return m_probeStateBuffer.get(); }

		u32 beginProbeUpdateWindow(u32 requestedCount);

		[[nodiscard]] u32 lastActiveProbeOffset() const { return m_lastActiveProbeOffset; }
		[[nodiscard]] u32 lastActiveProbeCount() const { return m_lastActiveProbeCount; }

	private:
		DDGIVolumeDesc m_desc;
		u32 m_probeCountX{ 0 };
		u32 m_probeCountY{ 0 };
		u32 m_probeCountZ{ 0 };

		std::unique_ptr<ITexture> m_irradianceAtlas;
		std::unique_ptr<ITexture> m_depthAtlas;

		std::unique_ptr<IBuffer> m_rayBuffer;
		u32 m_rayBufferCapacity{ 0 };

		std::unique_ptr<IBuffer> m_probeStateBuffer;

		u32 m_probeUpdateCursor{ 0 };
		u32 m_lastActiveProbeOffset{ 0 };
		u32 m_lastActiveProbeCount{ 0 };
	};

	struct DDGIRayTracePushConstants
	{
		u32 probeCountX{ 0 };
		u32 probeCountY{ 0 };
		u32 probeCountZ{ 0 };
		u32 raysPerProbe{ 0 };
		float maxRayDistance{ 0.f };
		float probeSpacing{ 0.f };
		float minCornerX{ 0.f };
		float minCornerY{ 0.f };
		float minCornerZ{ 0.f };
		float viewBias{ 0.f };
		float _pad0{ 0.f };
		float _pad1{ 0.f };
		math::Vec4f randomRotation{ 0.f, 0.f, 0.f, 1.f };
		u32 probeOffset{ 0 };
		u32 activeProbeCount{ 0 };
	};
	static_assert( sizeof(DDGIRayTracePushConstants) == 72
		&& "DDGIRayTracePushConstants must match the std430 layout in ddgi_ray_trace.comp" );

	struct DDGIProbeUpdatePushConstants
	{
		u32 probeCountX{ 0 };
		u32 probeCountY{ 0 };
		u32 probeCountZ{ 0 };
		u32 raysPerProbe{ 0 };
		u32 irradianceTileTexels{ 0 };
		u32 depthTileTexels{ 0 };
		u32 isDepthPass{ 0 };
		u32 isBorderPass{ 0 };
		float hysteresis{ 0.f };
		float depthSharpness{ 0.f };
	};

	struct DDGIInstanceMaterial
	{
		math::Vec4f baseColour{ 1.f, 1.f, 1.f, 1.f }; // rgb = albedo. .a unused
		math::Vec4f metallicRoughness{ 0.f, 1.f, 0.f, 0.f }; // x = metallic, y = roughness. .z,.w unused
	};
	static_assert( sizeof(DDGIInstanceMaterial) == 32 && "DDGIInstanceMaterial must stay std430 array friendly" );

	struct DDGIRayResult
	{
		math::Vec4f directionAndDistance{ 0.f, 0.f, 0.f, 0.f };
		math::Vec4f radiance{ 0.f, 0.f, 0.f, 0.f };
	};
	static_assert( sizeof(DDGIRayResult) == 32 && "DDGIRayResult must stay std430 array friendly" );

	struct DDGIProbeState
	{
		math::Vec4f offsetAndState{ 0.f, 0.f, 0.f, 1.f };
	};
	static_assert( sizeof(DDGIProbeState) == 16 && "DDGIProbeState must stay std430 array friendly" );

	struct DDGIClassifyPushConstants
	{
		u32 probeCountX{ 0 };
		u32 probeCountY{ 0 };
		u32 probeCountZ{ 0 };
		u32 raysPerProbe{ 0 };
		float maxRayDistance{ 0.f };
		float probeSpacing{ 0.f };
		float backfaceThreshold{ 0.f };
		float maxRelocationOffset{ 0.f };
		float relocationStep{ 0.f };
		float backfaceRatioHigh{ 0.f };
		float backfaceRatioLow{ 0.f };
		u32 probeOffset{ 0 };
		u32 activeProbeCount{ 0 };
	};
	static_assert( sizeof(DDGIClassifyPushConstants) == 52 &&
		"DDGIClassifyPushConstants must match the std430 layout in ddgi_ray_trace.comp" );

	struct DDGIVolumeUBO
	{
		math::Vec4f minCornerAndSpacing{ 0.f, 0.f, 0.f, 0.f };
		u32 probeCountX{ 0 };
		u32 probeCountY{ 0 };
		u32 probeCountZ{ 0 };
		u32 enabled{ 0 };
	};
	static_assert( sizeof(DDGIVolumeUBO) == 32 && "DDGIVolumeUBO must match the std140 layout in mesh.frag" );

	struct DDGIProbeDebugPushConstants
	{
		math::Mat4f viewProj;
		math::Vec4f cameraForwardAndRadius{ 0.f, 0.f, 1.f, 0.15f };
		math::Vec4f minCornerAndSpacing{ 0.f, 0.f, 0.f, 0.f };
		u32 probeCountX{ 0 };
		u32 probeCountY{ 0 };
		u32 probeCountZ{ 0 };
		u32 showInactive{ 1 };
		u32 activeWindowOffset{ 0 };
		u32 activeWindowCount{ 0 };
	};
	static_assert( sizeof(DDGIProbeDebugPushConstants) == 120
		&& "DDGIProbeDebugPushConstants must stay under the 128B push constant floor" );

	struct DDGIRayDebugPushConstants
	{
		math::Mat4f viewProj;
		math::Vec4f minCornerAndSpacing{ 0.f, 0.f, 0.f, 0.f };
		u32 probeCountX{ 0 };
		u32 probeCountY{ 0 };
		u32 probeCountZ{ 0 };
		u32 probeIndex{ 0 };
		u32 rayBase{ 0 };
		float maxRayDistance{ 0.f };
		u32 _pad0{ 0 };
		u32 _pad1{ 0 };
	};
	static_assert( sizeof(DDGIRayDebugPushConstants) == 112
		&& "DDGIRayDebugPushConstants must stay under the 128B push constant floor" );
}
