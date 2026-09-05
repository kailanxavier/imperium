#include <gfx/ddgi_volume.h>
#include <core/log/log.h>

namespace imp::gfx
{
	bool DDGIVolume::create(IDevice& device, const DDGIVolumeDesc& desc)
	{
		if (!device.supportsRayTracing())
			return false;

		if (desc.probeSpacing <= 0.f)
		{
			LOG_ERROR("Global Illumination", "DDGIVolume::create(): probeSpacing must be > 0");
			return false;
		}

		m_desc = desc;

		m_probeCountX = static_cast<u32>( ( desc.extents.x * 2.f ) / desc.probeSpacing ) + 1;
		m_probeCountY = static_cast<u32>( ( desc.extents.y * 2.f ) / desc.probeSpacing ) + 1;
		m_probeCountZ = static_cast<u32>( ( desc.extents.z * 2.f ) / desc.probeSpacing ) + 1;

		if (probeCount() == 0)
		{
			LOG_ERROR("Global Illumination", "DDGIVolume::create(): No probes found. Check extent size and probe spacing");
			return false;
		}

		const u32 irrWidth = m_probeCountX * m_probeCountY * kIrradianceTileTexels;
		const u32 irrHeight = m_probeCountZ * kIrradianceTileTexels;

		TextureDesc irrandianceDesc{};
		irrandianceDesc.width = irrWidth;
		irrandianceDesc.height = irrHeight;
		irrandianceDesc.format = TextureFormat::RG16Float;
		irrandianceDesc.usage = TextureUsage::Storage | TextureUsage::Sampled;
		irrandianceDesc.debugName = "DDGI Irradiance Atlas";

		m_irradianceAtlas = device.createTexture(irrandianceDesc);
		if (!m_irradianceAtlas)
		{
			LOG_ERROR("Global Illumination", "DDGIVolume::create(): irradiance atlas allocation failed ({}x{})", 
				irrWidth, irrHeight);
			return false;
		}

		const u32 depthWidth = m_probeCountX * m_probeCountY * kDepthTileTexels;
		const u32 depthHeight = m_probeCountZ * kDepthTileTexels;

		TextureDesc depthDesc{};
		depthDesc.width = depthWidth;
		depthDesc.height = depthHeight;
		depthDesc.format = TextureFormat::RG16Float;
		depthDesc.usage = TextureUsage::Storage | TextureUsage::Sampled;
		depthDesc.debugName = "DDGI Depth Atlas";

		m_depthAtlas = device.createTexture(depthDesc);
		if (!m_depthAtlas)
		{
			LOG_ERROR("Global Illumination", "DDGIVolume::create(): depth atlas allocation failed ({}x{})",
				depthWidth, depthHeight);
			return false;
		}

		LOG_INFO("Global Illumination", "DDGIVolume created: {}x{}x{} probes, ({} total). Irradiance Atlas: {}x{}. Depth Atlas: {}x{}",
			m_probeCountX, m_probeCountY, m_probeCountZ, probeCount(), irrWidth, irrHeight, depthWidth, depthHeight);

		return true;
	}

	math::Vec3f DDGIVolume::probePosition(u32 x, u32 y, u32 z) const
	{
		const math::Vec3f minCorner = m_desc.origin - m_desc.extents;
		return minCorner + math::Vec3f(
			static_cast<float>( x ) * m_desc.probeSpacing,
			static_cast<float>( y ) * m_desc.probeSpacing,
			static_cast<float>( y ) * m_desc.probeSpacing);
	}
}
