#pragma once
#include <core/types/int_types.h>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace imp::protocol
{
    enum class CVarType : u8
    {
        Float = 0,
        Int,
        Bool,
    };

    enum class CVarCommandOp : u8
    {
        List = 0,
        Get,
        Set,
    };

    struct CVarEntryPayload
    {
        std::string name;
        CVarType type = CVarType::Float;

        float floatValue = 0.f;
        i32 intValue = 0;
        bool boolValue = false;
    };

    struct CVarCommandPayload
    {
        CVarCommandOp op = CVarCommandOp::List;
        std::string name;

        CVarType type = CVarType::Float;
        float floatValue = 0.f;
        i32 intValue = 0;
        bool boolValue = false;
    };

    struct CVarCommandResultPayload
    {
        CVarCommandOp op = CVarCommandOp::List;
        std::string name;
        bool success = false;
        std::string error;

        std::vector<CVarEntryPayload> entries;
        std::optional<CVarEntryPayload> entry;
    };

    std::vector<u8> serialiseCVarCommand(const CVarCommandPayload& cmd);
    std::optional<CVarCommandPayload> deserialiseCVarCommand(std::span<const u8> payload);

    std::vector<u8> serialiseCVarCommandResult(const CVarCommandResultPayload& cmd);
    std::optional<CVarCommandResultPayload> deserialiseCVarCommandResult(std::span<const u8> payload);
}
