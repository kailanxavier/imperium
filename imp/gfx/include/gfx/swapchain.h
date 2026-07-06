#pragma once
#include <core/memory/int_types.h>

namespace imp::gfx
{
    class ISwapChain
    {
    public:
        virtual ~ISwapChain() = default;

        // Tears down and rebuilds against a new size.
        // Safe to call with width/height 0 - implementations
        // should go into a not ready state instead of failing.
        virtual void resize(u32 width, u32 height) = 0;

        // Waits for the next back buffer to become available
        // and acquires it. Returns false if the frame should be skipped.
        // Callers must not record or submit any commands for this frame
        // when this returns false.
        virtual bool beginFrame() = 0;

        // Presents the image that was acquired by the most recent
        // successful beginFrame(). Only valid to call if begindFrame()
        // returned true.
        virtual void present() = 0;

        virtual u32 currentBackBufferIndex() const = 0;
        virtual u32 backBufferCount() const = 0;
    };
}