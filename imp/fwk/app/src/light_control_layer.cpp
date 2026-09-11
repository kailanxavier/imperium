#include <app/light_control_layer.h>
#include <imgui.h>
#include <bit>

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
        for (u32 i = 0; i < gfx::kCascadeCount; ++i)
        {
            char label[32];
            std::snprintf(label, sizeof(label), "Resolution [%u]", i);

            int exponent = std::countr_zero(m_shadowConfig.resolution[i]);
            if (m_shadowConfig.resolution[i] == 0)
                exponent = 7;

            if (ImGui::SliderInt(label, &exponent, 7, 13, "Resolution: %d"))
                m_shadowConfig.resolution[i] = 1u << exponent;

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("2^%d = Resolution: %u", exponent, 1u << exponent);
        }
        ImGui::End();

        m_sunDirection.x = std::cos(elevation) * std::sin(azimuth);
        m_sunDirection.y = std::sin(elevation);
        m_sunDirection.z = std::cos(elevation) * std::cos(azimuth);
    }
}
