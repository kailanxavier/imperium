#include <protocol/asset_command.h>
#include "asset_command_generated.h"
#include <flatbuffers/flatbuffers.h>

namespace imp::protocol
{
	std::vector<u8> serialiseAssetCommand(const AssetCommandPayload& cmd)
	{
		flatbuffers::FlatBufferBuilder builder;

		const auto pathOffset = builder.CreateString(cmd.path);
		const auto contentOffset = builder.CreateVector(cmd.content);

		asset::AssetCommandBuilder cb(builder);
		cb.add_op(static_cast<asset::AssetCommandOp>( cmd.op ));
		cb.add_path(pathOffset);
		cb.add_recursive(cmd.recursive);
		cb.add_content(contentOffset);
		builder.Finish(cb.Finish());

		return { builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize() };
	}

	std::optional<AssetCommandPayload> deserialiseAssetCommand(std::span<const u8> payload)
	{
		flatbuffers::Verifier verifier(payload.data(), payload.size());
		if (!asset::VerifyAssetCommandBuffer(verifier))
			return std::nullopt;

		const auto* cmd = asset::GetAssetCommand(payload.data());
		if (!cmd)
			return std::nullopt;

		AssetCommandPayload p;
		p.op = static_cast<AssetCommandOp>( cmd->op() );
		if (cmd->path()) p.path = cmd->path()->str();
		p.recursive = cmd->recursive();
		if (const auto* content = cmd->content())
			p.content.assign(content->begin(), content->end());

		return p;
	}

	std::vector<u8> serialiseAssetCommandResult(const AssetCommandResultPayload& result)
	{
		flatbuffers::FlatBufferBuilder builder;

		const auto pathOffset = builder.CreateString(result.path);
		const auto errorOffset = builder.CreateString(result.error);
		const auto contentOffset = builder.CreateVector(result.content);

		std::vector<flatbuffers::Offset<asset::AssetEntry>> entryOffsets;
		entryOffsets.reserve(result.entries.size());
		for (const auto& e : result.entries)
		{
			const auto entryPathOffset = builder.CreateString(e.virtualPath);

			asset::AssetEntryBuilder eb(builder);
			eb.add_virtual_path(entryPathOffset);
			eb.add_is_directory(e.isDirectory);
			eb.add_size_bytes(e.sizeBytes);
			entryOffsets.push_back(eb.Finish());
		}
		const auto entriesOffset = builder.CreateVector(entryOffsets);

		asset::AssetCommandResultBuilder rb(builder);
		rb.add_op(static_cast<asset::AssetCommandOp>( result.op ));
		rb.add_path(pathOffset);
		rb.add_success(result.success);
		rb.add_error(errorOffset);
		rb.add_entries(entriesOffset);
		rb.add_content(contentOffset);
		builder.Finish(rb.Finish());

		return { builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize() };
	}

	std::optional<AssetCommandResultPayload> deserialiseAssetCommandResult(std::span<const u8> payload)
	{
		flatbuffers::Verifier verifier(payload.data(), payload.size());
		if (!verifier.VerifyBuffer<asset::AssetCommandResult>(nullptr))
			return std::nullopt;

		const auto* result = flatbuffers::GetRoot<asset::AssetCommandResult>(payload.data());
		if (!result)
			return std::nullopt;

		AssetCommandResultPayload p;
		p.op = static_cast<AssetCommandOp>( result->op() );
		if (result->path()) p.path = result->path()->str();
		p.success = result->success();
		if (result->error()) p.error = result->error()->str();

		if (const auto* entries = result->entries())
		{
			p.entries.reserve(entries->size());
			for (const auto* e : *entries)
			{
				if (!e) continue;

				AssetEntryPayload ep;
				if (e->virtual_path()) ep.virtualPath = e->virtual_path()->str();
				ep.isDirectory = e->is_directory();
				ep.sizeBytes = e->size_bytes();
				p.entries.push_back(std::move(ep));
			}
		}

		if (const auto* content = result->content())
			p.content.assign(content->begin(), content->end());

		return p;
	}

}
