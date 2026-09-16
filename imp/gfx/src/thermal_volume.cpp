#include <gfx/thermal_volume.h>
#include <algorithm>
#include <vector>

namespace imp::gfx
{
	bool ThermalVolume::create(IDevice& device, const ThermalVolumeDesc& desc, u32 probeCountX, u32 probeCountY, u32 probeCountZ)
	{
		m_desc = desc;

		if (probeCountX == 0 || probeCountY == 0 || probeCountZ == 0)
		{
			auto countForExtent = [&](float extent)
				{
					return static_cast<u32>( std::max(0.f, ( extent * 2.f ) / std::max(m_desc.probeSpacing, 1e-8f)) ) + 1u;
				};
			m_probeCountX = countForExtent(m_desc.extents.x);
			m_probeCountY = countForExtent(m_desc.extents.y);
			m_probeCountZ = countForExtent(m_desc.extents.z);
		}
		else
		{
			m_probeCountX = probeCountX;
			m_probeCountY = probeCountY;
			m_probeCountZ = probeCountZ;
		}

		BufferDesc heatDesc{};
		heatDesc.size = static_cast<u64>( probeCount() ) * sizeof(float);
		heatDesc.usage = BufferUsage::Storage;
		heatDesc.memoryAccess = MemoryAccess::HostVisible;
		heatDesc.debugName = "ThermalVolume heat buffer";
		m_heatBuffer = device.createBuffer(heatDesc);
		if (!m_heatBuffer)
			return false;

		std::vector<float> zeros(probeCount(), 0.f);
		m_heatBuffer->update(zeros.data(), zeros.size() * sizeof(float), 0);

		return probeCount() > 0;
	}
}
