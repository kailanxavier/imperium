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

    struct ScreenParamsUBO
    {
        math::Vec4f resolutionAndInv;
        math::Vec4f flags;
    };

    struct BlurParamsUBO
    {
        math::Vec4f texelSizeAndSigmas;
    };
}
