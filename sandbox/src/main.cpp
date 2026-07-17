#include <core/core.h>
#include <core/log/log.h>
#include <core/memory/heap_allocator.h>
#include <fwk/window.h>
#include <fwk/gfx_device.h>
#include <gfx/gfx.h>
#include <iostream>

int main()
{
	imp::log::Logger::get().initialise();
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

	while (!window.shouldClose())
	{
		window.pollEvents();

		if (window.isMinimised())
			continue; // skip rendering while minimised

		gfx->beginFrame();

		// render calls go here

		gfx->endFrame();
	}

	window.detachGfxDevice();
	gfx->shutdown();
	window.destroy();
	}

	imp::log::Logger::get().shutdown();
	return 0;
}
