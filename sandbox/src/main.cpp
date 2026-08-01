#include <app/application.h>
#include <app/telemetry_layer.h>
#include <app/light_control_layer.h>

#include <sandbox/sandbox_app.h>

#include <core/log/log.h>
#include <core/platform/exe_path.h>

#include <memory>

using namespace imp;

int main()
{
	log::Logger::get().initialise();
	LOG_INFO("Sandbox", "App starting...");

	app::ApplicationDesc desc{};
#ifndef NDEBUG
	desc.window.title = "Atlas";
#else
	desc.window.title = "Velvet";
#endif
	desc.window.width = 1280;
	desc.window.height = 720;

	const auto shadersPath = ( platform::executableDir() / "assets").string();
	desc.vfsMounts.push_back(app::VfsMountDesc{ "assets/", shadersPath, 0, true, true });

	app::Application application;
	if (!application.initialise(desc, std::make_unique<app::SandboxApp>()))
	{
		LOG_FATAL("Sandbox", "Application failed to initialise");
		log::Logger::get().shutdown();
		return 1;
	}

	LOG_INFO("Sandbox", "Running with {} device, window ({}, {})", 
		application.device().apiName(), application.window().width(), application.window().height());

	auto* sb = static_cast<app::SandboxApp*>(application.app().get());
	application.layers().pushOverlay(std::make_unique<app::TelemetryLayer>(application.gfxAllocator()));
	application.layers().pushOverlay(std::make_unique<app::LightControlLayer>(sb->sunDirection(), sb->pointPos()));

	application.run();
	application.shutdown();
	
	imp::log::Logger::get().shutdown();
	return 0;
}
