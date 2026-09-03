#pragma once
#include <core/types/int_types.h>

namespace imp::gfx
{
    class IPipeline;
    class IBuffer;
    class ITexture;
    class ISampler;
    class IRenderTarget;

    // Default clear colour is (42, 3, 14, 255) RGBA
    struct ClearColour
    {
        float r = 42.f / 255.f;
        float g = 3.f / 255.f;
        float b = 14.f / 255.f;
        float a = 1.f;
    };

    struct RenderPassDesc
    {
        IRenderTarget* colourTarget = nullptr;
        IRenderTarget* depthTarget = nullptr;
        IRenderTarget* resolveTarget = nullptr;

        bool clearColour = true;
        ClearColour clearColourValue;

        bool clearDepth = true;
        float clearDepthValue = 1.f;

        const char* debugName = "unnamed pass";
    };

    class ICommandList
    {
    public:
        virtual ~ICommandList() = default;

        virtual void beginRenderPass(const RenderPassDesc& desc) = 0;
        virtual void endRenderPass() = 0;

        virtual void bindPipeline(IPipeline& pipeline) = 0;
        virtual void bindVertexBuffer(IBuffer& buffer, u32 binding) = 0;
        virtual void bindIndexBuffer(IBuffer& buffer) = 0;

        virtual void bindUniformBuffer(IBuffer& buffer, u32 binding) = 0;
        virtual void bindTexture(ITexture& texture, ISampler& sampler, u32 binding) = 0;

        virtual void pushConstants(const void* data, u32 size, u32 offset = 0) = 0;

        virtual void draw(u32 vertexCount, u32 instanceCount = 1) = 0;
        virtual void drawIndexed(u32 indexCount, u32 instanceCount, u32 firstInstance) = 0;

        // Compute dispatch and explicit resource barriers will go here
        // but since no backend implements compute yet, I won't do it rn
    };
}
