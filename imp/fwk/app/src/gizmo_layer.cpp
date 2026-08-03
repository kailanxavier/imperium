#include <app/gizmo_layer.h>
#include <camera/camera.h>
#include <gfx/device.h>
#include <core/log/log.h>

namespace imp::app
{
	GizmoLayer::GizmoLayer(gfx::IDevice& device, ecs::World& world, const fwk::Camera& camera,
		gfx::TextureFormat colourFormat, gfx::TextureFormat depthFormat, gfx::SampleCount sampleCount)
		: ILayer("Gizmo")
		, m_device(device)
		, m_world(world)
		, m_camera(camera)
		, m_colourFormat(colourFormat)
		, m_depthFormat(depthFormat)
		, m_sampleCount(sampleCount)
	{
	}

	void GizmoLayer::onAttach()
	{
		auto& gizmos = gfx::GizmoRenderer::instance();
		if (!gizmos.isInitialised())
		{
			if (!gizmos.initialise(m_device, m_colourFormat, m_depthFormat, m_sampleCount))
				LOG_ERROR("Gizmo", "GizmoLayer failed to initialise GizmoRenderer");
		}
	}

	void GizmoLayer::onDetach()
	{
		// We're not shutting down gizmo renderer here because
		// it's a shared singleton, so other layers might still be using it
	}


	void GizmoLayer::onUpdate(float /*deltaSeconds*/)
	{
		auto& gizmos = gfx::GizmoRenderer::instance();
		if (!gizmos.isInitialised())
			return;

		if (m_showGrid)
			gizmos.drawGrid(50.f, 1.f, { 0.4f, 0.4f, 0.4f, 1.f });

		if (m_selected.isValid() && m_world.transforms.contains(m_selected))
		{
			const math::Mat4f& world = m_world.transforms.worldMatrix(m_selected);
			gizmos.drawAxes(world, m_axisLength);
		}
	}

	void GizmoLayer::onRender(gfx::ICommandList& cmd)
	{
		auto& gizmos = gfx::GizmoRenderer::instance();
		if (!gizmos.isInitialised())
			return;

		const u32 w = m_device.backBuffer().width();
		const u32 h = m_device.backBuffer().height();
		const float aspect = h > 0 ? static_cast<float>( w ) / static_cast<float>( h ) : 1.f;
		const math::Mat4f viewProj = m_camera.projection(aspect) * m_camera.view();

		gizmos.render(cmd, viewProj);
	}
}
