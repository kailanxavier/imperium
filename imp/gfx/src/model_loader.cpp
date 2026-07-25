#include <gfx/model_loader.h>

#include <gfx/gfx.h>
#include <gfx/image.h>

#include <core/fs/vfs.h>
#include <core/log/log.h>
#include <core/memory/int_types.h>

#include <cgltf.h>

#include <cstring>
#include <fstream>
#include <functional>
#include <unordered_map>

namespace imp::gfx
{
	namespace
	{
		bool readFileBytes(const std::string& path, const fs::VirtualFileSystem* vfs, std::vector<u8>& outBytes)
		{
			if (vfs)
				return vfs->readEntireFile(path, outBytes);

			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if (!file.is_open())
				return false;

			std::streamsize size = file.tellg();
			if (size <= 0)
				return false;

			file.seekg(0, std::ios::beg);
			outBytes.resize(static_cast<size_t>( size ));
			return static_cast<bool>( file.read(reinterpret_cast<char*>( outBytes.data() ), size) );
		}

		std::string directoryOf(const std::string& path)
		{
			auto pos = path.find_last_of("/\\");
			return pos == std::string::npos ? std::string() : path.substr(0, pos + 1);
		}

		math::Mat4f toMat4(const float* m)
		{
			math::Mat4f result;
			std::memcpy(&result, m, sizeof(float) * 16);
			return result;
		}

		u32 readIndex(const cgltf_accessor* accessor, cgltf_size i)
		{
			return static_cast<u32>( cgltf_accessor_read_index(accessor, i) );
		}

		i32 getOrLoadTexture(IDevice& device, const cgltf_image* image, const std::string& modelDir,
			const fs::VirtualFileSystem* vfs, std::unordered_map<const cgltf_image*, i32>& cache, Model& outModel)
		{
			if (!image)
				return -1;

			if (auto it = cache.find(image); it != cache.end())
				return it->second;

			ImageData imageData;
			if (image->uri && std::strncmp(image->uri, "data:", 5) != 0)
			{
				std::string texPath = modelDir + image->uri;
				imageData = loadImageFromFile(texPath, vfs);
			}
			else if (image->buffer_view)
			{
				const cgltf_buffer_view* view = image->buffer_view;

				std::vector<u8> bytes(
					static_cast<const u8*>( view->buffer->data ) + view->offset,
					static_cast<const u8*>( view->buffer->data ) + view->offset + view->size
				);

				imageData = loadImageFromMemory(bytes);
			}
			else
			{
				LOG_WARN("Model Loader", "Skipping base64 data-URI image: {}. (unsupported)", image->name ? image->name : "<unnamed>");
				cache[image] = -1;
				return -1;
			}

			if (!imageData.isValid())
			{
				LOG_ERROR("Model Loader", "Failed to load glTF image: {}", image->uri ? image->uri : "<embedded>");
				cache[image] = -1;
				return -1;
			}

			TextureDesc texDesc;
			texDesc.width = imageData.width;
			texDesc.height = imageData.height;
			texDesc.format = TextureFormat::RGBA8Unorm;
			texDesc.usage = TextureUsage::Sampled;
			texDesc.initialData = imageData.pixels.data();

			ModelTexture modelTex;
			modelTex.texture = device.createTexture(texDesc);
			if (!modelTex.texture)
			{
				LOG_ERROR("Model Loader", "createTexture() failed for glTF image '{}'", image->uri ? image->uri : "<embedded>");
				cache[image] = -1;
				return -1;
			}

			outModel.textures.push_back(std::move(modelTex));
			i32 index = static_cast<i32>( outModel.textures.size() - 1 );
			cache[image] = index;
			return index;
		}

		bool loadPrimitive(IDevice& device, const cgltf_primitive& prim, const cgltf_data* data, MeshPrimitive& out)
		{
			if (prim.type != cgltf_primitive_type::cgltf_primitive_type_triangles)
			{
				LOG_WARN("Model Loader", "Skipping non triangle primitive");
				return false;
			}

			const cgltf_accessor* posAccessor = nullptr;
			const cgltf_accessor* normalAccessor = nullptr;
			const cgltf_accessor* uvAccessor = nullptr;

			for (cgltf_size i = 0; i < prim.attributes_count; ++i)
			{
				const cgltf_attribute& attr = prim.attributes[i];
				if (attr.type == cgltf_attribute_type_position) posAccessor = attr.data;
				else if (attr.type == cgltf_attribute_type_normal) normalAccessor = attr.data;
				else if (attr.type == cgltf_attribute_type_texcoord && !uvAccessor) uvAccessor = attr.data;
			}

			if (!posAccessor)
			{
				LOG_ERROR("Model Loader", "Primitive has no position attribute");
				return false;
			}

			const cgltf_size vertexCount = posAccessor->count;
			std::vector<ModelVertex> vertices(vertexCount);

			for (cgltf_size i = 0; i < vertexCount; ++i)
			{
				float pos[3] = { 0.f, 0.f, 0.f };
				cgltf_accessor_read_float(posAccessor, i, pos, 3);
				vertices[i].position = { pos[0], pos[1], pos[2] };

				if (normalAccessor)
				{
					float n[3] = { 0.f, 0.f, 1.f };
					cgltf_accessor_read_float(normalAccessor, i, n, 3);
					vertices[i].normal = { n[0], n[1], n[2] };
				}
				else
				{
					vertices[i].normal = { 0.f, 0.f, 0.f };
				}

				if (uvAccessor)
				{
					float uv[2] = { 0.f, 0.f };
					cgltf_accessor_read_float(uvAccessor, i, uv, 2);
					vertices[i].uv = { uv[0], uv[1] };
				}
				else
				{
					vertices[i].uv = { 0.f, 0.f };
				}
			}

			std::vector<u32> indices;
			if (prim.indices)
			{
				indices.resize(prim.indices->count);
				for (cgltf_size i = 0; i < prim.indices->count; ++i)
					indices[i] = readIndex(prim.indices, i);
			}
			else
			{
				indices.resize(vertexCount);
				for (cgltf_size i = 0; i < vertexCount; ++i)
					indices[i] = static_cast<u32>(i);
			}

			const bool needs32BitIndices = vertexCount > 0xFFFFu;
			const IndexFormat indexFormat = needs32BitIndices ? IndexFormat::Uint32 : IndexFormat::Uint16;

			BufferDesc vbDesc;
			vbDesc.size = vertices.size() * sizeof(ModelVertex);
			vbDesc.usage = BufferUsage::Vertex;
			vbDesc.memoryAccess = MemoryAccess::HostVisible;
			out.vertexBuffer = device.createBuffer(vbDesc);

			BufferDesc ibDesc;
			ibDesc.usage = BufferUsage::Index;
			ibDesc.memoryAccess = MemoryAccess::HostVisible;
			ibDesc.indexFormat = indexFormat;

			if (needs32BitIndices)
			{
				ibDesc.size = indices.size() * sizeof(u32);
				out.indexBuffer = device.createBuffer(ibDesc);
				if (out.vertexBuffer && out.indexBuffer)
				{
					std::memcpy(out.vertexBuffer->mappedData(), vertices.data(), static_cast<size_t>( vbDesc.size ));
					std::memcpy(out.indexBuffer->mappedData(), indices.data(), static_cast<size_t>( ibDesc.size ));
				}
			}
			else
			{
				std::vector<u16> indices16(indices.size());
				for (size_t i = 0; i < indices.size(); ++i)
					indices16[i] = static_cast<u16>(indices[i]);

				ibDesc.size = indices16.size() * sizeof(u16);
				out.indexBuffer = device.createBuffer(ibDesc);
				if (out.vertexBuffer && out.indexBuffer)
				{
					std::memcpy(out.vertexBuffer->mappedData(), vertices.data(), static_cast<size_t>(vbDesc.size));
					std::memcpy(out.indexBuffer->mappedData(), indices16.data(), static_cast<size_t>(ibDesc.size));
				}
			}

			if (!out.vertexBuffer || !out.indexBuffer)
			{
				LOG_ERROR("Model Loader", "Failed to create vertex/index buffer for primitive");
				return false;
			}

			out.indexCount = static_cast<u32>( indices.size() );
			out.materialIndex = prim.material ? static_cast<i32>( prim.material - data->materials ) : -1;

			return true;
		}

		void buildNodes(const cgltf_data* data, Model& outModel, std::unordered_map<const cgltf_node*, u32>& nodeIndexMap)
		{
			outModel.nodes.resize(data->nodes_count);

			for (cgltf_size i = 0; i < data->nodes_count; ++i)
				nodeIndexMap[&data->nodes[i]] = static_cast<u32>(i);

			for (cgltf_size i = 0; i < data->nodes_count; ++i)
			{
				const cgltf_node& srcNode = data->nodes[i];
				ModelNode& dstNode = outModel.nodes[i];
				dstNode.name = srcNode.name ? srcNode.name : "";

				float local[16];
				cgltf_node_transform_local(&srcNode, local);
				dstNode.localTransform = toMat4(local);

				if (srcNode.mesh)
					dstNode.meshIndex = static_cast<i32>( srcNode.mesh - data->meshes );

				dstNode.children.reserve(srcNode.children_count);
				for (cgltf_size c = 0; c < srcNode.children_count; ++c)
					dstNode.children.push_back(nodeIndexMap[srcNode.children[c]]);
			}
		}
	} // namespace

	Model loadModel(IDevice& device, const std::string& path, const fs::VirtualFileSystem* vfs)
	{
		Model outModel;

		std::vector<u8> fileBytes;
		if (!readFileBytes(path, vfs, fileBytes))
		{
			LOG_ERROR("Model Loader", "Failed to read glTF file: {}", path.c_str());
			return outModel;
		}

		cgltf_options options{};
		cgltf_data* data = nullptr;
		cgltf_result result = cgltf_parse(&options, fileBytes.data(), fileBytes.size(), &data);
		if (result != cgltf_result_success)
		{
			LOG_ERROR("Model Loader", "cgltf_parse failed for: {}. ({})", path.c_str(), static_cast<int>( result ));
			return outModel;
		}

		const std::string modelDir = directoryOf(path);
		result = cgltf_load_buffers(&options, data, path.c_str());
		if (result != cgltf_result_success)
		{
			LOG_ERROR("Model Loader", "cgltf_load_buffers failed for: {}. ({})", path.c_str(), static_cast<int>( result ));
			cgltf_free(data);
			return outModel;
		}

		if (cgltf_validate(data) != cgltf_result_success)
			LOG_WARN("Model Loader", "cgltf_validate reported issues for: {}; continuing anyway", path.c_str());

		std::unordered_map<const cgltf_image*, i32> textureCache;
		outModel.materials.resize(data->materials_count);
		for (cgltf_size i = 0; i < data->materials_count; ++i)
		{
			const cgltf_material& srcMat = data->materials[i];
			Material& dstMat = outModel.materials[i];
			dstMat.name = srcMat.name ? srcMat.name : "";

			if (srcMat.has_pbr_metallic_roughness)
			{
				const auto& pbr = srcMat.pbr_metallic_roughness;
				dstMat.baseColourFactor = {
					pbr.base_color_factor[0], pbr.base_color_factor[1],
					pbr.base_color_factor[2], pbr.base_color_factor[3]
				};

				if (pbr.base_color_texture.texture)
				{
					dstMat.baseColourTextureIndex = getOrLoadTexture(
						device, pbr.base_color_texture.texture->image, modelDir, vfs, textureCache, outModel);
				}

				if (pbr.metallic_roughness_texture.texture)
				{
					dstMat.metallicRoughnessTextureIndex = getOrLoadTexture(
						device, pbr.metallic_roughness_texture.texture->image, modelDir, vfs, textureCache, outModel);
				}
			}

			if (srcMat.occlusion_texture.texture)
			{
				dstMat.occlusionTextureIndex = getOrLoadTexture(
					device, srcMat.occlusion_texture.texture->image, modelDir, vfs, textureCache, outModel);
			}

			if (srcMat.emissive_texture.texture)
			{
				dstMat.emissiveTextureIndex = getOrLoadTexture(
					device, srcMat.emissive_texture.texture->image, modelDir, vfs, textureCache, outModel);
			}

			dstMat.emissiveFactor = {
				srcMat.emissive_factor[0], srcMat.emissive_factor[1], srcMat.emissive_factor[2], 0.f
			};
		}

		outModel.meshes.resize(data->meshes_count);
		for (cgltf_size i = 0; i < data->meshes_count; ++i)
		{
			const cgltf_mesh& srcMesh = data->meshes[i];
			Mesh& dstMesh = outModel.meshes[i];
			dstMesh.name = srcMesh.name ? srcMesh.name : "";

			dstMesh.primitives.reserve(srcMesh.primitives_count);
			for (cgltf_size p = 0; p < srcMesh.primitives_count; ++p)
			{
				MeshPrimitive primitive;
				if (loadPrimitive(device, srcMesh.primitives[p], data, primitive))
					dstMesh.primitives.push_back(std::move(primitive));
			}
		}

		std::unordered_map<const cgltf_node*, u32> nodeIndexMap;
		buildNodes(data, outModel, nodeIndexMap);

		if (data->scene)
		{
			outModel.rootNodes.reserve(data->scene->nodes_count);
			for (cgltf_size i = 0; i < data->scene->nodes_count; ++i)
				outModel.rootNodes.push_back(nodeIndexMap[data->scene->nodes[i]]);
		}

		cgltf_free(data);

		if (outModel.meshes.empty())
			LOG_WARN("Model Loader", "{} loaded with zero usable meshes", path.c_str());
		else
			LOG_INFO("Model Loader", "Loaded {}: {} mesh(es), {} material(s), {} texture(s), {} node(s)",
				path.c_str(), outModel.meshes.size(), outModel.materials.size(),
				outModel.textures.size(), outModel.nodes.size());

		return outModel;
	}
}
