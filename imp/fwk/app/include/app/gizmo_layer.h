#pragma once

#include <fwk/layer.h>
#include <ecs/entity.h>
#include <ecs/world.h>
#include <gfx/gizmo_renderer.h>
#include <core/math/math.h>
#include <core/types/int_types.h>
#include <gfx/resources.h>

#include <physics/query.h>

namespace imp::gfx 
{ 
	class IDevice;
}

namespace imp::fwk { 
	class Camera; 
	class Input;
}

namespace imp::app
{
	enum class GizmoAxis { None, X, Y, Z };
	class GizmoLayer final : public fwk::ILayer
	{
	public:
		GizmoLayer(gfx::IDevice& device, ecs::World& world, const fwk::Camera& camera, fwk::Input& input,
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
		physics::Ray screenPointToRay(const math::Vec2f& screenPos, const math::Vec2u& screenSize) const;

		static bool closestParams(const physics::Ray& ray, const math::Vec3f& axisOrigin, const math::Vec3f& axisDir,
			float& tRay, float& tAxis);

		GizmoAxis pickAxis(const physics::Ray& ray, const math::Vec3f& origin, float axisLength, float cameraDistance) const;

		gfx::IDevice& m_device;
		ecs::World& m_world;
		const fwk::Camera& m_camera;
		fwk::Input& m_input;
		gfx::TextureFormat m_colourFormat;
		gfx::TextureFormat m_depthFormat;
		gfx::SampleCount m_sampleCount;

		ecs::EntityId m_selected = ecs::kInvalidEntity;
		bool m_showGrid = false;
		float m_axisLength = 8.f;

		GizmoAxis m_hoveredAxis = GizmoAxis::None;
		GizmoAxis m_activeAxis = GizmoAxis::None;
		math::Vec3f m_dragAxisOrigin{};
		math::Vec3f m_dragAxisDir{};
		float m_dragStartT = 0.f;
		math::Vec3f m_dragStartPosition{};

		physics::Raycaster m_raycaster;
	};
}
