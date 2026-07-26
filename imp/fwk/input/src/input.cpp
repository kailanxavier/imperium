#include <input/input.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace imp::fwk
{
	namespace
	{
		Key fromGlfwKey(int glfwKey)
		{
			switch (glfwKey)
			{
			case GLFW_KEY_Q: return Key::Q;
			case GLFW_KEY_W: return Key::W;
			case GLFW_KEY_E: return Key::E;
			case GLFW_KEY_R: return Key::R;
			case GLFW_KEY_T: return Key::T;
			case GLFW_KEY_Y: return Key::Y;
			case GLFW_KEY_U: return Key::U;
			case GLFW_KEY_I: return Key::I;
			case GLFW_KEY_O: return Key::O;
			case GLFW_KEY_P: return Key::P;
			case GLFW_KEY_A: return Key::A;
			case GLFW_KEY_S: return Key::S;
			case GLFW_KEY_D: return Key::D;
			case GLFW_KEY_F: return Key::F;
			case GLFW_KEY_G: return Key::G;
			case GLFW_KEY_H: return Key::H;
			case GLFW_KEY_J: return Key::J;
			case GLFW_KEY_K: return Key::K;
			case GLFW_KEY_L: return Key::L;
			case GLFW_KEY_Z: return Key::Z;
			case GLFW_KEY_X: return Key::X;
			case GLFW_KEY_C: return Key::C;
			case GLFW_KEY_V: return Key::V;
			case GLFW_KEY_B: return Key::B;
			case GLFW_KEY_N: return Key::N;
			case GLFW_KEY_M: return Key::M;
			case GLFW_KEY_SPACE: return Key::Space;
			case GLFW_KEY_LEFT_SHIFT: return Key::LeftShift;
			case GLFW_KEY_ESCAPE: return Key::Escape;
			case GLFW_KEY_LEFT_CONTROL: return Key::LeftControl;
			default: return Key::Invalid;
			}
		}

		MouseButton fromGlfwMouseButton(int glfwButton)
		{
			switch (glfwButton)
			{
			case GLFW_MOUSE_BUTTON_LEFT: return MouseButton::Left;
			case GLFW_MOUSE_BUTTON_RIGHT: return MouseButton::Right;
			case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
			case GLFW_MOUSE_BUTTON_4: return MouseButton::Four;
			case GLFW_MOUSE_BUTTON_5: return MouseButton::Five;
			default: return MouseButton::Invalid;
			}
		}

		constexpr size_t toIndex(Key key)
		{
			return static_cast<size_t>( key );
		}

		constexpr size_t toIndex(MouseButton button)
		{
			return static_cast<size_t>( button );
		}
	}

	void Input::newFrame()
	{
		m_keysPressed.fill(false);
		m_keysReleased.fill(false);
		m_mouseDelta = math::Vec2f::zero();
		m_scrollDelta = 0.f;
	}

	bool Input::isKeyDown(Key key) const
	{
		return m_keysDown[toIndex(key)];
	}

	bool Input::isKeyPressed(Key key) const
	{
		return m_keysPressed[toIndex(key)];
	}

	bool Input::isKeyReleased(Key key) const
	{
		return m_keysReleased[toIndex(key)];
	}

	bool Input::isMouseButtonDown(MouseButton button) const
	{
		return m_mouseDown[toIndex(button)];
	}

	void Input::onKeyEvent(int glfwKey, int action)
	{
		Key mapped = fromGlfwKey(glfwKey);
		if (mapped == Key::Count) return;

		size_t index = toIndex(mapped);
		if (action == GLFW_PRESS)
		{
			m_keysDown[index] = true;
			m_keysPressed[index] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			m_keysDown[index] = false;
			m_keysReleased[index] = true;
		}
	}

	void Input::onMouseButtonEvent(int glfwButton, int action)
	{
		MouseButton mapped = fromGlfwMouseButton(glfwButton);
		if (mapped == MouseButton::Count) return;

		m_mouseDown[toIndex(mapped)] = ( action != GLFW_RELEASE );
	}

	void Input::onCursorPosEvent(double x, double y)
	{
		math::Vec2f pos{ static_cast<float>( x ), static_cast<float>( y ) };

		if (m_firstMouseEvent)
		{
			m_lastMousePos = pos;
			m_firstMouseEvent = false;
		}

		m_mouseDelta = m_mouseDelta + ( pos - m_lastMousePos );
		m_lastMousePos = pos;
		m_mousePos = pos;
	}

	// Leave xoffset commented out to keep the compiler from bitching
	void Input::onScrollEvent(double /*xoffset*/, double yoffset)
	{
		m_scrollDelta += static_cast<float>( yoffset );
	}
}
