#include <app/application.h>
#include <backends/imgui_impl_glfw.h>

#include <gfx/gizmo_renderer.h>

#include <chrono>

namespace imp::app
{
	Application::~Application()
	{
		if (m_initialised)
			shutdown();
	}

	bool Application::initialise(const ApplicationDesc& desc, std::unique_ptr<IApp> app)
	{
		if (m_initialised)
			return false;

		for (const auto& mount : desc.vfsMounts)
		{
			if (!m_vfs.mount(mount.virtualPrefix, mount.physicalPath,
				mount.priority, mount.writable, mount.createIfMissing))
			{
				LOG_ERROR("Application", "Failed to mount {} -> {}", mount.virtualPrefix, mount.physicalPath);
				return false;
			}
		}

		if (!m_window.create(desc.window))
		{
			LOG_ERROR("Application", "Failed to create window");
			return false;
		}

		auto tryInitDevice = [&](gfx::GraphicsApi api) -> std::unique_ptr<gfx::IDevice>
			{
				std::unique_ptr<gfx::IDevice> candidate = gfx::createDevice(api);
				if (!candidate)
					return nullptr;

				gfx::DeviceDesc deviceDesc;
				deviceDesc.window = &m_window;
				deviceDesc.appName = desc.window.title.c_str();
				deviceDesc.enableValidation = desc.enableValidation;
				deviceDesc.vsync = desc.vsync;
				deviceDesc.vfs = &m_vfs;
				deviceDesc.allocator = &m_gfxAllocator;

				if (!candidate->initialise(deviceDesc))
					return nullptr;

				return candidate;
			};

		if (desc.autoSelectGraphicsApi)
		{
			for (gfx::GraphicsApi api : gfx::availableApis())
			{
				m_device = tryInitDevice(api);
				if (m_device)
					break;

				LOG_WARN("Application", "{} did not initialise, trying next available API", gfx::toString(api));
			}
		}
		else
		{
			m_device = tryInitDevice(desc.graphicsApi);
		}

		if (!m_device)
		{
			LOG_FATAL("Application", "No available graphics API could be initialised");
			m_device.reset();
			m_window.destroy();
			return false;
		}

		m_jobs.initialise(desc.jobWorkerCount);
		m_app = std::move(app);

		m_ctx = std::make_unique<AppContext>(AppContext{
			m_window, *m_device, m_window.input(), m_layers, m_vfs,
			m_world, m_jobs, m_gfxAllocator});

		if (!m_app->onInit(*m_ctx))
		{
			m_jobs.shutdown();
			m_device->shutdown();
			m_device.reset();
			m_window.destroy();
			m_app.reset();
			m_ctx.reset();
			return false;
		}

		ImGui::CreateContext();
		ImGui::StyleColorsClassic();

		ImGui_ImplGlfw_InitForVulkan(m_window.getNativeHandle(), true);
		m_device->initImGui(); 

		m_initialised = true;
		return true;
	}

	void Application::run()
	{
		if (!m_initialised)
			return;

		m_lastFrameTime = std::chrono::steady_clock::now();

		while (!m_window.shouldClose())
			mainLoopFrame();
	}

	void Application::shutdown()
	{
		if (!m_initialised)
			return;
		
		ImGui_ImplGlfw_Shutdown();
		m_device->shutdownImGui();
		ImGui::DestroyContext();

		if (m_app)
			m_app->onShutdown(*m_ctx);

		m_jobs.shutdown();

		imp::gfx::GizmoRenderer::instance().shutdown();

		if (m_device)
		{
			m_device->shutdown();
			m_device.reset();
		}

		m_window.destroy();

		m_app.reset();
		m_ctx.reset();

		m_initialised = false;
	}

	void Application::mainLoopFrame()
	{
		m_window.input().newFrame();
		m_window.pollEvents();

		if (m_window.isMinimised())
			return;

		const auto now = std::chrono::steady_clock::now();
		const float deltaSeconds = std::chrono::duration<float>(now - m_lastFrameTime).count();
		m_lastFrameTime = now;

		ImGui_ImplGlfw_NewFrame();
		m_device->newImGuiFrame();
		ImGui::NewFrame(); 

		m_app->onUpdate(*m_ctx, deltaSeconds);
		m_layers.updateAll(deltaSeconds);

		gfx::ICommandList* cmd = m_device->beginFrame();
		if (cmd)
		{
			m_app->onRender(*m_ctx, *cmd);
			//m_layers.renderAll(*cmd);

			ImGui::Render();
			m_device->renderImGui(*cmd);

			m_device->endFrame();
		}
	}
}
