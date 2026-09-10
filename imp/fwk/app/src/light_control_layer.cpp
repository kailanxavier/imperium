#include <app/light_control_layer.h>
#include <imgui.h>

namespace imp::app
{
    LightControlLayer::LightControlLayer(math::Vec3f& sunDirection, ecs::Transform& t, gfx::CascadeConfig& shadowConfig)
		: ILayer("LightControl")
		, m_sunDirection(sunDirection)
        , m_pointLightT(t)
        , m_shadowConfig(shadowConfig)
    {}

    void LightControlLayer::onUpdate(float /*deltaSeconds*/)
    {
        float azimuth, elevation;
        {
            const math::Vec3f dir = math::normalise(m_sunDirection);
            azimuth = std::atan2(dir.x, dir.z);
            elevation = std::asin(dir.y);
        }

        ImGui::Begin("Sun Control");
        ImGui::SliderAngle("Azimuth", &azimuth, -180.f, 180.f);
        ImGui::SliderAngle("Elevation", &elevation, -89.f, 89.f);
        ImGui::End();

        ImGui::Begin("CSM");
        ImGui::SliderFloat("Lambda", &m_shadowConfig.splitLambda, 0.f, 1.f);
        ImGui::SliderFloat("Padding Z", &m_shadowConfig.zPadding, 0.f, 100.f);
        ImGui::SliderInt("Shadow Map Resolution", reinterpret_cast<int*>( &m_shadowConfig.shadowMapResolution ), 1, 8192);
        ImGui::End();

        m_sunDirection.x = std::cos(elevation) * std::sin(azimuth);
        m_sunDirection.y = std::sin(elevation);
        m_sunDirection.z = std::cos(elevation) * std::cos(azimuth);
    }
}
