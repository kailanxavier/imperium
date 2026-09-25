#pragma once
#include <core/math/math.h>

namespace imp::gfx
{
    struct AOParamsUBO
    {
        math::Mat4f invProj;
        math::Mat4f invView;
        math::Mat4f view;
        math::Vec4f params;
        math::Vec4f params2;
    };

    struct SSGIParamsUBO
    {
        math::Mat4f invProj;
        math::Mat4f invView;
        math::Mat4f view;
        math::Vec4f params;
        math::Vec4f params2;
    };

    struct ScreenParamsUBO
    {
        math::Vec4f resolutionAndInv;
        math::Vec4f flags;
    };

    struct BlurParamsUBO
    {
        math::Vec4f texelSizeAndSigmas;
    };

    struct PrevViewProjUBO
    {
        math::Mat4f prevViewProj;
        math::Vec4f jitterNdc{0.f, 0.f, 0.f, 0.f};
    };
}
