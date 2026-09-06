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

	private:
		DDGIVolumeDesc m_desc;
		u32 m_probeCountX{ 0 };
		u32 m_probeCountY{ 0 };
		u32 m_probeCountZ{ 0 };

		std::unique_ptr<ITexture> m_irradianceAtlas;
		std::unique_ptr<ITexture> m_depthAtlas;

		std::unique_ptr<IBuffer> m_rayBuffer;
		u32 m_rayBufferCapacity{ 0 };
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
		math::Vec4f randomRotation{ 0.f, 0.f, 0.f, 1.f };
	};

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

	struct DDGIVolumeUBO
	{
		math::Vec4f minCornerAndSpacing{ 0.f, 0.f, 0.f, 0.f };
		u32 probeCountX{ 0 };
		u32 probeCountY{ 0 };
		u32 probeCountZ{ 0 };
		u32 enabled{ 0 };
	};
	static_assert( sizeof(DDGIVolumeUBO) == 32 && "DDGIVolumeUBO must match the std140 layout in mesh.frag" );
}
