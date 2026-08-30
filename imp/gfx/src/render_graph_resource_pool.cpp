#include "gfx/render_graph_resource_pool.h"
#include "gfx/device.h"
#include "gfx/config.h"

namespace imp::gfx
{
	namespace
	{
		u64 hashCombine(u64 seed, u64 value)
		{
			// same mix constant as boost::hash_combine.
			// won't claim i know why, but a reason must exist somewhere out there.
			return seed ^ ( value + 0x9e3779b97f4a7c15ULL + ( seed << 6 ) + ( seed >> 2 ) );
		}

		u64 hashTextureDesc(const TextureDesc& desc)
		{
			u64 h = 0;
			h = hashCombine(h, static_cast<u64>( desc.width ));
			h = hashCombine(h, static_cast<u64>( desc.height ));
			h = hashCombine(h, static_cast<u64>( desc.mipLevels ));
			h = hashCombine(h, static_cast<u64>( desc.arrayLayers ));
			h = hashCombine(h, static_cast<u64>( desc.format ));
			h = hashCombine(h, static_cast<u64>( desc.usage ));
			h = hashCombine(h, static_cast<u64>( desc.sampleCount ));
			return h;
		}

		u64 hashBufferDesc(const BufferDesc& desc)
		{
			u64 h = 0;
			h = hashCombine(h, desc.size);
			h = hashCombine(h, static_cast<u64>( desc.usage ));
			h = hashCombine(h, static_cast<u64>( desc.memoryAccess ));
			h = hashCombine(h, static_cast<u64>( desc.indexFormat ));
			return h;
		}
	}

	RenderGraphResourcePool::RenderGraphResourcePool(IDevice& device) : m_device(&device)
	{
	}

	IRenderTarget& RenderGraphResourcePool::acquireTexture(const TextureDesc& desc)
	{
		const u64 descHash = hashTextureDesc(desc);

		for (auto& entry : m_textures)
		{
			if (entry.inUse || entry.descHash != descHash)
				continue;

			if (entry.everFreed && ( m_frameCounter - entry.freedFrame ) < kMaxFramesInFlight)
				continue;

			entry.inUse = true;
			return *entry.target;
		}

		PooledTexture entry{};
		entry.target = m_device->createRenderTarget(desc);
		entry.descHash = descHash;
		entry.inUse = true;

		m_textures.push_back(std::move(entry));
		return *m_textures.back().target;
	}

	IBuffer& RenderGraphResourcePool::acquireBuffer(const BufferDesc& desc)
	{
		const u64 descHash = hashBufferDesc(desc);

		for (auto& entry : m_buffers)
		{
			if (entry.inUse || entry.descHash != descHash)
				continue;

			if (entry.everFreed && ( m_frameCounter - entry.freedFrame ) < kMaxFramesInFlight)
				continue;

			entry.inUse = true;
			return *entry.buffer;
		}

		PooledBuffer entry{};
		entry.buffer = m_device->createBuffer(desc);
		entry.descHash = descHash;
		entry.inUse = true;

		m_buffers.push_back(std::move(entry));
		return *m_buffers.back().buffer;
	}

	void RenderGraphResourcePool::releaseAll(u32 frameIndex)
	{
		m_lastDeviceFrame = frameIndex;

		for (auto& entry : m_textures)
		{
			if (!entry.inUse)
				continue;

			entry.inUse = false;
			entry.everFreed = true;
			entry.freedFrame = m_frameCounter;
		}

		for (auto& entry : m_buffers)
		{
			if (!entry.inUse)
				continue;

			entry.inUse = false;
			entry.everFreed = true;
			entry.freedFrame = m_frameCounter;
		}

		++m_frameCounter;
	}
}
