#pragma once

#include <core/memory/int_types.h>
#include <core/math/math.h>

#include <memory>
#include <string>
#include <vector>

namespace imp::gfx
{
	class IBuffer;
	class ITexture;

	struct ModelVertex
	{
		math::Vec3f position;
		math::Vec3f normal;
		math::Vec2f uv;
	};

	struct MeshPrimitive
	{
		std::unique_ptr<IBuffer> vertexBuffer;
		std::unique_ptr<IBuffer> indexBuffer;
		u32 indexCount = 0;
		i32 materialIndex = -1;
	};

	struct Mesh
	{
		std::string name;
		std::vector<MeshPrimitive> primitives;
	};

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

		[[nodiscard]] bool isValid() const { return !meshes.empty(); }
	};
}
