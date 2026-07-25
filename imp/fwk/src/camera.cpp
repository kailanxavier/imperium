#include <fwk/camera.h>
#include <fwk/input.h>

#include <algorithm>
#include <cmath>

namespace imp::fwk
{
	math::Vec3f Camera::forward() const
	{
		using namespace imp::math;
		float cp = std::cos(m_pitch);
		return normalise(Vec3f{ cp * std::sin(m_yaw), std::sin(m_pitch), cp * std::cos(m_yaw) });
	}

	math::Vec3f Camera::right() const
	{
		using namespace imp::math;
		return normalise(cross(Vec3f::up(), forward()));
	}

	void Camera::update(const Input & input, float deltaTime)
	{
		using namespace imp::math;

		if (input.isMouseButtonDown(MouseButton::Right))
		{
			Vec2f delta = input.mouseDelta();
			m_yaw += delta.x * lookSensitivity;
			m_pitch -= delta.y * lookSensitivity;

			constexpr float kPitchLimit = toRadians(89.f);
			m_pitch = std::clamp(m_pitch, -kPitchLimit, kPitchLimit);
		}

		Vec3f fwd = forward();
		Vec3f rgt = right();

		Vec3f moveDir = Vec3f::zero();
		if (input.isKeyDown(Key::W)) moveDir += fwd;
		if (input.isKeyDown(Key::S)) moveDir -= fwd;
		if (input.isKeyDown(Key::A)) moveDir -= rgt;
		if (input.isKeyDown(Key::D)) moveDir += rgt;
		if (input.isKeyDown(Key::LeftControl)) moveDir -= Vec3f::up();
		if (input.isKeyDown(Key::Space)) moveDir += Vec3f::up();

		if (dot(moveDir, moveDir) > 0.f)
			m_position += normalise(moveDir) * ( moveSpeed * deltaTime );
	}

	math::Mat4f Camera::view() const
	{
		using namespace imp::math;
		return makeLookAtLH(m_position, m_position + forward(), Vec3f::up());
	}

	math::Mat4f Camera::projection(float aspect) const
	{
		return math::makePerspectiveLH(fovRadians, aspect, nearPlane, farPlane);
	}
}
