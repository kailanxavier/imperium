#pragma once
#include <core/math/math.h>
#include <core/types/int_types.h>

namespace imp::gfx::taa
{
    [[nodiscard]] inline float halton(u32 index, u32 base) noexcept
    {
        float f = 1.f;
        float r = 0.f;

        while (index > 0)
        {
            f /= static_cast<float>(base);
            r += f * static_cast<float>(index % base);
            index /= base;
        }
        return r;
    }

    [[nodiscard]] inline math::Vec2f jitterPixels(u32 frameCounter, u32 sampleCount) noexcept
    {
        const u32 count = sampleCount > 0 ? sampleCount : 1;
        const u32 index = (frameCounter % count) + 1;
        return math::Vec2f{ halton(index, 2) - 0.5f, halton(index, 3) - 0.5f };
    }

    [[nodiscard]] inline math::Vec2f pixelsToNdc(const math::Vec2f& pixels, u32 width, u32 height) noexcept
    {
        return math::Vec2f{
            2.f * pixels.x / static_cast<float>(width),
            2.f * pixels.y / static_cast<float>(height) };
    }

    [[nodiscard]] inline math::Mat4f applyJitter(const math::Mat4f& clipFromSomething, const math::Vec2f& jitterNdc) noexcept
    {
        math::Mat4f jitter = math::Mat4f::identity();
        jitter(0, 3) = jitterNdc.x;
        jitter(1, 3) = jitterNdc.y;
        return jitter * clipFromSomething;
    }
}
