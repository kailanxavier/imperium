#include "fwk/window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <core/log/log.h>

namespace imp::fwk
{
	int Window::s_glfwRefCount = 0;

	Window::~Window() { destroy(); }

	bool Window::create(const WindowDesc& desc)
	{
		if (m_handle)
		{
			LOG_ERROR("Window", "Create called on an already created window");
			return false;
		}

		if (s_glfwRefCount == 0)
		{
			if (!glfwInit())
			{
				LOG_ERROR("Window", "glfwInit() failed");
				return false;
			}
		}
		++s_glfwRefCount;

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);

		m_handle = glfwCreateWindow(
			static_cast<int>( desc.width ), 
			static_cast<int>( desc.height ), 
			desc.title.c_str(), 
			nullptr, nullptr);

		if (!m_handle)
		{
			LOG_ERROR("Window", "glfwCreateWindow failed");
			if (--s_glfwRefCount == 0) glfwTerminate();
			return false;
		}

		int fbW = 0, fbH = 0;
		glfwGetFramebufferSize(m_handle, &fbW, &fbH);
		m_width = static_cast<u32>( fbW );
		m_height = static_cast<u32>( fbH );
		m_minimised = ( m_width == 0 || m_height == 0 );

		glfwSetWindowUserPointer(m_handle, this);
		glfwSetFramebufferSizeCallback(m_handle, &Window::framebufferSizeCallback);
		glfwSetWindowCloseCallback(m_handle, &Window::windowCloseCallback);
		glfwSetKeyCallback(m_handle, &Window::keyCallback);
		glfwSetMouseButtonCallback(m_handle, &Window::mouseButtonCallback);
		glfwSetCursorPosCallback(m_handle, &Window::cursorPosCallback);
		glfwSetScrollCallback(m_handle, &Window::scrollCallback);

		// I know this should be its own function like all others but this is temporary
		// and I don't really like this solution. For now, it will stay here. But if we
		// think of something better, please replace it.
		glfwSetWindowContentScaleCallback(m_handle, [](GLFWwindow* w, float xscale, float yscale)
		{
			auto* input = static_cast<Input*>(glfwGetWindowUserPointer(w));
			input->setMouseScale(xscale, yscale);
		});

		float xScale, yScale;
		glfwGetWindowContentScale(m_handle, &xScale, &yScale);

		m_input.setMouseScale(xScale, yScale);

		return true;
	}

	void Window::destroy()
	{
		if (!m_handle) return;

		glfwDestroyWindow(m_handle);
		m_handle = nullptr;

		if (--s_glfwRefCount == 0)
			glfwTerminate();
	}

	void Window::pollEvents()
	{
		glfwPollEvents();
	}

	void Window::framebufferSizeCallback(GLFWwindow* w, int width, int height)
	{
		auto* self = static_cast<Window*>( glfwGetWindowUserPointer(w) );
		if (self)
			self->handleResize(static_cast<u32>( width ), static_cast<u32>( height ));
	}

	void Window::windowCloseCallback(GLFWwindow* w)
	{
		auto* self = static_cast<Window*>( glfwGetWindowUserPointer(w) );
		if (self)
			self->m_shouldClose = true;
	}

	void Window::handleResize(u32 width, u32 height)
	{
		m_width = width;
		m_height = height;
		m_minimised = ( width == 0 || height == 0 );
		
		if (m_onResize)
			m_onResize(m_width, m_height, m_minimised);
	}

	void Window::keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods)
	{
		auto* self = static_cast<Window*>( glfwGetWindowUserPointer(w) );
		if (self)
			self->m_input.onKeyEvent(key, action);
	}

	void Window::mouseButtonCallback(GLFWwindow* w, int button, int action, int mods)
	{
		auto* self = static_cast<Window*>( glfwGetWindowUserPointer(w) );
		if (self)
			self->m_input.onMouseButtonEvent(button, action);
	}

	void Window::cursorPosCallback(GLFWwindow* w, double x, double y)
	{
		auto* self = static_cast<Window*>( glfwGetWindowUserPointer(w) );
		if (self)
			self->m_input.onCursorPosEvent(x, y);
	}

	void Window::scrollCallback(GLFWwindow* w, double xoffset, double yoffset)
	{
		auto* self = static_cast<Window*>( glfwGetWindowUserPointer(w) );
		if (self)
			self->m_input.onScrollEvent(xoffset, yoffset);
	}
}
