#include <app/gizmo_layer.h>
#include <camera/camera.h>
#include <input/input.h>
#include <gfx/device.h>
#include <core/log/log.h>

namespace imp::app
{
	namespace
	{
		constexpr float kPickThresholdFraction = 0.02f;
		// RGBA (224, 50, 50, 255)
		constexpr math::Vec4f kAxisColourX{ 0.8784314f, 0.1960784f, 0.1960784f, 1.f };
		// RGBA (114, 204, 114, 255)
		constexpr math::Vec4f kAxisColourY{ 0.4470588f, 0.8f, 0.4470588f, 1.f };
		// RGBA (93, 182, 229, 255)
		constexpr math::Vec4f kAxisColourZ{ 0.3647059f, 0.7137255f, 0.8980392f, 1.f };
		// RGBA (240, 200, 80, 255)
		constexpr math::Vec4f kAxisColourHighlight{ 0.9411765f, 0.7843137f, 0.3137255f, 1.f };

		math::Vec3f axisDirFor(GizmoAxis axis, const math::Mat4f& world)
		{
			switch (axis)
			{
			case imp::app::GizmoAxis::None: return math::Vec3f::zero();
			case imp::app::GizmoAxis::X: return math::normalise(math::Vec3f{ world[0][0], world[0][1], world[0][2] });
			case imp::app::GizmoAxis::Y: return math::normalise(math::Vec3f{ world[1][0], world[1][1], world[1][2] });
			case imp::app::GizmoAxis::Z: return math::normalise(math::Vec3f{ world[2][0], world[2][1], world[2][2] });
			}

			return math::Vec3f::zero();
		}

		math::Vec4f axisColour(GizmoAxis axis, GizmoAxis hovered, GizmoAxis active)
		{
			if (axis == active || axis == hovered)
				return kAxisColourHighlight;
			switch (axis)
			{
			case imp::app::GizmoAxis::None: return math::Vec4f{ 1.f, 1.f, 1.f, 1.f };
			case imp::app::GizmoAxis::X: return kAxisColourX;
			case imp::app::GizmoAxis::Y: return kAxisColourY;
			case imp::app::GizmoAxis::Z: return kAxisColourZ;
			}
			return math::Vec4f{ 1.f, 1.f, 1.f, 1.f };
		}
	}

	GizmoLayer::GizmoLayer(gfx::IDevice& device, ecs::World& world, const fwk::Camera& camera, fwk::Input& input,
		gfx::TextureFormat colourFormat, gfx::TextureFormat depthFormat, gfx::SampleCount sampleCount)
		: ILayer("Gizmo")
		, m_device(device)
		, m_world(world)
		, m_camera(camera)
		, m_input(input)
		, m_colourFormat(colourFormat)
		, m_depthFormat(depthFormat)
		, m_sampleCount(sampleCount)
		, m_raycaster(world)
	{}

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
			gizmos.drawGrid(10000.f, 10.f, { 0.4f, 0.4f, 0.4f, 0.4f });

		const math::Vec2u screenSize{ m_device.backBuffer().width(), m_device.backBuffer().height() };
		const math::Vec2f mousePos = m_input.mousePosition();
		const physics::Ray ray = screenPointToRay(mousePos, screenSize);

		const bool hasSelection = m_selected.isValid() && m_world.transforms.contains(m_selected);
		bool clickConsumedByGizmo = false;

		if (hasSelection)
		{
			const math::Mat4f world = m_world.transforms.worldMatrix(m_selected);
			const math::Vec3f origin = math::transformPoint(world, math::Vec3f::zero());
			const float cameraDistance = math::length(origin - m_camera.position());

			if (m_activeAxis == GizmoAxis::None)
			{
				m_hoveredAxis = pickAxis(ray, origin, m_axisLength, cameraDistance);

				if (m_hoveredAxis != GizmoAxis::None && m_input.isMouseButtonPressed(fwk::MouseButton::Left))
				{
					m_activeAxis = m_hoveredAxis;
					m_dragAxisOrigin = origin;
					m_dragAxisDir = axisDirFor(m_activeAxis, world);

					float tRay, tAxis;
					closestParams(ray, m_dragAxisOrigin, m_dragAxisDir, tRay, tAxis);
					m_dragStartT = tAxis;
					m_dragStartPosition = m_world.transforms.localTransform(m_selected).position;

					clickConsumedByGizmo = true;
				}
			}
			else
			{
				clickConsumedByGizmo = true; // still dragging

				if (!m_input.isMouseButtonDown(fwk::MouseButton::Left))
				{
					m_activeAxis = GizmoAxis::None;
				}
				else
				{
					float tRay, tAxis;
					if (closestParams(ray, m_dragAxisOrigin, m_dragAxisDir, tRay, tAxis))
					{
						const float delta = tAxis - m_dragStartT;
						ecs::Transform t = m_world.transforms.localTransform(m_selected);
						t.position = m_dragStartPosition + m_dragAxisDir * delta;
						m_world.transforms.setLocalTransform(m_selected, t);
					}
				}
			}

			for (GizmoAxis axis : { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z })
			{
				const math::Vec3f dir = axisDirFor(axis, world);
				const math::Vec4f colour = axisColour(axis, m_hoveredAxis, m_activeAxis);
				gizmos.drawArrow(origin, origin + dir * m_axisLength, colour);
			}
		}

		if (!clickConsumedByGizmo && m_input.isMouseButtonPressed(fwk::MouseButton::Left))
		{
			if (auto hit = m_raycaster.raycast(ray))
				setSelected(hit->entity);
			else
				clearSelection();
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

	physics::Ray GizmoLayer::screenPointToRay(const math::Vec2f& screenPos, const math::Vec2u& screenSize) const
	{
		const float aspect = screenSize.x > 0
			? static_cast<float>( screenSize.x ) / static_cast<float>( screenSize.y )
			: 1.f;

		const math::Mat4f viewProj = m_camera.projection(aspect) * m_camera.view();
		const math::Mat4f invViewProj = math::inverse(viewProj);

		const float ndcX = ( screenPos.x / static_cast<float>( screenSize.x ) ) * 2.f - 1.f;
		const float ndcY = 1.f - ( screenPos.y / static_cast<float>( screenSize.y ) ) * 2.f;

		math::Vec4f nearH = invViewProj * math::Vec4f{ ndcX, ndcY, 0.f, 1.f };
		math::Vec4f farH = invViewProj * math::Vec4f{ ndcX, ndcY, 1.f, 1.f };
		const math::Vec3f nearP = math::Vec3f{ nearH.x, nearH.y, nearH.z } / nearH.w;
		const math::Vec3f farP = math::Vec3f{ farH.x, farH.y, farH.z } / farH.w;

		return physics::Ray{ nearP, math::normalise(farP - nearP) };
	}

	bool GizmoLayer::closestParams(const physics::Ray& ray, const math::Vec3f& axisOrigin, const math::Vec3f& axisDir,
		float& tRay, float& tAxis)
	{
		const math::Vec3f r = ray.origin - axisOrigin;
		const float b = math::dot(ray.direction, axisDir);
		const float d = math::dot(ray.direction, r);
		const float e = math::dot(axisDir, r);
		const float denom = 1.f - b * b; // |ray.dir| == |axisDir| == 1

		if (std::abs(denom) < 1e-5f)
			return false;

		tRay = ( b * e - d ) / denom;
		tAxis = ( e - b * d ) / denom;
		return true;
	}

	GizmoAxis GizmoLayer::pickAxis(const physics::Ray& ray, const math::Vec3f& origin, float axisLength, float cameraDistance) const
	{
		const math::Mat4f world = m_world.transforms.worldMatrix(m_selected);
		const GizmoAxis axes[3] = { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z };

		const float threshold = std::max(cameraDistance * kPickThresholdFraction, 0.02f);
		GizmoAxis best = GizmoAxis::None;
		float bestDist = threshold;

		for (GizmoAxis axis : axes)
		{
			const math::Vec3f dir = axisDirFor(axis, world);
			float tRay, tAxis;
			if (!closestParams(ray, origin, dir, tRay, tAxis))
				continue;
			if (tRay < 0.f || tAxis < 0.f || tAxis > axisLength)
				continue;

			const math::Vec3f onRay = ray.origin + ray.direction * tRay;
			const math::Vec3f onAxis = origin + dir * tAxis;
			const float dist = math::length(onRay - onAxis);

			if (dist < bestDist)
			{
				bestDist = dist;
				best = axis;
			}
		}

		return best;
	}


}
