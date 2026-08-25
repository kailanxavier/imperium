#include <app/editor_bridge_layer.h>

#include <protocol/tool_server.h>
#include <protocol/world_snapshot.h>
#include <protocol/entity_command.h>
#include <protocol/scene_command.h>

namespace imp::app
{
	EditorBridgeLayer::EditorBridgeLayer(
		ecs::World& world,
		fs::VirtualFileSystem& vfs,
		fwk::Scene::ModelPathResolver modelPathResolver,
		fwk::Scene::ModelLoader modelLoader,
		u16 toolServerPort,
		std::chrono::milliseconds publishInterval)
		: ILayer("EditorBridge")
		, m_world(world)
		, m_vfs(vfs)
		, m_modelPathResolver(std::move(modelPathResolver))
		, m_modelLoader(std::move(modelLoader))
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

			if (m_world.scripts.contains(id))
			{
				protocol::ScriptComponentPayload scriptPayload;
				scriptPayload.path = m_world.scripts.scriptPath(id);
				scriptPayload.wantsTick = m_world.scripts.wantsTick(id);
				p.script = scriptPayload;
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
			if (raw.type == protocol::MessageType::EntityCommand)
			{
				handleEntityCommand(raw.payload);
			}
			else if (raw.type == protocol::MessageType::SceneCommand)
			{
				handleSceneCommand(raw.payload);
			}
		}
	}

	void EditorBridgeLayer::handleEntityCommand(std::span<const u8> payload)
	{
		auto& server = protocol::ToolServer::instance();

		const auto decoded = protocol::deserialiseEntityCommand(payload);
		if (!decoded)
			return;

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
				if (!m_world.transforms.contains(target))
				{
					result.success = false;
					result.error = "Target has no Transform component.";
					break;
				}

				const bool unparenting = ( cmd.refIndex == 0xFFFFFFFFu );
				const ecs::EntityId newParent = unparenting
					? ecs::EntityId{}
				: ecs::EntityId{ cmd.refIndex, cmd.refGeneration };

				if (!unparenting && !m_world.registry.isAlive(newParent))
				{
					result.success = false;
					result.error = "New parent is not alive.";
					break;
				}

				if (!unparenting && !m_world.transforms.contains(newParent))
				{
					result.success = false;
					result.error = "New parent has no Transform component.";
					break;
				}

				if (!m_world.transforms.reparent(target, newParent))
				{
					result.success = false;
					result.error = "Reparent rejected. Are you trying to create a cycle or parent it to itself?";
				}

				break;
			}
			case protocol::EntityCommandOp::Destroy:
			{
				m_world.destroyEntity(target);
				break;
			}
			case protocol::EntityCommandOp::AttachScript:
			{
				if (cmd.stringA.empty())
				{
					if (m_world.scripts.contains(target))
						m_world.scripts.destroy(target);
				}
				else
				{
					if (m_world.scripts.contains(target))
					{
						m_world.scripts.setScriptPath(target, cmd.stringA);
						m_world.scripts.setWantsTick(target, cmd.boolA);
					}
					else
					{
						m_world.scripts.create(target, cmd.stringA, cmd.boolA);
					}
				}
				
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

	void EditorBridgeLayer::handleSceneCommand(std::span<const u8> payload)
	{
		auto& server = protocol::ToolServer::instance();

		const auto decoded = protocol::deserialiseSceneCommand(payload);
		if (!decoded)
			return;

		const auto& cmd = *decoded;

		protocol::SceneCommandResultPayload result;
		result.op = cmd.op;
		result.path = cmd.path;

		if (cmd.path.empty())
		{
			result.success = false;
			result.error = "Empty path";
		}
		else if (cmd.op == protocol::SceneCommandOp::Save)
		{
			const auto scene = fwk::Scene::fromWorld(m_world, m_modelPathResolver);
			result.success = scene.saveToFile(m_vfs, cmd.path);
			if (!result.success)
				result.error = "Failed to write scene file.";
		}
		else if (cmd.op == protocol::SceneCommandOp::Load)
		{
			if (auto scene = fwk::Scene::loadFromFile(m_vfs, cmd.path))
			{
				scene->applyToWorld(m_world, m_modelLoader);
				result.success = true;
			}
			else
			{
				result.success = false;
				result.error = "Failed to read or parse scene file";
			}
		}

		if (server.hasSubscribers(protocol::MessageType::SceneCommandResult))
		{
			const auto bytes = protocol::serialiseSceneCommandResult(result);
			server.publish(protocol::MessageType::SceneCommandResult, bytes);
		}
	}
}
