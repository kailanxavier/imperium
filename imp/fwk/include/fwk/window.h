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

		bool create(const WindowDesc& desc);
		void destroy();
		void pollEvents();

		bool shouldClose() const { return m_shouldClose; }
		u32 width() const { return m_width; }
		u32 height() const { return m_height; }
		bool isMinimised() const { return m_minimised; }

		using ResizeCallback = std::function<void(u32 width, u32 height, bool minimised)>;
		void setResizeCallback(ResizeCallback cb) { m_onResize = std::move(cb); }

		GLFWwindow* getNativeHandle() const { return m_handle; }

	private:
		static void framebufferSizeCallback(GLFWwindow* w, int width, int height);
		static void windowCloseCallback(GLFWwindow* w);
		void handleResize(u32 width, u32 height);

		GLFWwindow* m_handle = nullptr;
		u32 m_width = 0;
		u32 m_height = 0;
		bool m_shouldClose = false;
		bool m_minimised = false;
		ResizeCallback m_onResize;
		static int s_glfwRefCount;
	};
}
