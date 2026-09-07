#pragma once
#include <gfx/resources.h>
#include <gfx/device.h>
#include <core/math/math.h>
#include <memory>

namespace imp::gfx
{
	struct ThermalVolumeDesc
	{
		math::Vec3f origin = math::Vec3f::zero();
		math::Vec3f extents = math::Vec3f(20.f, 5.f, 20.f);
		float probeSpacing{ 2.f };
	};

	class ThermalVolume
	{
	public:
		bool create(IDevice& device, const ThermalVolumeDesc& desc, u32 probeCountX = 0, u32 probeCountY = 0, u32 probeCountZ = 0);
		[[nodiscard]] const ThermalVolumeDesc& desc() const { return m_desc; }
		[[nodiscard]] math::Vec3f minCorner() const { return m_desc.origin - m_desc.extents; }

		[[nodiscard]] u32 probeCountX() const { return m_probeCountX; }
		[[nodiscard]] u32 probeCountY() const { return m_probeCountY; }
		[[nodiscard]] u32 probeCountZ() const { return m_probeCountZ; }
		[[nodiscard]] u32 probeCount() const { return m_probeCountX * m_probeCountY * m_probeCountZ; }

		[[nodiscard]] IBuffer* heatBuffer() const { return m_heatBuffer.get(); }

	private:
		ThermalVolumeDesc m_desc;
		u32 m_probeCountX{ 0 };
		u32 m_probeCountY{ 0 };
		u32 m_probeCountZ{ 0 };
		std::unique_ptr<IBuffer> m_heatBuffer;
	};

	struct ThermalUpdatePushConstants
	{
		u32 probeCountX{ 0 };
		u32 probeCountY{ 0 };
		u32 probeCountZ{ 0 };
		u32 raysPerProbe{ 0 };
		float hysteresis{ 0.f };
		float inputScale{ 0.f };
		float maxHeat{ 0.f };
		float _pad0{ 0.f };
		math::Vec3f minCorner{ 0.f, 0.f, 0.f };
		float probeSpacing{ 0.f };
	};

	struct ThermalVolumeUBO
	{
		math::Vec4f minCornerAndSpacing{ 0.f, 0.f, 0.f, 0.f };
		u32 probeCountX{ 0 };
		u32 probeCountY{ 0 };
		u32 probeCountZ{ 0 };
		u32 enabled{ 0 };
		float glowIntensity{ 0.f };
		float ignitionThreshold{ 0.f };
		float _pad0{ 0.f };
		float _pad1{ 0.f };
	};
	static_assert( sizeof(ThermalVolumeUBO) == 48 && "ThermalVolumeUBO must match the std140 layout in mesh.frag" );
}
