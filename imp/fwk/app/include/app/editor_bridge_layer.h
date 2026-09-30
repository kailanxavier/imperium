#pragma once

#include <app/iapp.h>
#include <fwk/layer.h>
#include <fwk/service_registry.h>
#include <core/types/int_types.h>
#include <core/fs/vfs.h>
#include <ecs/world.h>
#include <scene/scene.h>
#include <protocol/entity_command.h>
#include <chrono>
#include <span>

namespace imp::app
{
	class EditorBridgeLayer final : public fwk::ILayer
	{
	public:
		explicit EditorBridgeLayer(
			u16 toolServerPort = 47810, 
			std::chrono::milliseconds publishInterval = std::chrono::milliseconds(200));

		void onAttach(AppContext& ctx) override;
		void onUpdate(AppContext& ctx, float deltaSeconds) override;
		void onDetach(AppContext& ctx) override;

	private:
		void publishSnapshot();
		void drainCommands();

		void handleEntityCommand(std::span<const u8> payload);

		void handleCreateEntityCommand(const protocol::EntityCommandPayload& cmd,
			protocol::EntityCommandResultPayload& result);

		void handleSceneCommand(std::span<const u8> payload);
		void handleAssetCommand(std::span<const u8> payload);
		void handleScriptStatus(std::span<const u8> payload);
		void handleCVarCommand(std::span<const u8> payload);

		ecs::World* m_world = nullptr;
		fs::VirtualFileSystem* m_vfs = nullptr;
		fwk::ServiceRegistry* m_services = nullptr;
		fwk::Scene::ModelPathResolver m_modelPathResolver;
		fwk::Scene::ModelLoader m_modelLoader;
		u16 m_toolServerPort;
		std::chrono::milliseconds m_publishInterval;
		std::chrono::steady_clock::time_point m_lastPublish;
	};
}
