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
	};

	struct ModelTexture
	{
		std::unique_ptr<ITexture> texture;
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

		[[nodiscard]] bool isValid() const { return !meshes.empty(); }
	};
}
