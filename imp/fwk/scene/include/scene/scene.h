#pragma once
#include <core/fs/vfs.h>
#include <core/math/math.h>
#include <core/types/int_types.h>
#include <ecs/light_storage.h>
#include <ecs/world.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace imp::fwk
{
	struct SceneEntity
	{
		int id = -1;
		int parent = -1;
		std::string name;

		math::Vec3f localPosition;
		math::Quaternionf localRotation;
		math::Vec3f localScale = math::Vec3f::one();

		std::optional<std::string> renderableModelPath;
		bool renderableVisible = true;

		std::optional<ecs::LightType> lightKind;
		math::Vec3f lightColour;
		float lightIntensity = 0.f;
	};

	class Scene
	{
	public:
		using ModelPathResolver = std::function<std::string(ecs::ModelHandle)>;
		using ModelLoader = std::function<ecs::ModelHandle(const std::string&)>;

		std::vector<SceneEntity> entities;

		[[nodiscard]] static Scene fromWorld(const ecs::World& world, const ModelPathResolver& resolveModelPath = {});
		void applyToWorld(ecs::World& world, const ModelLoader& loadModel = {}) const;

		[[nodiscard]] std::string toJson() const;
		[[nodiscard]] static std::optional<Scene> fromJson(const std::string& json);

		bool saveToFile(const std::filesystem::path& path) const;
		[[nodiscard]] static std::optional<Scene> loadFromFile(const std::filesystem::path& path);

		bool saveToFile(const fs::VirtualFileSystem& vfs, const std::string& virtualPath) const;
		[[nodiscard]] static std::optional<Scene> loadFromFile(const fs::VirtualFileSystem& vfs, const std::string& virtualPath);
	};
}
