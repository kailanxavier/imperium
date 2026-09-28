#pragma once

#include <app/iapp.h>
#include <fwk/layer.h>
#include <core/memory/heap_allocator.h>
#include <core/types/int_types.h>
#include <chrono>

namespace imp::app
{
	class TelemetryLayer final : public fwk::ILayer
	{
	public:
		explicit TelemetryLayer(
			u16 toolServerPort = 47810,
			std::chrono::milliseconds publishInterval = std::chrono::milliseconds(200));

		void onAttach(AppContext& ctx) override;
		void onDetach(AppContext& ctx) override;
		void onUpdate(AppContext& ctx, float deltaSeconds) override;

	private:
		memory::HeapAllocator* m_gfxAllocator = nullptr;
		u16 m_toolServerPort;
		std::chrono::milliseconds m_publishInterval;
		std::chrono::steady_clock::time_point m_lastPublish;
	};
}
