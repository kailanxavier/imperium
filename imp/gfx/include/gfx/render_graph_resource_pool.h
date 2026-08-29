#pragma once

#include <core/types/int_types.h>
#include "gfx/resources.h"

#include <memory>
#include <vector>

namespace imp::gfx
{
	class IDevice;

	class RenderGraphResourcePool
	{
	public:
		RenderGraphResourcePool(IDevice& device);

		IRenderTarget& acquireTexture(const TextureDesc& desc);
		IBuffer& acquireBuffer(const BufferDesc& desc);

		void releaseAll(u32 frameIndex);

	private:
		struct PooledTexture
		{
			std::unique_ptr<IRenderTarget> target;
			u64 descHash = 0;
			bool inUse = false;
			bool everFreed = false;
			u32 freedFrame = 0;
		};

		struct PooledBuffer
		{
			std::unique_ptr<IBuffer> buffer;
			u64 descHash = 0;
			bool inUse = false;
			bool everFreed = false;
			u32 freedFrame = 0;
		};

		IDevice* m_device = nullptr;
		std::vector<PooledTexture> m_textures;
		std::vector<PooledBuffer> m_buffers;

		u32 m_frameCounter = 0;
		u32 m_lastDeviceFrame = 0;
	};
}
