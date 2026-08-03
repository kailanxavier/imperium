#pragma once

#include <fwk/layer.h>
#include <ecs/entity.h>
#include <ecs/world.h>
#include <gfx/gizmo_renderer.h>
#include <core/math/math.h>
#include <core/types/int_types.h>
#include <gfx/resources.h>

namespace imp::gfx 
{ 
	class IDevice;
}

namespace imp::fwk { class Camera; }

namespace imp::app
{
	class GizmoLayer final : public fwk::ILayer
	{
	public:
		GizmoLayer(gfx::IDevice& device, ecs::World& world, const fwk::Camera& camera,
			gfx::TextureFormat colourFormat, gfx::TextureFormat depthFormat, gfx::SampleCount sampleCount);

		void onAttach() override;
		void onDetach() override;
		void onUpdate(float deltaSeconds) override;
		void onRender(gfx::ICommandList& cmd) override;

		void setSelected(ecs::EntityId entity) { m_selected = entity; }
		void clearSelection() { m_selected = ecs::kInvalidEntity; }
		ecs::EntityId selected() const { return m_selected; }

		void setShowGrid(bool show) { m_showGrid = show; }
		void setAxisLength(float length) { m_axisLength = length; }

	private:
		gfx::IDevice& m_device;
		ecs::World& m_world;
		const fwk::Camera& m_camera;
		gfx::TextureFormat m_colourFormat;
		gfx::TextureFormat m_depthFormat;
		gfx::SampleCount m_sampleCount;

		ecs::EntityId m_selected = ecs::kInvalidEntity;
		bool m_showGrid = false;
		float m_axisLength = 2.f;
	};
}
