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
        gfx::CascadeConfig& shadowConfig = *m_refs.cascade;

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
