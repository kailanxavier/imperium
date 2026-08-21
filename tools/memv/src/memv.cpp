#include "memv/memv_app.h"
#include "memory_generated.h"

#include <fwk/window.h>
#include <gfx/gfx.h>
#include <core/log/log.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#include "core/memory/allocator_types.h"
#include <app/application.h>

using namespace imp;

int main(int argc, char** argv)
{
	using namespace imp::tools::memv;
	log::Logger::get().initialise();

	app::ApplicationDesc desc{};
	desc.window.title = "Memory Viewer";
	desc.window.width = 800;
	desc.window.height = 800;
	desc.enableValidation = true;
	desc.vsync = true;

	app::Application application;
	if (!application.initialise(desc, std::make_unique<MemoryViewerApp>()))
	{
		LOG_ERROR("Memory Viwer", "Application failed to initialise");
		log::Logger::get().shutdown();
		return 1;
	}

	application.run();
	application.shutdown();

	log::Logger::get().shutdown();
	return 0;
}
