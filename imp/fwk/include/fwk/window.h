#pragma once

#include <fwk/input.h>

#include <core/memory/int_types.h>
#include <functional>
#include <string>

struct GLFWwindow;

namespace imp::fwk
{
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

		[[nodiscard]] bool shouldClose() const { return m_shouldClose; }
		[[nodiscard]] u32 width() const { return m_width; }
		[[nodiscard]] u32 height() const { return m_height; }
		[[nodiscard]] bool isMinimised() const { return m_minimised; }

		[[nodiscard]] Input& input() { return m_input; }
		[[nodiscard]] const Input& input() const { return m_input; }

		using ResizeCallback = std::function<void(u32 width, u32 height, bool minimised)>;
		void setResizeCallback(ResizeCallback cb) { m_onResize = std::move(cb); }

		[[nodiscard]] GLFWwindow* getNativeHandle() const { return m_handle; }

	private:
		static void framebufferSizeCallback(GLFWwindow* w, int width, int height);
		static void windowCloseCallback(GLFWwindow* w);
		void handleResize(u32 width, u32 height);

		static void keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods);
		static void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods);
		static void cursorPosCallback(GLFWwindow* w, double x, double y);
		static void scrollCallback(GLFWwindow* w, double xoffset, double yoffset);

		GLFWwindow* m_handle = nullptr;
		u32 m_width = 0;
		u32 m_height = 0;
		bool m_shouldClose = false;
		bool m_minimised = false;
		ResizeCallback m_onResize;
		static int s_glfwRefCount;

		Input m_input;
	};
}
