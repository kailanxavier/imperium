#include <gfx/gizmo_renderer.h>
#include <gfx/device.h>
#include <gfx/commands.h>
#include <gfx/pipeline.h>
#include <core/log/log.h>

namespace imp::gfx
{
	GizmoRenderer& GizmoRenderer::instance()
	{
		static GizmoRenderer s_instance;
		return s_instance;
	}

	bool GizmoRenderer::initialise(gfx::IDevice& device, gfx::TextureFormat colourFormat,
		gfx::TextureFormat depthFormat, gfx::SampleCount sampleCount)
	{
		if (m_initialised)
			return true;

		m_device = &device;

		gfx::ShaderDesc vertDesc{};
		vertDesc.stage = gfx::ShaderStage::Vertex;
		vertDesc.path = "assets/shaders/gizmo.vert.spv";
		m_vertShader = m_device->createShader(vertDesc);

		gfx::ShaderDesc fragDesc{};
		fragDesc.stage = gfx::ShaderStage::Fragment;
		fragDesc.path = "assets/shaders/gizmo.frag.spv";
		m_fragShader = m_device->createShader(fragDesc);

		if (!m_vertShader || !m_fragShader)
		{
			LOG_ERROR("Gizmo", "Failed to load Gizmo shaders");
			return false;
		}

		gfx::VertexAttribute attrs[2] = {
			{ 0, static_cast<u32>( offsetof(GizmoVertex, position) ), 3, true},
			{ 1, static_cast<u32>( offsetof(GizmoVertex, colour) ), 4, true},
		};

		gfx::PipelineDesc desc{};
		desc.vertexShader = m_vertShader.get();
		desc.fragmentShader = m_fragShader.get();
		desc.vertexLayout.stride = sizeof(GizmoVertex);
		desc.vertexLayout.attributeCount = 2;
		desc.vertexLayout.attributes = attrs;
		desc.rasterizerState.cullMode = gfx::CullMode::None;
		desc.depthStencilState.depthTestEnable = true;
		desc.depthStencilState.depthWriteEnable = false; // never occludes scene geometry
		desc.depthStencilState.depthCompareOp = gfx::CompareOp::LessOrEqual;
		desc.blendState.blendEnable = false;
		desc.colourFormat = colourFormat;
		desc.depthFormat = depthFormat;
		desc.sampleCount = sampleCount;
		desc.rasterizerState.topology = gfx::PrimitiveTopology::LineList;

		m_pipeline = m_device->createPipeline(desc);
		if (!m_pipeline)
		{
			LOG_ERROR("Gizmo", "Failed to create Gizmo pipeline");
			return false;
		}

		m_initialised = true;
		return true;
	}

	void GizmoRenderer::shutdown()
	{
		m_vertices.clear();
		m_vertexBuffer.reset();
		m_fragShader.reset();
		m_vertShader.reset();
		m_pipeline.reset();
		m_vertexCapacity = 0;
		m_device = nullptr;
		m_initialised = false;
	}

	void GizmoRenderer::ensureCapacity(u32 vertexCount)
	{
		if (m_vertexBuffer && vertexCount <= m_vertexCapacity)
			return;

		u32 newCapacity = std::max<u32>(vertexCount, m_vertexCapacity * 2);
		newCapacity = std::max<u32>(newCapacity, 512u);

		gfx::BufferDesc desc;
		desc.size = static_cast<u64>( newCapacity ) * sizeof(GizmoVertex);
		desc.usage = gfx::BufferUsage::Vertex;
		desc.memoryAccess = gfx::MemoryAccess::HostVisible;
		desc.debugName = "GizmoRenderer vertex buffer";

		auto newBuffer = m_device->createBuffer(desc);
		if (!newBuffer)
		{
			LOG_ERROR("Gizmo", "Failed to grow gizmo vertex buffer to {}", newCapacity);
			return;
		}

		m_vertexBuffer = std::move(newBuffer);
		m_vertexCapacity = newCapacity;
	}

	void GizmoRenderer::drawLine(const math::Vec3f& a, const math::Vec3f& b, const math::Vec4f& colour)
	{
		m_vertices.push_back({ a, colour });
		m_vertices.push_back({ b, colour });
	}

	void GizmoRenderer::drawRay(const math::Vec3f& origin, const math::Vec3f& dir, float length, const math::Vec4f& colour)
	{
		drawLine(origin, origin + math::normalise(dir) * length, colour);
	}

	void GizmoRenderer::drawAxes(const math::Mat4f& transform, float axisLength)
	{
		const math::Vec3f origin = math::transformPoint(transform, math::Vec3f::zero());
		const math::Vec3f x = math::transformPoint(transform, math::Vec3f{ axisLength, 0.f, 0.f });
		const math::Vec3f y = math::transformPoint(transform, math::Vec3f{ 0.f, axisLength, 0.f });
		const math::Vec3f z = math::transformPoint(transform, math::Vec3f{ 0.f, 0.f, axisLength });

		drawLine(origin, x, { 1.f, 0.f, 0.f, 1.f });
		drawLine(origin, y, { 0.f, 1.f, 0.f, 1.f });
		drawLine(origin, z, { 0.f, 0.f, 1.f, 1.f });
	}

	void GizmoRenderer::drawBox(const math::Mat4f& transform, const math::Vec4f& colour)
	{
		static constexpr math::Vec3f kCorners[8] = {
			{ -0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, -0.5f }, { -0.5f, 0.5f, -0.5f },
			{ -0.5f, -0.5f,  0.5f }, { 0.5f, -0.5f,  0.5f }, { 0.5f, 0.5f,  0.5f }, { -0.5f, 0.5f,  0.5f },
		};
		static constexpr int kEdges[12][2] = {
			{0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4}, {0,4},{1,5},{2,6},{3,7}
		};

		math::Vec3f world[8];
		for (int i = 0; i < 8; ++i)
			world[i] = math::transformPoint(transform, kCorners[i]);

		for (auto& e : kEdges)
			drawLine(world[e[0]], world[e[1]], colour);
	}

	void GizmoRenderer::drawSphere(const math::Vec3f& centre, float radius, const math::Vec4f& colour, u32 segments)
	{
		auto ring = [&](int axis)
			{
				math::Vec3f prev{};
				for (u32 i = 0; i <= segments; ++i)
				{
					const float t = ( static_cast<float>(i) / static_cast<float>(segments) ) * 2.f * math::kPif;
					math::Vec3f p{};
					if (axis == 0) p = { 0.f, std::cos(t) * radius, std::sin(t) * radius };
					else if (axis == 1) p = { std::cos(t) * radius, 0.f, std::sin(t) * radius };
					else p = { std::cos(t) * radius, std::sin(t) * radius, 0.f };
					p = p + centre;
					if (i > 0) drawLine(prev, p, colour);
					prev = p;
				}
			};
		ring(0); ring(1); ring(2);
	}

	void GizmoRenderer::drawArrow(const math::Vec3f& from, const math::Vec3f& to, const math::Vec4f& colour, float headSize)
	{
		drawLine(from, to, colour);
		const math::Vec3f dir = math::normalise(to - from);
		const math::Vec3f up = std::abs(math::dot(dir, math::Vec3f::up())) > 0.95f ? math::Vec3f::unitX() : math::Vec3f::up();
		const math::Vec3f side = math::normalise(math::cross(dir, up)) * headSize;
		const math::Vec3f back = to - dir * headSize * 2.f;
		drawLine(to, back + side, colour);
		drawLine(to, back - side, colour);
	}

	void GizmoRenderer::drawGrid(float extent, float spacing, const math::Vec4f& colour)
	{
		for (float x = -extent; x <= extent; x += spacing)
			drawLine({ x, 0.f, -extent }, { x, 0.f, extent }, colour);
		for (float z = -extent; z <= extent; z += spacing)
			drawLine({ -extent, 0.f, z }, { extent, 0.f, z }, colour);
	}

	void GizmoRenderer::render(gfx::ICommandList& cmd, const math::Mat4f& viewProj)
	{
		if (!m_initialised || m_vertices.empty())
		{
			m_vertices.clear();
			return;
		}

		ensureCapacity(static_cast<u32>( m_vertices.size() ));
		if (!m_vertexBuffer)
		{
			m_vertices.clear();
			return;
		}

		std::memcpy(m_vertexBuffer->mappedData(), m_vertices.data(), m_vertices.size() * sizeof(GizmoVertex));

		GizmoPushConstants pc{ viewProj };
		cmd.bindPipeline(*m_pipeline);
		cmd.pushConstants(&pc, sizeof(pc));
		cmd.bindVertexBuffer(*m_vertexBuffer, 0);
		cmd.draw(static_cast<u32>( m_vertices.size() ), 1);

		m_vertices.clear();
	}
}
