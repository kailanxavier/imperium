#pragma once

#include <fwk/layer.h>
#include <core/types/int_types.h>
#include <ecs/world.h>
#include <chrono>

#include <span>

namespace imp::app
{
	class EditorBridgeLayer final : public fwk::ILayer
	{
	public:
		explicit EditorBridgeLayer(ecs::World& world, u16 toolServerPort = 47810, 
			std::chrono::milliseconds publishInterval = std::chrono::milliseconds(200));

		void onAttach() override;
		void onDetach() override;

		void onUpdate(float deltaSeconds) override;

	private:
		void publishSnapshot();
		void drainCommands();
		void handleEntityCommand(std::span<const u8> payload);
		void handleSceneCommand(std::span<const u8> payload);

		ecs::World& m_world;
		u16 m_toolServerPort;
		std::chrono::milliseconds m_publishInterval;
		std::chrono::steady_clock::time_point m_lastPublish;
	};
}
