#pragma once

#include <core/math/math.h>
#include <array>

namespace imp::fwk
{
	class Input;
	class Camera
	{
	public:
		void update(const Input& input, float deltaTime);

		[[nodiscard]] math::Mat4f view() const;
		[[nodiscard]] math::Mat4f projection(float aspect) const;
		[[nodiscard]] math::Vec3f position() const { return m_position; }

		[[nodiscard]] std::array<math::Vec3f, 8> frustumCornersWorldSpace(
			float aspect, float sliceNear, float sliceFar) const;

		void setPosition(const math::Vec3f& pos) { m_position = pos; }
		void setYawPitch(float yawRadians, float pitchRadians) 
		{ 
			m_yaw = yawRadians; 
			m_pitch = pitchRadians; 
		}

		float moveSpeed = 2.f;
		float lookSensitivity = 0.0025f;
		float fovRadians = math::toRadians(90.f);
		float nearPlane = 0.1f;
		float farPlane = 1000.f;

	private:
		[[nodiscard]] math::Vec3f forward() const;
		[[nodiscard]] math::Vec3f right() const;

		math::Vec3f m_position = math::Vec3f::zero();
		float m_yaw = 0.f;
		float m_pitch = 0.f;
	};
}
