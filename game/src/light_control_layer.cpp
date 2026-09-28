#include <game/light_control_layer.h>
#include <imgui.h>
#include <bit>

namespace imp::game
{
    LightControlLayer::LightControlLayer() : ILayer("LightControl") {}

    void LightControlLayer::onAttach(AppContext& ctx)
    {
        m_refs = ctx.services.require<LightControlRefs>();
    }

    void LightControlLayer::onUpdate(AppContext& /*ctx*/, float /*deltaSeconds*/)
    {
        math::Vec3f& sunDirection = *m_refs.sunDirection;
        gfx::CascadeConfig& shadowConfig = *m_refs.cascade;

        float azimuth, elevation;
        {
            const math::Vec3f dir = math::normalise(sunDirection);
            azimuth = std::atan2(dir.x, dir.z);
            elevation = std::asin(dir.y);
        }

        ImGui::Begin("Sun Control");
        ImGui::SliderAngle("Azimuth", &azimuth, -180.f, 180.f);
        ImGui::SliderAngle("Elevation", &elevation, -89.f, 89.f);
        ImGui::End();

        sunDirection.x = std::cos(elevation) * std::sin(azimuth);
        sunDirection.y = std::sin(elevation);
        sunDirection.z = std::cos(elevation) * std::cos(azimuth);

        ImGui::Begin("CSM");
        ImGui::SliderFloat("Lambda", &shadowConfig.splitLambda, 0.f, 1.f);
        ImGui::SliderFloat("Padding Z", &shadowConfig.zPadding, 0.f, 100.f);
        for (u32 i = 0; i < gfx::kCascadeCount; ++i)
        {
            char label[32];
            std::snprintf(label, sizeof(label), "Resolution [%u]", i);

            int exponent = std::countr_zero(shadowConfig.resolution[i]);
            if (shadowConfig.resolution[i] == 0)
                exponent = 7;

            if (ImGui::SliderInt(label, &exponent, 7, 13, "Resolution: %d"))
                shadowConfig.resolution[i] = 1u << exponent;

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("2^%d = Resolution: %u", exponent, 1u << exponent);
        }
        ImGui::End();
    }
}
