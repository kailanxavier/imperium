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

	private:
		DDGIVolumeDesc m_desc;
		u32 m_probeCountX{ 0 };
		u32 m_probeCountY{ 0 };
		u32 m_probeCountZ{ 0 };

		std::unique_ptr<ITexture> m_irradianceAtlas;
		std::unique_ptr<ITexture> m_depthAtlas;
	};

	struct DDGIProbeUpdatePushConstants
	{
		u32 probeCountX{ 0 };
		u32 probeCountY{ 0 };
		u32 probeCountZ{ 0 };
		u32 irradianceTileTexels{ 0 };
		u32 depthTileTexels{ 0 };
	};
}
