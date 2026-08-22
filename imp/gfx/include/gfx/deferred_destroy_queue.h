#pragma once
#include <core/types/int_types.h>
#include <functional>
#include <vector>
#include <memory>

namespace imp::gfx
{
	class DeferredDestroyQueue
	{
	public:
		void init(u32 framesInFlight) { m_pending.assign(framesInFlight, {}); }
		void retire(std::function<void()> deleter)
		{
			m_pending[m_currentFrame].push_back(std::move(deleter));
		}

		void flushFrame(u32 frameIndex)
		{
			auto& slot = m_pending[frameIndex];
			for (auto& deleter : slot) 
				deleter();
			slot.clear();
			m_currentFrame = frameIndex;
		}

		void flushAll()
		{
			for (auto& slot : m_pending)
			{
				for (auto& deleter : slot)
					deleter();
				slot.clear();
			}
		}

	private:
		std::vector<std::vector<std::function<void()>>> m_pending;
		u32 m_currentFrame = 0;
	};

	template <typename T>
	void deferDestroy(class IDevice& device, std::unique_ptr<T> resource)
	{
		if (!resource)
			return;

		device.deferredDestroy([r = std::move(resource)]() mutable { r.reset(); });
	}
	template <typename T>
	void deferDestroy(class IDevice& device, std::shared_ptr<T> resource)
	{
		if (!resource)
			return;

		device.deferredDestroy([r = std::move(resource)]() mutable { r.reset(); });
	}
}
