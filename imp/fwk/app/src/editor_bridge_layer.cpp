#include <app/editor_bridge_layer.h>

#include <protocol/tool_server.h>
#include <protocol/world_snapshot.h>
#include <protocol/entity_command.h>
#include <protocol/scene_command.h>
#include <protocol/asset_command.h>
#include <protocol/script_status.h>
#include <protocol/cvar_command.h>

#include <filesystem>

#include "core/config/cvar.h"

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

	void EditorBridgeLayer::onUpdate(float /*deltaSeconds*/)
	{
		drainCommands();

		const auto now = std::chrono::steady_clock::now();
		if (now - m_lastPublish < m_publishInterval)
			return;

		publishSnapshot();
		m_lastPublish = now;
	}

	void EditorBridgeLayer::onDetach()
	{
		protocol::ToolServer::instance().stop();
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
			else if (raw.type == protocol::MessageType::AssetCommand)
			{
				handleAssetCommand(raw.payload);
			}
			else if (raw.type == protocol::MessageType::ScriptStatus)
			{
				handleScriptStatus(raw.payload);
			}
			else if (raw.type == protocol::MessageType::CVarCommand)
			{
				handleCVarCommand(raw.payload);
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

		protocol::EntityCommandResultPayload result;
		result.op = cmd.op;
		result.targetIndex = cmd.targetIndex;
		result.targetGeneration = cmd.targetGeneration;

		if (cmd.op == protocol::EntityCommandOp::Create)
		{
			handleCreateEntityCommand(cmd, result);
		}
		else
		{
			const ecs::EntityId target{ cmd.targetIndex, cmd.targetGeneration };

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
				case protocol::EntityCommandOp::Create:
						// This is handled outside the switch because the entity
						// isn't alive when it is created.
						break;
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
		}

		if (server.hasSubscribers(protocol::MessageType::EntityCommandResult))
		{
			const auto bytes = protocol::serialiseEntityCommandResult(result);
			server.publish(protocol::MessageType::EntityCommandResult, bytes);
		}
	}

	void EditorBridgeLayer::handleCreateEntityCommand(const protocol::EntityCommandPayload &cmd, protocol::EntityCommandResultPayload &result)
	{
		if (cmd.stringA.empty())
		{
			result.success = false;
			result.error = "Empty model path.";
			return;
		}

		if (!m_modelLoader)
		{
			result.success = false;
			result.error = "No model loader configured.";
			return;
		}

		const ecs::ModelHandle model = m_modelLoader(cmd.stringA);
		if (!model.isValid())
		{
			result.success = false;
			result.error = "Failed to load model: " + cmd.stringA;
			return;
		}

		ecs::EntitySpawnDesc desc;
		desc.transform.position = cmd.vec3A;
		desc.transform.rotation = cmd.quatA;
		desc.transform.scale = ( cmd.vec3B == math::Vec3f::zero() ) ? math::Vec3f::one() : cmd.vec3B;
		desc.model = model;

		if (cmd.refIndex != 0xFFFFFFFFu)
		{
			const ecs::EntityId parent{ cmd.refIndex, cmd.refGeneration };
			if (!m_world.registry.isAlive(parent))
			{
				result.success = false;
				result.error = "Parent entity is not alive.";
				return;
			}
			if (!m_world.transforms.contains(parent))
			{
				result.success = false;
				result.error = "Parent has no Transform component.";
				return;
			}
			desc.parent = parent;
		}

		const auto slash = cmd.stringA.find_last_of('/');
		desc.name = ( slash == std::string::npos ) ? cmd.stringA : cmd.stringA.substr(slash + 1);

		const ecs::EntityId newEntity = m_world.spawnEntity(desc);

		result.success = true;
		result.targetIndex = newEntity.index;
		result.targetGeneration = newEntity.generation;
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

	void EditorBridgeLayer::handleAssetCommand(std::span<const u8> payload)
	{
		auto& server = protocol::ToolServer::instance();

		const auto decoded = protocol::deserialiseAssetCommand(payload);
		if (!decoded)
			return;

		const auto& cmd = *decoded;

		protocol::AssetCommandResultPayload result;
		result.op = cmd.op;
		result.path = cmd.path;

		if (cmd.op != protocol::AssetCommandOp::List && cmd.path.empty())
		{
			result.success = false;
			result.error = "Empty path";
		}
		else switch (cmd.op)
		{
		case protocol::AssetCommandOp::List:
		{
			const auto virtualFiles = m_vfs.listFiles(cmd.path, cmd.recursive);
			result.entries.reserve(virtualFiles.size());

			for (const auto& virtualFile : virtualFiles)
			{
				protocol::AssetEntryPayload entry;
				entry.virtualPath = virtualFile;
				entry.isDirectory = false;

				const auto physical = m_vfs.resolvePhysicalPath(virtualFile, false);
				if (!physical.empty())
				{
					std::error_code ec;
					const auto size = std::filesystem::file_size(physical, ec);
					if (!ec) entry.sizeBytes = size;
				}

				result.entries.push_back(std::move(entry));
			}

			result.success = true;
			break;
		}
		case protocol::AssetCommandOp::Read:
		{
			fs::Bytes data;
			if (m_vfs.readEntireFile(cmd.path, data))
			{
				result.content = std::move(data);
				result.success = true;
			}
			else
			{
				result.success = false;
				result.error = "Failed to read file";
			}
			break;
		}
		case protocol::AssetCommandOp::Write:
		{
			const fs::Bytes data(cmd.content.begin(), cmd.content.end());
			result.success = m_vfs.writeEntireFile(cmd.path, data);
			if (!result.success)
				result.error = "Failed to write file.";
			break;
		}
		case protocol::AssetCommandOp::Delete:
		{
			result.success = m_vfs.Delete(cmd.path);
			if (!result.success)
				result.error = "Failed to delete file.";
			break;
		}
		}

		if (server.hasSubscribers(protocol::MessageType::AssetCommandResult))
		{
			const auto bytes = protocol::serialiseAssetCommandResult(result);
			server.publish(protocol::MessageType::AssetCommandResult, bytes);
		}
	}

	void EditorBridgeLayer::handleScriptStatus(std::span<const u8> payload)
	{
		auto& server = protocol::ToolServer::instance();

		const auto decoded = protocol::deserialiseScriptStatus(payload);
		if (!decoded)
			return;

		const auto& cmd = *decoded;
		protocol::ScriptStatusPayload status;
		status.error = cmd.error;
		status.path = cmd.path;
		status.reloadedAtMs = cmd.reloadedAtMs;
		status.success = cmd.success;

		if (server.hasSubscribers(protocol::MessageType::ScriptStatus))
		{
			const auto bytes = protocol::serialiseScriptStatus(status);
			server.publish(protocol::MessageType::ScriptStatus, bytes);
		}
	}

	void EditorBridgeLayer::handleCVarCommand(std::span<const u8> payload)
	{
		auto& server = protocol::ToolServer::instance();

		const auto decoded = protocol::deserialiseCVarCommand(payload);
		if (!decoded)
			return;

		const auto& cmd = *decoded;
		auto& registry = CVarRegistry::get();

		protocol::CVarCommandResultPayload result;
		result.op = cmd.op;
		result.name = cmd.name;

		auto toPayloadEntry = [](const CVarSnapshot& s)
		{
			protocol::CVarEntryPayload e;
			e.name = s.name;
			e.type = static_cast<protocol::CVarType>(s.kind);
			e.floatValue = s.floatValue;
			e.intValue = s.intValue;
			e.boolValue = s.boolValue;
			return e;
		};

		switch (cmd.op)
		{
			case protocol::CVarCommandOp::List:
			{
				const auto snapshots = registry.list();
				result.entries.reserve(snapshots.size());
				for (const auto& s : snapshots)
					result.entries.push_back(toPayloadEntry(s));

				result.success = true;
				break;
			}
			case protocol::CVarCommandOp::Get:
			{
				if (cmd.name.empty())
				{
					result.success = false;
					result.error = "Empty name";
					break;
				}

				if (const auto snapshot = registry.get(cmd.name))
				{
					result.entry = toPayloadEntry(*snapshot);
					result.success = true;
				}
				else
				{
					result.success = false;
					result.error = "Unknown cvar.";
				}
				break;
			}
			case protocol::CVarCommandOp::Set:
			{
				if (cmd.name.empty())
				{
					result.success = false;
					result.error = "Empty name";
					break;
				}

				bool applied = false;
				switch (cmd.type)
				{
					case protocol::CVarType::Float: applied = registry.setFloat(cmd.name, cmd.floatValue); break;
					case protocol::CVarType::Int:   applied = registry.setInt(cmd.name, cmd.intValue); break;
					case protocol::CVarType::Bool:  applied = registry.setBool(cmd.name, cmd.boolValue); break;
				}

				if (applied)
				{
					if (const auto snapshot = registry.get(cmd.name))
						result.entry = toPayloadEntry(*snapshot);
					result.success = true;
				}
				else
				{
					result.success = false;
					result.error = "Unknown cvar, or type mismatch.";
				}
				break;
			}
		}

		if (server.hasSubscribers(protocol::MessageType::CVarCommandResult))
		{
			const auto bytes = protocol::serialiseCVarCommandResult(result);
			server.publish(protocol::MessageType::CVarCommandResult, bytes);
		}
	}

}
