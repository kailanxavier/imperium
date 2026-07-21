#pragma once

#include "resources.h"
#include <core/memory/int_types.h>
#include <string>

namespace imp::gfx
{
    enum class ShaderStage
    {
        Vertex,
        Fragment,
        Compute,

        // Geometry,
        // Tesselation,
        // Mesh
    };

    struct ShaderDesc
    {
        ShaderStage stage = ShaderStage::Vertex;
        std::string path;
        const char* entryPoint = "main";
    };

    class IShader
    {
    public:
        virtual ~IShader() = default;
        virtual ShaderStage stage() const = 0;
    };

    enum class PrimitiveTopology { TriangleList, LineList, PointList };
    enum class CullMode { None, Front, Back };
    enum class CompareOp { Never, Less, Equal, LessOrEqual, Greater, NotEqual, GreaterOrEqual, Always };

    struct RasterizerStateDesc
    {
        PrimitiveTopology topology = PrimitiveTopology::TriangleList;
        CullMode cullMode = CullMode::None;
        bool wireframe = false;
    };

    struct DepthStencilStateDesc
    {
        bool depthTestEnable = false;
        bool depthWriteEnable = false;
        CompareOp depthCompareOp = CompareOp::Less;
        // no stencil here yet because nothing needs it
    };

    struct BlendStateDesc
    {
        bool blendEnable = false;
    };

    struct VertexAttribute
    {
        u32 location = 0;
        u32 offset = 0;
        u32 componentCount = 3; // 1..4
        bool isFloat = true;
    };

    struct VertexLayoutDesc
    {
        u32 stride = 0;
        const VertexAttribute* attributes = nullptr;
        u32 attributeCount = 0;
    };

    struct PipelineDesc
    {
        IShader* vertexShader = nullptr;
        IShader* fragmentShader = nullptr;

        VertexLayoutDesc vertexLayout;
        RasterizerStateDesc rasterizerState;
        DepthStencilStateDesc depthStencilState;
        BlendStateDesc blendState;

        TextureFormat colourFormat = TextureFormat::Unknown;
        TextureFormat depthFormat = TextureFormat::Unknown;

        u32 pushConstantSize = 0;

        bool hasUniformBuffer = false;
        bool hasTexture = false;
    };

    class IPipeline
    {
    public:
        virtual ~IPipeline() = default;
    };
}
