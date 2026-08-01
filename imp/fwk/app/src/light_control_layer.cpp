#include <app/light_control_layer.h>
#include <imgui.h>

namespace imp::app
{
    LightControlLayer::LightControlLayer(math::Vec3f& sunDirection, ecs::Transform& t)
		: ILayer("LightControl")
		, m_sunDirection(sunDirection)
        , m_pointLightT(t)
    {}

    void LightControlLayer::onUpdate(float /*deltaSeconds*/)
    {
        float azimuth, elevation;
        {
            const math::Vec3f dir = math::normalise(m_sunDirection);
            azimuth = math::toDegrees(std::atan2(dir.x, dir.z));
            elevation = math::toDegrees(std::asin(dir.y));
        }

        ImGui::Begin("Sun Control");
        ImGui::SliderFloat("Azimuth", &azimuth, -180.0f, 180.0f);
        ImGui::SliderFloat("Elevation", &elevation, -89.0f,  89.0f);
        ImGui::End();

        ImGui::Begin("Point Light");
        ImGui::DragFloat3("Position", &m_pointLightT.position.x, 0.01f);
        ImGui::DragFloat3("Rotation", &m_pointLightT.rotation.x, 0.01f);
        ImGui::DragFloat3("Scale", &m_pointLightT.scale.x, 0.01f);
        ImGui::End();

        const float az = math::toRadians(azimuth);
        const float el = math::toRadians(elevation);
        m_sunDirection.x =  std::cos(el) * std::sin(az);
        m_sunDirection.y =  std::sin(el);
        m_sunDirection.z =  std::cos(el) * std::cos(az);
    }
}
