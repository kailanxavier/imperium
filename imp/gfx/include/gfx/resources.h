#pragma once
#include <core/types/int_types.h>

namespace imp::gfx
{
    enum class BufferUsage : u32
    {
        Vertex = 1u << 0,
        Index = 1u << 1,
        Uniform = 1u << 2,
        Storage = 1u << 3,
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
        virtual u64 size() const = 0;

        virtual void* mappedData() = 0;
        virtual const void* mappedData() const = 0;

        [[nodiscard]] virtual IndexFormat indexFormat() const = 0;
    };

    enum class TextureFormat
    {
        Unknown,
        RGBA8Unorm,
        RGBA8Srgb,
        BGRA8Srgb,
        RGBA16Float,
        Depth32Float,
    };

    enum class TextureUsage : u32
    {
        Sampled = 1u << 0, // readable in a shader via a sampler
        RenderTarget = 1u << 1, // writable as a colour attachment
        DepthStencil = 1u << 2, // writable as a depth/stencil attachment
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
        TextureFormat format = TextureFormat::RGBA8Unorm;
        TextureUsage usage = TextureUsage::Sampled;
        u32 mipLevels = 1;
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
