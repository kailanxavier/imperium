#include <app/application.h>
#include <app/launch_options.h>
#include <app/telemetry_layer.h>
#include <app/gizmo_layer.h>
#include <app/editor_bridge_layer.h>
#include <app/sky_layer.h>

#include <game/game_app.h>
#include <game/light_control_layer.h>

#include <core/log/log.h>
#include <core/platform/exe_path.h>

#include <filesystem>
#include <memory>

using namespace imp;

namespace
{
	std::filesystem::path resolveScenesDir()
	{
#if !defined(NDEBUG) && defined(IMP_SCENE_SOURCE_DIR)
		const std::filesystem::path sourceDir = IMP_SCENE_SOURCE_DIR;
		if (std::filesystem::is_directory(sourceDir))
			return sourceDir;

		LOG_WARN("Game", "Scene source dir '{}' not found.", sourceDir.string());
#endif
		return platform::executableDir() / "scenes";
	}

}

int main(int argc, char** argv)
{
	log::Logger::get().initialise();
	LOG_INFO("Game", "App starting...");

	app::LaunchOptions launch = app::parseLaunchOptions(argc, argv);

	app::ApplicationDesc desc{};
#ifndef NDEBUG
	desc.window.title = "Atlas";
#else
	desc.window.title = "Velvet";
#endif
	desc.window.width = 1280;
	desc.window.height = 720;
	desc.window.fullscreen = false;
	desc.enableValidation = true;
	desc.vsync = true;
	launch.applyTo(desc);

	app::EditorHostDesc editorHost{};
	editorHost.launchEditor = launch.editor;
	editorHost.embedViewport = launch.editor && !launch.useOwnWindow;
#ifdef IMP_EDITOR_PATH
	editorHost.editorPathHint = IMP_EDITOR_PATH;
#endif
	desc.window.startVisible = !editorHost.embedViewport;

	const auto shadersPath = ( platform::executableDir() / "assets" ).string();
	desc.vfsMounts.push_back(app::VfsMountDesc{ "assets/", shadersPath, 0, true, true });

	const auto scenesPath = resolveScenesDir().string();
	desc.vfsMounts.push_back(app::VfsMountDesc{ "scenes/", scenesPath, 0, true, true });

	{
		app::Application application;
		application.services().provide(launch);

		application.layers().pushOverlay(std::make_unique<app::SkyLayer>());
		application.layers().pushOverlay(std::make_unique<app::EditorBridgeLayer>(editorHost));
		application.layers().pushOverlay(std::make_unique<app::TelemetryLayer>());
		application.layers().pushOverlay(std::make_unique<game::LightControlLayer>());

		if (!application.initialise(desc, std::make_unique<game::GameApp>()))
		{
			LOG_FATAL("Game", "Application failed to initialise");
			log::Logger::get().shutdown();
			return 1;
		}

		application.layers().pushOverlay(std::make_unique<app::GizmoLayer>());

		LOG_INFO("Game", "Running with {} device, window ({}, {})",
			application.device().apiName(), application.window().width(), application.window().height());

		application.run();
		application.shutdown();
	}

	imp::log::Logger::get().shutdown();
	return 0;
}
