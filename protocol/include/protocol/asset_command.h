#pragma once
#include <core/types/int_types.h>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace imp::protocol
{
	enum class AssetCommandOp : u8
	{
		List = 0,
		Read,
		Write,
		Delete,
	};

	struct AssetEntryPayload
	{
		std::string virtualPath;
		bool isDirectory = false;
		u64 sizeBytes = 0;
	};

	struct AssetCommandPayload
	{
		AssetCommandOp op = AssetCommandOp::List;
		std::string path;
		bool recursive = false;
		std::vector<u8> content;
	};

	struct AssetCommandResultPayload
	{
		AssetCommandOp op = AssetCommandOp::List;
		std::string path;
		bool success = false;
		std::string error;
		std::vector<AssetEntryPayload> entries;
		std::vector<u8> content;
	};

	std::vector<u8> serialiseAssetCommand(const AssetCommandPayload& cmd);
	std::optional<AssetCommandPayload> deserialiseAssetCommand(std::span<const u8> payload);

	std::vector<u8> serialiseAssetCommandResult(const AssetCommandResultPayload& result);
	std::optional<AssetCommandResultPayload> deserialiseAssetCommandResult(std::span<const u8> payload);
}
