#include <scene/scene.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <unordered_map>

using namespace nlohmann;

namespace imp::fwk
{
	namespace
	{
		u64 entityKey(ecs::EntityId id)
		{
			return ( static_cast<u64>( id.index ) << 32 ) | id.generation;
		}

		json vec3ToJson(const math::Vec3f& v) { return { v.x, v.y, v.z }; }
		json quatToJson(const math::Quaternionf& q) { return { q.x, q.y, q.z, q.w }; }

		math::Vec3f vec3FromJson(const json& j, math::Vec3f fallback)
		{
			if (!j.is_array() || j.size() != 3)
				return fallback;
			return math::Vec3f(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
		}

		math::Quaternionf quatFromJson(const json& j, math::Quaternionf fallback)
		{
			if (!j.is_array() || j.size() != 4)
				return fallback;
			return math::Quaternionf(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>());
		}

		const char* lightKindToString(ecs::LightType kind)
		{
			return ( kind == ecs::LightType::Point ) ? "Point" : "Directional";
		}

		std::optional<ecs::LightType> lightKindFromString(const std::string& s)
		{
			if (s == "Point") return ecs::LightType::Point;
			if (s == "Directional") return ecs::LightType::Directional;
			return std::nullopt;
		}
	}

	Scene Scene::fromWorld(const ecs::World& world, const ModelPathResolver& resolveModelPath)
	{
		Scene scene;

		const auto& owners = world.transforms.m_owner;

		std::unordered_map<u64, int> idToLocal;
		idToLocal.reserve(owners.size());
		for (size_t i = 0; i < owners.size(); ++i)
			idToLocal.emplace(entityKey(owners[i]), static_cast<int>(i));

		scene.entities.reserve(owners.size());
		for (size_t i = 0; i < owners.size(); ++i)
		{
			const auto id = owners[i];
			SceneEntity e;
			e.id = static_cast<int>(i);

			for (const auto parent = world.transforms.parentOf(id); parent.isValid();)
			{
				const auto it = idToLocal.find(entityKey(parent));
				e.parent = ( it != idToLocal.end() ) ? it->second : -1;
			}

			if (world.names.contains(id))
				e.name = world.names.name(id);

			const auto local = world.transforms.localTransform(id);
			e.localPosition = local.position;
			e.localRotation = local.rotation;
			e.localScale = local.scale;

			if (world.renderables.contains(id) && resolveModelPath)
			{
				if (std::string path = resolveModelPath(world.renderables.model(id)); !path.empty())
				{
					e.renderableModelPath = std::move(path);
					e.renderableVisible = world.renderables.visible(id);
				}
			}

			if (world.lights.contains(id))
			{
				e.lightKind = world.lights.type(id);
				e.lightColour = world.lights.colour(id);
				e.lightIntensity = world.lights.intensity(id);
			}

			if (world.scripts.contains(id))
			{
				e.scriptPath = world.scripts.scriptPath(id);
				e.scriptWantsTick = world.scripts.wantsTick(id);
			}

			scene.entities.push_back(std::move(e));
		}
		
		return scene;
	}

	void Scene::applyToWorld(ecs::World& world, const ModelLoader& loadModel) const
	{
		const std::vector<ecs::EntityId> existing = world.transforms.m_owner;
		for (const auto& id : existing)
			world.destroyEntity(id);

		std::unordered_map<int, ecs::EntityId> localToId;
		localToId.reserve(entities.size());

		for (const auto& e : entities)
		{
			ecs::Transform t;
			t.position = e.localPosition;
			t.rotation = e.localRotation;
			t.scale = e.localScale;

			const auto id = world.createEntity();
			world.transforms.create(id, t);

			if (!e.name.empty())
				world.names.create(id, e.name);

			if (e.renderableModelPath && loadModel)
			{
				if (const auto handle = loadModel(*e.renderableModelPath); handle.isValid())
					world.renderables.create(id, handle, e.renderableVisible);
			}

			if (e.lightKind)
				world.lights.create(id, *e.lightKind, e.lightColour, e.lightIntensity);

			if (e.scriptPath)
				world.scripts.create(id, *e.scriptPath, e.scriptWantsTick);

			localToId.emplace(e.id, id);
		}

		// Second pass to parent all entities
		for (const auto& e : entities)
		{
			if (e.parent < 0)
				continue;

			const auto childIt = localToId.find(e.id);
			const auto parentIt = localToId.find(e.parent);
			if (childIt == localToId.end() || parentIt == localToId.end())
				continue; // malformed file, we skip instead of crashing

			world.transforms.reparent(childIt->second, parentIt->second);
		}
	}

	std::string Scene::toJson() const
	{
		json root;
		root["version"] = 1;

		auto& entitiesJson = root["entities"];
		entitiesJson = json::array();

		for (const auto& e : entities)
		{
			json ej;
			ej["id"] = e.id;
			ej["parent"] = e.parent;
			if (!e.name.empty())
				ej["name"] = e.name;

			ej["transform"] = {
				{ "position", vec3ToJson(e.localPosition) },
				{ "rotation", quatToJson(e.localRotation) },
				{ "scale", vec3ToJson(e.localScale) },
			};

			if (e.renderableModelPath)
			{
				ej["renderable"] = {
					{ "model", *e.renderableModelPath },
					{ "visible", e.renderableVisible },
				};
			}

			if (e.lightKind)
			{
				ej["light"] = {
					{ "kind", lightKindToString(*e.lightKind) },
					{ "colour", vec3ToJson(e.lightColour) },
					{ "intensity", e.lightIntensity },
				};
			}

			if (e.scriptPath)
			{
				ej["script"] = {
					{ "path", *e.scriptPath },
					{ "wantsTick", e.scriptWantsTick },
				};
			}

			entitiesJson.push_back(std::move(ej));
		}

		// Pretty print
		return root.dump(2);
	}

	std::optional<Scene> Scene::fromJson(const std::string& j)
	{
		json root;
		try
		{
			root = json::parse(j);
		}
		catch (const json::parse_error&)
		{
			return std::nullopt;
		}

		if (!root.contains("entities") || !root["entities"].is_array())
			return std::nullopt;

		Scene scene;

		for (const auto& ej : root["entities"])
		{
			SceneEntity e;
			e.id = ej.value("id", -1);
			e.parent = ej.value("parent", -1);
			e.name = ej.value("name", std::string{});

			if (ej.contains("transform"))
			{
				const auto& tj = ej["transform"];
				e.localPosition = tj.contains("position")
					? vec3FromJson(tj["position"], math::Vec3f::zero())
					: math::Vec3f::zero();
				e.localRotation = tj.contains("rotation")
					? quatFromJson(tj["rotation"], math::Quaternionf::identity())
					: math::Quaternionf::identity();
				e.localScale = tj.contains("scale")
					? vec3FromJson(tj["scale"], math::Vec3f::one())
					: math::Vec3f::one();
			}
			else
			{
				e.localScale = math::Vec3f::one();
			}

			if (ej.contains("renderable"))
			{
				const auto& rj = ej["renderable"];
				if (rj.contains("model") && rj["model"].is_string())
				{
					e.renderableModelPath = rj["model"].get<std::string>();
					e.renderableVisible = rj.value("visible", true);
				}
			}

			if (ej.contains("light"))
			{
				const auto& lj = ej["light"];
				e.lightKind = lightKindFromString(lj.value("kind", std::string{ "Directional" }));
				e.lightColour = lj.contains("colour")
					? vec3FromJson(lj["colour"], math::Vec3f::one())
					: math::Vec3f::one();
				e.lightIntensity = lj.value("intensity", 0.f);
			}

			if (ej.contains("script"))
			{
				const auto& sj = ej["script"];
				if (sj.contains("path") && sj["path"].is_string())
				{
					e.scriptPath = sj["path"].get<std::string>();
					e.scriptWantsTick = sj.value("wantsTick", false);
				}
			}

			if (e.id < 0)
				continue;

			scene.entities.push_back(std::move(e));
		}

		return scene;
	}

	bool Scene::saveToFile(const std::filesystem::path& path) const
	{
		std::ofstream file(path, std::ios::binary);
		if (!file.is_open())
			return false;

		const auto json = toJson();
		file.write(json.data(), static_cast<std::streamsize>( json.size() ));
		return file.good();
	}

	std::optional<Scene> Scene::loadFromFile(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open())
			return std::nullopt;

		std::ostringstream buffer;
		buffer << file.rdbuf();
		return fromJson(buffer.str());
	}

	bool Scene::saveToFile(const fs::VirtualFileSystem& vfs, const std::string& virtualPath) const
	{
		return vfs.writeEntireFileText(virtualPath, toJson());
	}

	std::optional<Scene> Scene::loadFromFile(const fs::VirtualFileSystem& vfs, const std::string& virtualPath)
	{
		std::string text;
		if (!vfs.readEntireFileText(virtualPath, text))
			return std::nullopt;

		return fromJson(text);
	}
}
