#pragma once
#include <vector>
#include <core/types/int_types.h>
#include <core/math/math.h>

namespace imp::gfx
{
    enum class BufferUsage : u32
    {
        Vertex = 1u << 0,
        Index = 1u << 1,
        Uniform = 1u << 2,
        Storage = 1u << 3,
        AccelStructBuildInput = 1u << 4,
    };

    inline BufferUsage operator|(BufferUsage a, BufferUsage b)
    {
        return static_cast<BufferUsage>(static_cast<u32>(a) | static_cast<u32>(b));
    }
    inline bool hasFlag(BufferUsage value, BufferUsage flag)
    {
        return (static_cast<u32>(value) & static_cast<u32>(flag)) != 0;
    }

    enum class MemoryAccess
    {
        DeviceOnly,
        HostVisible,
    };

    enum class IndexFormat : u8 { Uint16, Uint32 };

    struct BufferDesc
    {
        u64 size = 0;
        BufferUsage usage = BufferUsage::Vertex;
        MemoryAccess memoryAccess = MemoryAccess::DeviceOnly;
        IndexFormat indexFormat = IndexFormat::Uint16;
        const char* debugName = nullptr;
    };

    class IBuffer
    {
    public:
        virtual ~IBuffer() = default;
        [[nodiscard]] virtual u64 size() const = 0;

        virtual void* mappedData() = 0;
        [[nodiscard]] virtual const void* mappedData() const = 0;

        [[nodiscard]] virtual IndexFormat indexFormat() const = 0;

        virtual bool update(const void* data, u64 size, u64 offset) = 0;
        [[nodiscard]] virtual u64 deviceAddress() const { return 0; }
    };

    class IBlas
    {
    public:
        virtual ~IBlas() = default;
        [[nodiscard]] virtual u64 deviceAddress() const = 0;
    };

    struct BlasBuildDesc
    {
        IBuffer* vertexBuffer = nullptr;
        u32 vertexCount = 0;
        u32 vertexStride = 0;

        IBuffer* indexBuffer = nullptr;
        u32 indexCount = 0;
        IndexFormat indexFormat = IndexFormat::Uint16;

        const char* debugName = nullptr;
    };

    class ITlas
    {
    public:
        virtual ~ITlas() = default;
        [[nodiscard]] virtual u64 deviceAddress() const = 0;
    };

    struct TlasInstanceDesc
    {
        const IBlas* blas = nullptr;
        math::Mat4f transformWS = math::Mat4f::identity();
        u32 customIndex = 0;
    };

    struct TlasBuildDesc
    {
        std::vector<TlasInstanceDesc> instances;
        const char* debugName = nullptr;
    };

    enum class TextureFormat
    {
        Unknown,
        RGBA8Unorm,
        RGBA8Srgb,
        BGRA8Srgb,
        RGBA16Float,
        Depth32Float,
        RG16Float,
    };

    enum class TextureUsage : u32
    {
        Sampled = 1u << 0, // readable in a shader via a sampler
        RenderTarget = 1u << 1, // writable as a colour attachment
        DepthStencil = 1u << 2, // writable as a depth/stencil attachment
        TransferSrc = 1u << 3, // readable back to CPU
        Storage = 1u << 4, // writes them directly, no rasterization involved
    };

    inline TextureUsage operator|(TextureUsage a, TextureUsage b)
    {
        return static_cast<TextureUsage>(static_cast<u32>(a) | static_cast<u32>(b));
    }

    inline bool hasFlag(TextureUsage value, TextureUsage flag)
    {
        return ( static_cast<u32>( value ) & static_cast<u32>( flag ) ) != 0;
    }

    enum class SampleCount : u32
    {
        One = 1,
        Two = 2,
        Four = 4,
        Eight = 8,
        Sixteen = 16,
    };

    struct TextureDesc
    {
        u32 width = 0;
        u32 height = 0;
        u32 mipLevels = 1;
        u32 arrayLayers = 1;
        TextureFormat format = TextureFormat::RGBA8Unorm;
        TextureUsage usage = TextureUsage::Sampled;
        SampleCount sampleCount = SampleCount::One;
        const char* debugName = nullptr;
        const void* initialData = nullptr;
    };

    class ITexture
    {
    public:
        virtual ~ITexture() = default;

        virtual u32 width() const = 0;
        virtual u32 height() const = 0;
        virtual TextureFormat format() const = 0;
    };

    enum class FilterMode { Nearest, Linear };
    enum class AddressMode { Repeat, ClampToEdge, MirroredRepeat };

    struct SamplerDesc
    {
        FilterMode minFilter = FilterMode::Linear;
        FilterMode magFilter = FilterMode::Linear;
        AddressMode addressModeU = AddressMode::Repeat;
        AddressMode addressModeV = AddressMode::Repeat;
        bool enableAnisotropy = false;
        float maxLod = 1000.f;
    };

    class ISampler
    {
    public:
        virtual ~ISampler() = default;
    };

    class IRenderTarget
    {
    public:
        virtual ~IRenderTarget() = default;
        virtual u32 width() const = 0;
        virtual u32 height() const = 0;
        virtual TextureFormat format() const = 0;
        virtual SampleCount sampleCount() const = 0;
        virtual ITexture* asTexture() { return nullptr; }
    };

    constexpr u32 mipLevelsForSize(u32 width, u32 height)
    {
        u32 levels = 1;
        u32 dim = width > height ? width : height;
        while (dim > 1) { dim >>= 1; ++levels; }
        return levels;
    }

}
