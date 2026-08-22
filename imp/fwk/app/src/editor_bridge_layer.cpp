#include <app/editor_bridge_layer.h>

#include <protocol/tool_server.h>
#include <protocol/world_snapshot.h>
#include <protocol/entity_command.h>

namespace imp::app
{
	EditorBridgeLayer::EditorBridgeLayer(
		ecs::World& world, u16 toolServerPort, std::chrono::milliseconds publishInterval)
		: ILayer("EditorBridge")
		, m_world(world)
		, m_toolServerPort(toolServerPort)
		, m_publishInterval(publishInterval)
		, m_lastPublish(std::chrono::steady_clock::now())
	{
	}

	void EditorBridgeLayer::onAttach()
	{
		protocol::ToolServer::instance().start(m_toolServerPort);
	}

	void EditorBridgeLayer::onDetach()
	{
		protocol::ToolServer::instance().stop();
	}

	void EditorBridgeLayer::onUpdate(float /*deltaSeconds*/)
	{
		drainCommands();

		const auto now = std::chrono::steady_clock::now();
		if (now - m_lastPublish < m_publishInterval)
			return;

		publishSnapshot();
		m_lastPublish = now;
	}

	void EditorBridgeLayer::publishSnapshot()
	{
		auto& server = protocol::ToolServer::instance();
		if (!server.hasSubscribers(protocol::MessageType::WorldSnapshot))
			return;

		const auto& owners = m_world.transforms.m_owner;

		std::vector<protocol::EntitySnapshotPayload> entities;
		entities.reserve(owners.size());

		for (const auto& id : owners)
		{
			protocol::EntitySnapshotPayload p;
			p.index = id.index;
			p.generation = id.generation;

			const auto parent = m_world.transforms.parentOf(id);
			if (parent.isValid())
			{
				p.parentIndex = parent.index;
				p.parentGeneration = parent.generation;
			}

			if (m_world.names.contains(id))
				p.name = m_world.names.name(id);
			{
				protocol::TransformComponentPayload tp;
				const auto local = m_world.transforms.localTransform(id);
				tp.localPosition = local.position;
				tp.localRotation = local.rotation;
				tp.localScale = local.scale;
				p.transform = tp;
			}

			if (m_world.renderables.contains(id))
			{
				protocol::RenderableComponentPayload rp;
				const auto model = m_world.renderables.model(id);
				rp.modelIndex = model.index;
				rp.modelGeneration = model.generation;
				rp.visible = m_world.renderables.visible(id);
				p.renderable = rp;
			}

			if (m_world.lights.contains(id))
			{
				protocol::LightComponentPayload lp;
				lp.kind = static_cast<protocol::LightKindPayload>( m_world.lights.type(id) );
				lp.colour = m_world.lights.colour(id);
				lp.intensity = m_world.lights.intensity(id);
				p.light = lp;
			}

			entities.push_back(std::move(p));
		}

		const auto bytes = protocol::serialiseWorldSnapshot(entities);
		server.publish(protocol::MessageType::WorldSnapshot, bytes);
	}

	void EditorBridgeLayer::drainCommands()
	{
		auto& server = protocol::ToolServer::instance();
		protocol::ToolServer::InboundCommand raw;

		while (server.pollCommand(raw))
		{
			if (raw.type != protocol::MessageType::EntityCommand)
				continue;

			const auto decoded = protocol::deserialiseEntityCommand(raw.payload);
			if (!decoded)
				continue;

			const auto& cmd = *decoded;
			const ecs::EntityId target{ cmd.targetIndex, cmd.targetGeneration };

			protocol::EntityCommandResultPayload result;
			result.op = cmd.op;
			result.targetIndex = cmd.targetIndex;
			result.targetGeneration = cmd.targetGeneration;

			if (!m_world.registry.isAlive(target))
			{
				result.success = false;
				result.error = "Target entity is not alive.";
			}
			else
			{
				result.success = true;

				switch (cmd.op)
				{
				case protocol::EntityCommandOp::SetLocalTransform:
				{
					if (m_world.transforms.contains(target))
					{
						ecs::Transform t;
						t.position = cmd.vec3A;
						t.rotation = cmd.quatA;
						t.scale = cmd.vec3B;
						m_world.transforms.setLocalTransform(target, t);
					}
					else
					{
						result.success = false;
						result.error = "Target has no transform component.";
					}
					break;
				}
				case protocol::EntityCommandOp::SetName:
				{
					if (m_world.names.contains(target))
						m_world.names.setName(target, cmd.stringA);
					else
						m_world.names.create(target, cmd.stringA);
					break;
				}
				case protocol::EntityCommandOp::SetRenderableVisible:
				{
					if (m_world.renderables.contains(target))
					{
						m_world.renderables.setVisible(target, cmd.boolA);
					}
					else
					{
						result.success = false;
						result.error = "Target has no renderable component.";
					}
					break;
				}
				case protocol::EntityCommandOp::SetLightColour:
				{
					if (m_world.lights.contains(target))
					{
						m_world.lights.setColour(target, cmd.vec3A);
					}
					else
					{
						result.success = false;
						result.error = "Target has no light component.";
					}
					break;
				}
				case protocol::EntityCommandOp::SetLightIntensity:
				{
					if (m_world.lights.contains(target))
					{
						m_world.lights.setIntensity(target, cmd.floatA);
					}
					else
					{
						result.success = false;
						result.error = "Target has no light component.";
					}
					break;
				}
				case protocol::EntityCommandOp::Reparent:
				{
					result.success = false;
					result.error = "Reparenting isn't supported yet.";
					break;
				}
				case protocol::EntityCommandOp::Destroy:
				{
					m_world.destroyEntity(target);
					break;
				}
				}
			}

			if (server.hasSubscribers(protocol::MessageType::EntityCommandResult))
			{
				const auto bytes = protocol::serialiseEntityCommandResult(result);
				server.publish(protocol::MessageType::EntityCommandResult, bytes);
			}
		}
	}
}
