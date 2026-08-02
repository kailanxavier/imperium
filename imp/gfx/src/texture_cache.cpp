#include <gfx/texture_cache.h>
#include <gfx/gfx.h>

namespace imp::gfx
{
	std::shared_ptr<ITexture> imp::gfx::TextureCache::find(const std::string& cacheKey)
	{
		std::lock_guard<std::mutex> lock(m_pathMutex);
		auto it = m_pathTextures.find(cacheKey);
		return it != m_pathTextures.end() ? it->second : nullptr;
	}

	std::shared_ptr<ITexture> TextureCache::insert(const std::string & cacheKey, std::shared_ptr<ITexture> texture)
	{
		std::lock_guard<std::mutex> lock(m_pathMutex);
		auto [it, inserted] = m_pathTextures.try_emplace(cacheKey, std::move(texture));
		return it->second;
	}

	std::shared_ptr<ITexture> imp::gfx::TextureCache::fallbackAlbedo()
	{
		return getOrCreateFallback(m_fallbackAlbedo, 255, 255, 255, 255);
	}

	std::shared_ptr<ITexture> TextureCache::fallbackMetallicRoughness()
	{
		return getOrCreateFallback(m_fallbackMetallicRoughness, 0, 255, 0, 255);
	}

	std::shared_ptr<ITexture> TextureCache::fallbackNormal()
	{
		return getOrCreateFallback(m_fallbackNormal, 128, 128, 255, 255);
	}

	std::shared_ptr<ITexture> TextureCache::fallbackOcclusion()
	{
		return getOrCreateFallback(m_fallbackOcclusion, 255, 255, 255, 255);
	}

	std::shared_ptr<ITexture> TextureCache::getOrCreateFallback(std::shared_ptr<ITexture>& slot, u8 r, u8 g, u8 b, u8 a)
	{
		std::lock_guard<std::mutex> lock(m_fallbackMutex);
		if (slot)
			return slot;

		const u8 pixel[4] = { r, g, b, a };

		TextureDesc desc;
		desc.width = 1;
		desc.height = 1;
		desc.format = TextureFormat::RGBA8Unorm;
		desc.usage = TextureUsage::Sampled;
		desc.initialData = pixel;

		// unique_ptr to shared_ptr.
		// ITexture has a real destructor path so no problem
		slot = m_device.createTexture(desc);
		return slot;
	}
}
