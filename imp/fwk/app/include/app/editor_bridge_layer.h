#pragma once

#include <fwk/layer.h>
#include <core/types/int_types.h>
#include <core/fs/vfs.h>
#include <ecs/world.h>
#include <scene/scene.h>
#include <chrono>
#include <span>

namespace imp::app
{
	class EditorBridgeLayer final : public fwk::ILayer
	{
	public:
		explicit EditorBridgeLayer(
			ecs::World& world, 
			fs::VirtualFileSystem& vfs,
			fwk::Scene::ModelPathResolver modelPathResolver = {},
			fwk::Scene::ModelLoader modelLoader = {},
			u16 toolServerPort = 47810, 
			std::chrono::milliseconds publishInterval = std::chrono::milliseconds(200));

		void onAttach() override;
		void onUpdate(float deltaSeconds) override;
		void onDetach() override;


	private:
		void publishSnapshot();
		void drainCommands();

		void handleEntityCommand(std::span<const u8> payload);
		void handleSceneCommand(std::span<const u8> payload);
		void handleAssetCommand(std::span<const u8> payload);
		void handleScriptStatus(std::span<const u8> payload);

		ecs::World& m_world;
		fs::VirtualFileSystem& m_vfs;
		fwk::Scene::ModelPathResolver m_modelPathResolver;
		fwk::Scene::ModelLoader m_modelLoader;
		u16 m_toolServerPort;
		std::chrono::milliseconds m_publishInterval;
		std::chrono::steady_clock::time_point m_lastPublish;
	};
}
