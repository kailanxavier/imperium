#pragma once

#include <app/iapp.h>
#include <fwk/window.h>
#include <fwk/layer.h>
#include <gfx/device.h>
#include <core/fs/vfs.h>
#include <core/memory/heap_allocator.h>
#include <jobs/job_system.h>
#include <ecs/world.h>

namespace imp::app 
{
	struct VfsMountDesc
	{
		std::string virtualPrefix;
		std::string physicalPath;
		int priority = 0;
		bool writable = true;
		bool createIfMissing = false;
	};

	struct ApplicationDesc
	{
		fwk::WindowDesc window;
		std::vector<VfsMountDesc> vfsMounts;

		bool autoSelectGraphicsApi = true;
		gfx::GraphicsApi graphicsApi = gfx::GraphicsApi::Vulkan;

		bool enableValidation = false;
		bool vsync = true;
		u32 jobWorkerCount = 0;
	};

	class Application
	{
	public:
		Application() = default;
		~Application();

		Application(const Application&) = delete;
		Application operator=(const Application&) = delete;

		bool initialise(const ApplicationDesc& desc, std::unique_ptr<IApp> app);

		void run();
		void shutdown();

		[[nodiscard]] bool initialised() const noexcept { return m_initialised; }

		[[nodiscard]] fwk::LayerStack& layers() noexcept { return m_layers; }
		[[nodiscard]] jobs::JobSystem& jobs() noexcept { return m_jobs; }
		[[nodiscard]] ecs::World& world() noexcept { return m_world; }
		[[nodiscard]] memory::HeapAllocator& gfxAllocator() noexcept { return m_gfxAllocator; }
		[[nodiscard]] fwk::Window& window() noexcept { return m_window; }
		[[nodiscard]] gfx::IDevice& device() const noexcept { return *m_device; }
		[[nodiscard]] std::unique_ptr<IApp>& app() noexcept { return m_app; }

	private:
		void mainLoopFrame();

		fwk::Window m_window;
		std::unique_ptr<gfx::IDevice> m_device;
		fs::VirtualFileSystem m_vfs;
		fwk::LayerStack m_layers;
		jobs::JobSystem m_jobs;
		ecs::World m_world;
		memory::HeapAllocator m_gfxAllocator{ "GfxAllocator" };

		std::unique_ptr<IApp> m_app;
		std::unique_ptr<AppContext> m_ctx;

		std::chrono::steady_clock::time_point m_lastFrameTime;

		bool m_initialised = false;
	};
}
