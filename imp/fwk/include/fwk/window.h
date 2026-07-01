#pragma once

#include <core/memory/int_types.h>
#include <functional>
#include <string>

struct GLFWwindow;

namespace imp::fwk
{
	class IGfxDevice;

	struct WindowDesc
	{
		std::string title = "Untitled";
		u32 width = 1280;
		u32 height = 720;
		bool resizable = true;
		bool startVisible = true;
	};

	class Window
	{
	public:
		Window() = default;
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;
		Window(Window&&) = delete;
		Window& operator=(Window&&) = delete;

		bool create(const WindowDesc& desc);
		void destroy();

		void pollEvents();

		bool shouldClose() const { return m_shouldClose; }
		void requestClose() { m_shouldClose = true; }

		u32 width() const { return m_width; }
		u32 height() const { return m_height; }
		bool isMinimised() const { return m_minimised; }

		void attachGfxDevice(IGfxDevice* device) { m_gfxDevice = device; }
		void detachGfxDevice() { m_gfxDevice = nullptr; }
		IGfxDevice* getAttachedDevice() const { return m_gfxDevice; }

		using ResizeCallback = std::function<void(u32 width, u32 height)>;
		void setResizeCallback(ResizeCallback cb) { m_onResize = std::move(cb); }

		GLFWwindow* getNativeHandle() const { return m_handle; }

	private:
		static void glfwFramebufferSizeCallback(GLFWwindow* w, int width, int height);
		static void glfwWindowCloseCallback(GLFWwindow* w);

		void handleResize(u32 width, u32 height);

		GLFWwindow* m_handle = nullptr;
		u32 m_width = 0;
		u32 m_height = 0;
		bool m_shouldClose = false;
		bool m_minimised = false;

		IGfxDevice* m_gfxDevice = nullptr;
		ResizeCallback m_onResize;
		
		static int s_glfwRefCount;
	};
}
