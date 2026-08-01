#include <gfx/model_loader.h>

#include <gfx/gfx.h>
#include <gfx/image.h>
#include <gfx/texture_cache.h>

#include <core/fs/vfs.h>
#include <core/log/log.h>
#include <core/types/int_types.h>

#include <jobs/job_system.h>
#include <chrono>

#include <cgltf.h>

#include <cstring>
#include <fstream>
#include <functional>
#include <map>
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

		struct TextureSlotRequest
		{
			const cgltf_image* image = nullptr;
			bool isSrgb = false;
			std::string cacheKey;
		};

		std::string resolveCacheKey(const cgltf_image& image, const std::string& modelDir, bool isSrgb)
		{
			if (image.uri && std::strncmp(image.uri, "data:", 5) != 0)
				return modelDir + image.uri + ( isSrgb ? "#srgb" : "#linear" );

			return {};
		}

		ImageData decodeImage(const cgltf_image& image, const std::string& modelDir, const fs::VirtualFileSystem* vfs)
		{
			if (image.uri && std::strncmp(image.uri, "data:", 5) != 0)
			{
				std::string texPath = modelDir + image.uri;
				return loadImageFromFile(texPath, vfs);
			}

			if (image.buffer_view)
			{
				const cgltf_buffer_view* view = image.buffer_view;

				std::vector<u8> bytes(
					static_cast<const u8*>( view->buffer->data ) + view->offset,
					static_cast<const u8*>( view->buffer->data ) + view->offset + view->size
				);

				return loadImageFromMemory(bytes);
			}

			LOG_WARN("Model Loader", "Skipping base64 data-URI image: {}. (unsupported)", image.name ? image.name : "<unnamed>");
			return {};
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

		struct MaterialTextureRefs
		{
			i64 baseColour = -1;
			i64 metallicRoughness = -1;
			i64 normal = -1;
			i64 occlusion = -1;
			i64 emissive = -1;
		};

	} // namespace

	Model loadModel(IDevice& device, const std::string& path, jobs::JobSystem& jobSystem,
		TextureCache& textureCache, const fs::VirtualFileSystem* vfs)
	{
		Model outModel;

		std::vector<u8> fileBytes;
		auto t0 = std::chrono::steady_clock::now();
		if (!readFileBytes(path, vfs, fileBytes))
		{
			LOG_ERROR("Model Loader", "Failed to read glTF file: {}", path.c_str());
			return outModel;
		}

		cgltf_options options{};
		cgltf_data* data = nullptr;
		auto t1 = std::chrono::steady_clock::now();
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

		std::vector<TextureSlotRequest> requests;
		std::map<std::pair<const cgltf_image*, bool>, size_t> requestIndexByImageAndSpace;

		auto getOrAddRequest = [&](const cgltf_image* image, bool isSrgb) -> i64
			{
				if (!image)
					return -1;

				const auto key = std::make_pair(image, isSrgb);
				if (auto it = requestIndexByImageAndSpace.find(key); it != requestIndexByImageAndSpace.end())
					return static_cast<i64>( it->second );

				TextureSlotRequest req;
				req.image = image;
				req.isSrgb = isSrgb;
				req.cacheKey = resolveCacheKey(*image, modelDir, isSrgb);

				requests.push_back(std::move(req));
				const size_t index = requests.size() - 1;
				requestIndexByImageAndSpace[key] = index;
				return static_cast<i64>( index );
			};

		std::vector<MaterialTextureRefs> materialRefs(data->materials_count);
		outModel.materials.resize(data->materials_count);

		for (cgltf_size i = 0; i < data->materials_count; ++i)
		{
			const cgltf_material& srcMat = data->materials[i];
			Material& dstMat = outModel.materials[i];
			MaterialTextureRefs& refs = materialRefs[i];
			dstMat.name = srcMat.name ? srcMat.name : "";

			if (srcMat.has_pbr_metallic_roughness)
			{
				const auto& pbr = srcMat.pbr_metallic_roughness;
				dstMat.baseColourFactor = {
					pbr.base_color_factor[0], pbr.base_color_factor[1],
					pbr.base_color_factor[2], pbr.base_color_factor[3]
				};
				dstMat.metallicFactor = pbr.metallic_factor;
				dstMat.roughnessFactor = pbr.roughness_factor;

				if (pbr.base_color_texture.texture)
					refs.baseColour = getOrAddRequest(pbr.base_color_texture.texture->image, /*isSrgb=*/true);

				if (pbr.metallic_roughness_texture.texture)
					refs.metallicRoughness = getOrAddRequest(pbr.metallic_roughness_texture.texture->image, /*isSrgb=*/false);
			}

			switch (srcMat.alpha_mode)
			{
			case cgltf_alpha_mode_mask: dstMat.alphaMode = AlphaMode::Mask; break;
			case cgltf_alpha_mode_blend: dstMat.alphaMode = AlphaMode::Blend; break;
			case cgltf_alpha_mode_opaque: dstMat.alphaMode = AlphaMode::Opaque; break;
			case cgltf_alpha_mode_max_enum: break;
			}
			dstMat.alphaCutoff = srcMat.alpha_cutoff;

			if (srcMat.normal_texture.texture)
				refs.normal = getOrAddRequest(srcMat.normal_texture.texture->image, /*isSrgb=*/false);

			if (srcMat.occlusion_texture.texture)
				refs.occlusion = getOrAddRequest(srcMat.occlusion_texture.texture->image, /*isSrgb=*/false);

			if (srcMat.emissive_texture.texture)
				refs.emissive = getOrAddRequest(srcMat.emissive_texture.texture->image, /*isSrgb=*/true);

			dstMat.emissiveFactor = {
				srcMat.emissive_factor[0], srcMat.emissive_factor[1], srcMat.emissive_factor[2], 0.f
			};
		}

		for (cgltf_size i = 0; i < data->materials_count; ++i)
		{
			Material& dstMat = outModel.materials[i];

			MaterialFactorsUBO factors;
			factors.baseColourFactor = dstMat.baseColourFactor;
			factors.metallicFactor = dstMat.metallicFactor;
			factors.roughnessFactor = dstMat.roughnessFactor;
			factors.alphaCutoff = dstMat.alphaCutoff;
			factors.alphaMode = static_cast<float>(static_cast<int>(dstMat.alphaMode));	// or in the lovely C: 
																						// (float)(*(int *)((char *)&dstMat 
																						//		+ offsetof(typeof(dstMat), alphaMode)));

			BufferDesc factorsDesc;
			factorsDesc.size = sizeof(MaterialFactorsUBO);
			factorsDesc.usage = BufferUsage::Uniform;
			factorsDesc.memoryAccess = MemoryAccess::HostVisible;
			dstMat.factorsBuffer = device.createBuffer(factorsDesc);

			if (dstMat.factorsBuffer)
				std::memcpy(dstMat.factorsBuffer->mappedData(), &factors, sizeof(factors));
			else
				LOG_ERROR("Model Loader", "Failed to create material factors buffer for material {}", dstMat.name.c_str());
		}

		std::vector<std::shared_ptr<ITexture>> resolvedTextures(requests.size());
		std::vector<size_t> pendingRequestIndices;
		pendingRequestIndices.reserve(requests.size());

		for (size_t i = 0; i < requests.size(); ++i)
		{
			if (!requests[i].cacheKey.empty())
			{
				if (auto cached = textureCache.find(requests[i].cacheKey))
				{
					resolvedTextures[i] = std::move(cached);
					continue;
				}
			}

			pendingRequestIndices.push_back(i);
		}

		std::vector<ImageData> decoded(pendingRequestIndices.size());
		auto t2 = std::chrono::steady_clock::now();
		jobSystem.parallelFor(static_cast<u32>( pendingRequestIndices.size() ), 1,
			[&](u32 start, u32 end)
			{
				for (u32 i = start; i < end; ++i)
				{
					const TextureSlotRequest& req = requests[pendingRequestIndices[i]];
					decoded[i] = decodeImage(*req.image, modelDir, vfs);
				}
			});

		std::vector<TextureDesc> uploadDescs;
		std::vector<size_t> uploadRequestIndices;
		uploadDescs.reserve(pendingRequestIndices.size());
		uploadRequestIndices.reserve(pendingRequestIndices.size());

		for (size_t i = 0; i < pendingRequestIndices.size(); ++i)
		{
			const TextureSlotRequest& req = requests[pendingRequestIndices[i]];
			if (!decoded[i].isValid())
			{
				LOG_ERROR("Model Loader", "Failed to decode glTF image: {}", req.image->uri ? req.image->uri : "<embedded>");
				continue;
			}

			TextureDesc desc;
			desc.width = decoded[i].width;
			desc.height = decoded[i].height;
			desc.format = req.isSrgb ? TextureFormat::RGBA8Srgb : TextureFormat::RGBA8Unorm;
			desc.usage = TextureUsage::Sampled;
			desc.initialData = decoded[i].pixels.data();

			uploadDescs.push_back(desc);
			uploadRequestIndices.push_back(pendingRequestIndices[i]);
		}

		auto t3 = std::chrono::steady_clock::now();
		if (!uploadDescs.empty())
		{
			std::vector<std::unique_ptr<ITexture>> uploaded = device.createTextures(uploadDescs);

			for (size_t i = 0; i < uploaded.size(); ++i)
			{
				const size_t requestIndex = uploadRequestIndices[i];
				if (!uploaded[i])
				{
					LOG_ERROR("Model Loader", "createTextures(): upload failed for {}",
						requests[requestIndex].image->uri ? requests[requestIndex].image->uri : "<embedded>");
					continue;
				}

				std::shared_ptr<ITexture> shared = std::move(uploaded[i]);
				const TextureSlotRequest& req = requests[requestIndex];

				if (!req.cacheKey.empty())
					shared = textureCache.insert(req.cacheKey, shared);

				resolvedTextures[requestIndex] = std::move(shared);
			}
		}

		std::vector<i32> modelTextureIndexForRequest(requests.size(), -1);
		for (size_t i = 0; i < requests.size(); ++i)
		{
			if (!resolvedTextures[i])
				continue; // decode or upload failed, material falls back to -1

			outModel.textures.push_back(ModelTexture{ resolvedTextures[i] });
			modelTextureIndexForRequest[i] = static_cast<i32>( outModel.textures.size() - 1 );
		}

		outModel.textures.push_back(ModelTexture{ textureCache.fallbackAlbedo() });
		outModel.fallbackAlbedoTextureIndex = static_cast<i32>( outModel.textures.size() - 1 );

		outModel.textures.push_back(ModelTexture{ textureCache.fallbackMetallicRoughness() });
		outModel.fallbackMetallicRoughnessTextureIndex = static_cast<i32>( outModel.textures.size() - 1 );

		outModel.textures.push_back(ModelTexture{ textureCache.fallbackNormal() });
		outModel.fallbackNormalTextureIndex = static_cast<i32>( outModel.textures.size() - 1 );

		outModel.textures.push_back(ModelTexture{ textureCache.fallbackOcclusion() });
		outModel.fallbackOcclusionTextureIndex = static_cast<i32>( outModel.textures.size() - 1 );

		{
			MaterialFactorsUBO defaultFactors;

			BufferDesc factorsDesc;
			factorsDesc.size = sizeof(MaterialFactorsUBO);
			factorsDesc.usage = BufferUsage::Uniform;
			factorsDesc.memoryAccess = MemoryAccess::HostVisible;
			outModel.defaultMaterialFactorsBuffer = device.createBuffer(factorsDesc);

			if (outModel.defaultMaterialFactorsBuffer)
				std::memcpy(outModel.defaultMaterialFactorsBuffer->mappedData(), &defaultFactors, sizeof(defaultFactors));
			else
				LOG_ERROR("Model Loader", "Failed to create default material factors buffer");
		}

		for (cgltf_size i = 0; i < data->materials_count; ++i)
		{
			Material& dstMat = outModel.materials[i];
			const MaterialTextureRefs& refs = materialRefs[i];

			if (refs.baseColour >= 0) dstMat.baseColourTextureIndex = modelTextureIndexForRequest[refs.baseColour];
			if (refs.metallicRoughness >= 0) dstMat.metallicRoughnessTextureIndex = modelTextureIndexForRequest[refs.metallicRoughness];
			if (refs.normal >= 0) dstMat.normalTextureIndex = modelTextureIndexForRequest[refs.normal];
			if (refs.occlusion >= 0) dstMat.occlusionTextureIndex = modelTextureIndexForRequest[refs.occlusion];
			if (refs.emissive >= 0) dstMat.emissiveTextureIndex = modelTextureIndexForRequest[refs.emissive];
		}

		auto t4 = std::chrono::steady_clock::now();

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
				{
					if (primitive.materialIndex >= 0 && outModel.materials[primitive.materialIndex].alphaMode == AlphaMode::Blend)
						outModel.hasBlendPrimitives = true;

					dstMesh.primitives.push_back(std::move(primitive));
				}
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

		const float read = std::chrono::duration<float, std::milli>(t1 - t0).count();
		const float parse = std::chrono::duration<float, std::milli>(t2 - t1).count();
		const float decode = std::chrono::duration<float, std::milli>(t3 - t2).count();
		const float upload = std::chrono::duration<float, std::milli>(t4 - t3).count();

		LOG_INFO("Model Loader", "read={}ms parse={}ms decode={}ms upload={}ms", read, parse, decode, upload);

		return outModel;
	}
}
