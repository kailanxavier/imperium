#include "fwk/window.h"
#include "fwk/gfx_device.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdio>
#include <core/log/log.h>

namespace imp::fwk
{
	int Window::s_glfwRefCount = 0;

	Window::~Window()
	{
		destroy();
	}


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
		glfwWindowHint(GLFW_VISIBLE, desc.startVisible ? GLFW_TRUE : GLFW_FALSE);

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

		int fbWidth = 0, fbHeight = 0;
		glfwGetFramebufferSize(m_handle, &fbWidth, &fbHeight);
		m_width = static_cast<u32>( fbWidth );
		m_height = static_cast<u32>( fbHeight );
		m_minimised = ( m_width == 0 || m_height == 0 );

		glfwSetWindowUserPointer(m_handle, this);
		glfwSetFramebufferSizeCallback(m_handle, &Window::glfwFramebufferSizeCallback);
		glfwSetWindowCloseCallback(m_handle, &Window::glfwWindowCloseCallback);

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

	void Window::glfwFramebufferSizeCallback(GLFWwindow* w, int width, int height)
	{
		auto* self = static_cast<Window*>( glfwGetWindowUserPointer(w) );
		if (self)
			self->handleResize(static_cast<u32>( width ), static_cast<u32>( height ));
	}

	void Window::glfwWindowCloseCallback(GLFWwindow* w)
	{
		auto* self = static_cast<Window*>( glfwGetWindowUserPointer(w) );
		if (self)
			self->requestClose();
	}

	void Window::handleResize(u32 width, u32 height)
	{
		m_width = width;
		m_height = height;

		bool nowMinimised = ( width == 0 || height == 0 );
		if (nowMinimised != m_minimised)
		{
			m_minimised = nowMinimised;
			if (m_gfxDevice) m_gfxDevice->onMinimiseChanged(m_minimised);
		}

		// Forward to the attached gfx device first
		// (e.g. to recreate a swapchain) before notifying app-level code,
		// so app code observing the resize can assume the device is already caught up.
		if (m_gfxDevice && !m_minimised)
			m_gfxDevice->onResize(m_width, m_height);

		if (m_onResize) m_onResize(m_width, m_height);
	}

}