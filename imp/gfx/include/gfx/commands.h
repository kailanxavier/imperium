#pragma once

#include <core/memory/int_types.h>

namespace imp::gfx
{
    class IPipeline;
    class IBuffer;
    class IRenderTarget;

    struct ClearColour
    {
        float r = 0.f;
        float g = 0.f;
        float b = 0.f;
        float a = 1.f;
    };

    struct RenderPassDesc
    {
        IRenderTarget* colourTarget = nullptr;
        IRenderTarget* depthTarget = nullptr;

        bool clearColour = true;
        ClearColour clearColourValue;

        bool clearDepth = true;
        float clearDepthValue = 1.f;
    };

    class ICommandList
    {
    public:
        virtual ~ICommandList() = default;

        virtual void beginRenderPass(const RenderPassDesc& desc) = 0;
        virtual void endRenderPass() = 0;

        virtual void bindPipeline(IPipeline& pipeline) = 0;
        virtual void bindVertexBuffer(IBuffer& buffer) = 0;
        virtual void bindIndexBuffer(IBuffer& buffer) = 0;

        virtual void pushConstants(const void* data, u32 size, u32 offset = 0) = 0;
        virtual void draw(u32 vertexCount, u32 instanceCount = 1) = 0;
        virtual void drawIndexed(u32 indexCount, u32 instanceCount = 1) = 0;

        // Compute dispatch and explicit resource barriers will go here
        // but since no backend implements compute yet, I won't do it rn
    };
}
