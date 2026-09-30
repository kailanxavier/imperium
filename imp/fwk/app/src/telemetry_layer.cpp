#include <app/telemetry_layer.h>
#include <core/log/log.h>

#include <protocol/tool_server.h>
#include <protocol/memory_telemetry.h>

namespace imp::app
{
	TelemetryLayer::TelemetryLayer(u16 toolServerPort, std::chrono::milliseconds publishInterval)
		: ILayer("Telemetry")
		, m_toolServerPort(toolServerPort)
		, m_publishInterval(publishInterval)
		, m_lastPublish(std::chrono::steady_clock::now())
	{
	}

	void TelemetryLayer::onAttach(AppContext& ctx)
	{
		m_gfxAllocator = &ctx.gfxAllocator;
	}

	void TelemetryLayer::onDetach(AppContext& /*ctx*/)
	{
		m_gfxAllocator = nullptr;
	}

	void TelemetryLayer::onUpdate(AppContext& /*ctx*/, float /*deltaSeconds*/)
	{
		if (!m_gfxAllocator)
			return;

		const auto now = std::chrono::steady_clock::now();

		if (now - m_lastPublish < m_publishInterval)
			return;

		if (!protocol::ToolServer::instance().hasSubscribers(protocol::MessageType::MemoryTelemetry))
			return;

		const auto snap = m_gfxAllocator->statsSnapshot();

		protocol::AllocatorStatsPayload payload;
		payload.name = std::string(m_gfxAllocator->name());
		payload.totalAllocated = snap.totalAllocated;
		payload.totalFreed = snap.totalFreed;
		payload.currentUsed = snap.currentUsed;
		payload.peakUsed = snap.peakUsed;
		payload.allocationCount = snap.allocationCount;
		payload.freeCount = snap.freeCount;
		payload.tagBytes.assign(std::begin(snap.tagBytes), std::end(snap.tagBytes));

		const auto bytes = protocol::serialiseMemoryTelemetry({payload});
		protocol::ToolServer::instance().publish(protocol::MessageType::MemoryTelemetry, bytes);

		m_lastPublish = now;
	}
}
