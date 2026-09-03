#pragma once

#include <core/types/int_types.h>
#include <core/math/math.h>

#include <memory>
#include <string>
#include <vector>

namespace imp::gfx
{
	class IBuffer;
	class ITexture;
	class IBlas;

	struct ModelVertex
	{
		math::Vec3f position;
		math::Vec3f normal;
		math::Vec2f uv;
		math::Vec4f tangent;
	};

	struct MeshPrimitive
	{
		std::unique_ptr<IBuffer> vertexBuffer;
		std::unique_ptr<IBuffer> indexBuffer;
		u32 indexCount = 0;
		i32 materialIndex = -1;

		math::Vec3f boundsCentreLocal = math::Vec3f::zero();
		float boundsRadiusLocal = 0.f;

		std::unique_ptr<IBlas> blas;
	};

	struct Mesh
	{
		std::string name;
		std::vector<MeshPrimitive> primitives;
	};

	enum class AlphaMode { Opaque, Mask, Blend };
	enum class AlphaModePass { OpaqueAndMask, Blend };

	struct MaterialFactorsUBO
	{
		math::Vec4f baseColourFactor{ 1.f, 1.f, 1.f, 1.f };
		float metallicFactor = 1.f;
		float roughnessFactor = 1.f;
		float alphaCutoff = 0.5f;
		float alphaMode = 0.f; // 0 = opaque, 1 = mask, 2 = blend. why float not int? look below
	};
	static_assert( sizeof(MaterialFactorsUBO) == 32 && "MaterialFactorsUBO must stay std140 friendly" );

	struct Material
	{
		std::string name;
		math::Vec4f baseColourFactor{ 1.f, 1.f, 1.f, 1.f };
		i32 baseColourTextureIndex = -1;
		i32 normalTextureIndex = -1;
		i32 metallicRoughnessTextureIndex = -1;
		i32 occlusionTextureIndex = -1;
		i32 emissiveTextureIndex = -1;
		math::Vec4f emissiveFactor{ 0.f, 0.f, 0.f, 0.f };
		float metallicFactor = 1.f;
		float roughnessFactor = 1.f;
		AlphaMode alphaMode = AlphaMode::Opaque;
		float alphaCutoff = 0.5f;
		std::unique_ptr<IBuffer> factorsBuffer;
	};

	struct ModelTexture
	{
		std::shared_ptr<ITexture> texture;
	};

	struct ModelNode
	{
		std::string name;
		math::Mat4f localTransform;
		i32 meshIndex = -1;
		std::vector<u32> children;
	};

	struct Model
	{
		std::vector<Mesh> meshes;
		std::vector<Material> materials;
		std::vector<ModelTexture> textures;
		std::vector<ModelNode> nodes;
		std::vector<u32> rootNodes;

		i32 fallbackAlbedoTextureIndex = -1;
		i32 fallbackMetallicRoughnessTextureIndex = -1;
		i32 fallbackNormalTextureIndex = -1;
		i32 fallbackOcclusionTextureIndex = -1;

		std::unique_ptr<IBuffer> defaultMaterialFactorsBuffer;
		bool hasBlendPrimitives = false;

		[[nodiscard]] bool isValid() const { return !meshes.empty(); }
	};
}
