#pragma once

#include <core/types/handle.h>
#include <core/math/math.h>
#include <core/types/int_types.h>
#include <gfx/resources.h>
#include <vector>
#include <memory>

namespace imp::gfx
{
	class IDevice;
	class IShader;
	class IPipeline;
	class IBuffer;
	class ICommandList;
}

namespace imp::gfx
{
	struct GizmoVertex
	{
		math::Vec3f position;
		math::Vec4f colour;
	};

	struct GizmoPushConstants
	{
		math::Mat4f viewProj;
	};

	class GizmoRenderer
	{
	public:
		static GizmoRenderer& instance();

		bool initialise(gfx::IDevice& device, gfx::TextureFormat colourFormat,
			gfx::TextureFormat depthFormat, gfx::SampleCount sampleCount);

		void shutdown();
		bool isInitialised() const { return m_initialised; }

		void drawLine(const math::Vec3f& a, const math::Vec3f& b, const math::Vec4f& colour);
		void drawRay(const math::Vec3f& origin, const math::Vec3f& dir, float length, const math::Vec4f& colour);
		void drawBox(const math::Mat4f& transform, const math::Vec4f& colour);
		void drawSphere(const math::Vec3f& centre, float radius, const math::Vec4f& colour, u32 segments = 24);
		void drawArrow(const math::Vec3f& from, const math::Vec3f& to, const math::Vec4f& colour, float headSize = 0.15f);
		void drawAxes(const math::Mat4f& transform, float axisLength = 1.f);
		void drawGrid(float extent, float spacing, const math::Vec4f& colour);

		void render(gfx::ICommandList& cmd, const math::Mat4f& viewProj);
	private:
		GizmoRenderer() = default;

		void ensureCapacity(u32 vertexCount);

		std::unique_ptr<IShader> m_vertShader;
		std::unique_ptr<IShader> m_fragShader;
		std::unique_ptr<IPipeline> m_pipeline;
		std::unique_ptr<IBuffer> m_vertexBuffer;
		u32 m_vertexCapacity = 0;

		std::vector<GizmoVertex> m_vertices;

		IDevice* m_device = nullptr;
		bool m_initialised = false;
	};
}
