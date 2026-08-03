#pragma once

#include <core/types/int_types.h>
#include <core/math/math.h>

#include <array>

struct GLFWwindow;

namespace imp::fwk
{
	enum class Key : u32
	{
		Invalid = 0,

		Q, W, E, R, T, Y, U, I, O, P,
		A, S, D, F, G, H, J, K, L,
		Z, X, C, V, B, N, M,
		Space, LeftShift, Escape, LeftControl,

		Count
	};

	enum class MouseButton : u32
	{
		Invalid = 0,

		Left, Right, Middle,
		Four, Five,

		Count
	};

	class Input
	{
	public:
		void newFrame();

		[[nodiscard]] bool isKeyDown(Key key) const;
		[[nodiscard]] bool isKeyPressed(Key key) const;
		[[nodiscard]] bool isKeyReleased(Key key) const;

		[[nodiscard]] bool isMouseButtonDown(MouseButton button) const;
		[[nodiscard]] bool isMouseButtonPressed(MouseButton button) const;
		[[nodiscard]] math::Vec2f mousePosition() const { return m_mousePos; }
		[[nodiscard]] math::Vec2f mouseDelta() const { return m_mouseDelta; }
		[[nodiscard]] float scrollDelta() const { return m_scrollDelta; }

		void onKeyEvent(int glfwKey, int action);
		void onMouseButtonEvent(int glfwButton, int action);
		void onCursorPosEvent(double x, double y);
		void onScrollEvent(double xoffset, double yoffset);

	private:
		std::array<bool, static_cast<size_t>(Key::Count)> m_keysDown{};
		std::array<bool, static_cast<size_t>(Key::Count)> m_keysPressed{};
		std::array<bool, static_cast<size_t>(Key::Count)> m_keysReleased{};
		std::array<bool, static_cast<size_t>(MouseButton::Count)> m_mousePressed{};

		std::array<bool, static_cast<size_t>(MouseButton::Count)> m_mouseDown{};

		math::Vec2f m_mousePos = math::Vec2f::zero();
		math::Vec2f m_lastMousePos = math::Vec2f::zero();
		math::Vec2f m_mouseDelta = math::Vec2f::zero();
		float m_scrollDelta = 0.f;
		bool m_firstMouseEvent = true;
 	};
}
