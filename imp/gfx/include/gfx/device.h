#pragma once

#include "resources.h"
#include "pipeline.h"
#include "commands.h"

#include <core/types/int_types.h>
#include <memory>
#include <vector>
#include <functional>

namespace imp::fwk { class Window; }
namespace imp::memory { class IAllocator; }
namespace imp::fs { class VirtualFileSystem; }

namespace imp::gfx
{
    enum class GraphicsApi { Vulkan, D3D12, D3D11 };

    struct DeviceDesc
    {
        fwk::Window* window = nullptr;
        const char* appName = "App";
        bool enableValidation = false;
        bool vsync = true;

        const fs::VirtualFileSystem* vfs = nullptr;
        memory::IAllocator* allocator = nullptr;
    };

    class IDevice
    {
    public:
        virtual ~IDevice() = default;

        virtual bool initialise(const DeviceDesc& desc) = 0;
        virtual void shutdown() = 0;

        virtual std::unique_ptr<IBuffer> createBuffer(const BufferDesc& desc) = 0;

        virtual std::unique_ptr<IBlas> createBlas(const BlasBuildDesc&) { return nullptr; }

        virtual std::unique_ptr<ITexture> createTexture(const TextureDesc& desc) = 0;
        virtual std::vector<std::unique_ptr<ITexture>> createTextures(const std::vector<gfx::TextureDesc>& descs) = 0;

        virtual std::unique_ptr<ISampler> createSampler(const SamplerDesc& desc) = 0;
        virtual std::unique_ptr<IShader> createShader(const ShaderDesc& desc) = 0;
        virtual std::unique_ptr<IPipeline> createPipeline(const PipelineDesc& desc) = 0;

        virtual std::unique_ptr<IPipeline> createComputePipeline(const ComputePipelineDesc&) { return nullptr; }

        virtual std::unique_ptr<IRenderTarget> createRenderTarget(const TextureDesc& desc) = 0;
        virtual std::vector<std::unique_ptr<IRenderTarget>> createCascadeRenderTargets(const TextureDesc& desc, ITexture** outArrayTexture) = 0;

        virtual u32 currentFrameIndex() const = 0;

        virtual void waitIdle() = 0;

        virtual IRenderTarget& backBuffer() = 0;
        virtual IRenderTarget* depthBuffer() = 0;

        virtual ICommandList* beginFrame() = 0;
        virtual void endFrame() = 0;

        virtual bool initImGui() = 0;
        virtual void shutdownImGui() = 0;
        virtual void newImGuiFrame() = 0;
        virtual void renderImGui(ICommandList& cmd) = 0;

        virtual GraphicsApi api() const = 0;
        virtual const char* apiName() const = 0;

        virtual bool supportsRayTracing() const { return false; }

        virtual void deferredDestroy(std::function<void()> deleter) = 0;
        virtual bool readbackTexture(IRenderTarget& target, std::vector<u8>& outPixels) = 0;
    };

    std::unique_ptr<IDevice> createDevice(GraphicsApi api);
    bool isApiAvailable(GraphicsApi api);
    std::vector<GraphicsApi> availableApis();
    const char* toString(GraphicsApi api);
}
