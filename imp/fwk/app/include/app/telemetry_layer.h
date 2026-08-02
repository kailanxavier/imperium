#pragma once

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
			memory::HeapAllocator& gfxAllocator,
			u16 toolServerPort = 47810,
			std::chrono::milliseconds publishInterval = std::chrono::milliseconds(200));

		void onAttach() override;
		void onDetach() override;
		void onUpdate(float deltaSeconds) override;

	private:
		memory::HeapAllocator& m_gfxAllocator;
		u16 m_toolServerPort;
		std::chrono::milliseconds m_publishInterval;
		std::chrono::steady_clock::time_point m_lastPublish;
	};
}
