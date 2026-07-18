#include <core/core.h>
#include <core/log/log.h>
#include <core/memory/heap_allocator.h>
#include <fwk/window.h>
#include <fwk/gfx_device.h>
#include <gfx/gfx.h>

#include <protocol/tool_server.h>
#include <protocol/memory_telemetry.h>

int main()
{
	imp::log::Logger::get().initialise();
	imp::protocol::ToolServer::instance().start(47810);

	LOG_INFO("Sandbox", "App starting...");
	{
	imp::memory::HeapAllocator gfxHostAllocator("GfxHost");

	imp::fwk::Window window;
	imp::fwk::WindowDesc windowDesc{};
	windowDesc.title = "Titus' sandbox";
	windowDesc.width = 1280;
	windowDesc.height = 720;

	if (!window.create(windowDesc))
	{
		LOG_ERROR("Sandbox", "Failed to create window");
		return 1;
	}

	std::unique_ptr<imp::fwk::IGfxDevice> gfx = imp::gfx::createDevice();

	imp::fwk::GfxDeviceDesc gfxDesc;
	gfxDesc.window = &window;
	gfxDesc.appName = "Sandbox";
	gfxDesc.allocator = &gfxHostAllocator;
#ifndef NDEBUG
	gfxDesc.enableValidation = true;
#endif

	if (!gfx->initialise(gfxDesc))
	{
		LOG_FATAL("Sandbox", "Failed to initialise {} device", gfx->getApiName());
		window.destroy();
		return 1;
	}

	window.attachGfxDevice(gfx.get());

	LOG_INFO("Sandbox", "Running with {} device, window ({}, {})",
		gfx->getApiName(), window.width(), window.height());

	auto lastTelemetryPublish = std::chrono::steady_clock::now();
	constexpr auto kTelemetryInterval = std::chrono::milliseconds(200);

	while (!window.shouldClose())
	{
		window.pollEvents();

		if (window.isMinimised())
			continue; // skip rendering while minimised

		const auto now = std::chrono::steady_clock::now();
		if (now - lastTelemetryPublish >= kTelemetryInterval &&
			imp::protocol::ToolServer::instance().hasSubscribers(imp::protocol::MessageType::MemoryTelemetry))
		{
			const auto snap = gfxHostAllocator.statsSnapshot();

			imp::protocol::AllocatorStatsPayload payload;
			payload.name            = std::string(gfxHostAllocator.name());
			payload.totalAllocated  = snap.totalAllocated;
			payload.totalFreed      = snap.totalFreed;
			payload.currentUsed     = snap.currentUsed;
			payload.peakUsed        = snap.peakUsed;
			payload.allocationCount = snap.allocationCount;
			payload.freeCount       = snap.freeCount;
			payload.tagBytes.assign(std::begin(snap.tagBytes), std::end(snap.tagBytes));

			const auto bytes = imp::protocol::serialiseMemoryTelemetry({payload});
			imp::protocol::ToolServer::instance().publish(imp::protocol::MessageType::MemoryTelemetry, bytes);

			lastTelemetryPublish = now;
		}

		gfx->beginFrame();

		// render calls go here

		gfx->endFrame();
	}

	window.detachGfxDevice();
	gfx->shutdown();
	window.destroy();
	}

	imp::protocol::ToolServer::instance().stop();
	imp::log::Logger::get().shutdown();
	return 0;
}
