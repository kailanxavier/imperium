#pragma once

#include "resources.h"
#include "pipeline.h"
#include "commands.h"

#include <core/memory/int_types.h>
#include <memory>
#include <vector>

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
        virtual std::unique_ptr<ITexture> createTexture(const TextureDesc& desc) = 0;
        virtual std::unique_ptr<ISampler> createSampler(const SamplerDesc& desc) = 0;
        virtual std::unique_ptr<IShader> createShader(const ShaderDesc& desc) = 0;
        virtual std::unique_ptr<IPipeline> createPipeline(const PipelineDesc& desc) = 0;

        virtual IRenderTarget& backBuffer() = 0;
        virtual IRenderTarget* depthBuffer() = 0;

        virtual ICommandList* beginFrame() = 0;
        virtual void endFrame() = 0;

        virtual GraphicsApi api() const = 0;
        virtual const char* apiName() const = 0;
    };

    std::unique_ptr<IDevice> createDevice(GraphicsApi api);
    bool isApiAvailable(GraphicsApi api);
    std::vector<GraphicsApi> availableApis();
    const char* toString(GraphicsApi api);
}
