#include <camera/camera.h>
#include <input/input.h>

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

		const float speedMul = input.isKeyDown(Key::LeftShift) ? moveSpeed * 100.f : moveSpeed;

		if (dot(moveDir, moveDir) > 0.f)
			m_position += normalise(moveDir) * ( speedMul * deltaTime );
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

	std::array<math::Vec3f, 8> Camera::frustumCornersWorldSpace(float aspect, float sliceNear, float sliceFar) const
	{
		const math::Mat4f sliceProj = math::makePerspectiveLH(fovRadians, aspect, sliceNear, sliceFar);
		const math::Mat4f invViewProj = math::inverse(sliceProj * view());

		std::array<math::Vec3f, 8> corners{};
		u32 i = 0;
		for (int x = 0; x < 2; ++x)
			for (int y = 0; y < 2; ++y)
				for (int z = 0; z < 2; ++z)
				{
					math::Vec4f ndc{
						2.f * static_cast<float>(x) - 1.f,
						2.f * static_cast<float>(y) - 1.f,
						static_cast<float>(z),
						1.f
					};
					math::Vec4f world = invViewProj * ndc;
					corners[i++] = math::perspectiveDivide(world);
				}
		return corners;
	}
}
