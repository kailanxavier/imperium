#pragma once
#include <core/types/int_types.h>

namespace imp::gfx
{
    class IPipeline;
    class IBuffer;
    class ITexture;
    class ISampler;
    class IRenderTarget;

    struct ClearColour
    {
        float r = 42.f / 255.f;
        float g = 3.f / 255.f;
        float b = 14.f / 255.f;
        float a = 1.f;
    };

    struct RenderPassColourAttachment
    {
        IRenderTarget* target = nullptr;
        bool clear = false;
        ClearColour clearValue;
    };

    struct RenderPassDesc
    {
        static constexpr u32 kMaxColourAttachments = 4;
        RenderPassColourAttachment colourTargets[kMaxColourAttachments]{};
        u32 colourTargetCount = 0;

        IRenderTarget* depthTarget = nullptr;
        IRenderTarget* resolveTarget = nullptr;

        bool clearDepth = true;
        float clearDepthValue = 1.f;

        const char* debugName = "unnamed pass";

        void setSingleColour(IRenderTarget* target, bool clearIt, ClearColour value = {})
        {
            colourTargetCount = target ? 1u : 0u;
            colourTargets[0].target = target;
            colourTargets[0].clear = clearIt;
            colourTargets[0].clearValue = value;
        }
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

        virtual void bindComputePipeline(IPipeline& pipeline) = 0;
        virtual void bindStorageImage(ITexture& texture, u32 binding) = 0;
        virtual void dispatch(u32 groupCountX, u32 groupCountY, u32 groupCountZ) = 0;

        // Compute is there. Still need explicit barriers beyond what
        // beginRenderPass and bindStorageImage already handle. That's phase 3 
        // stuff and isn't needed right now. Maybe we should start uploading photos 
        // of the sketchbook as some sort of roadmap.
    };
}
