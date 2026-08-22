#pragma once
#include <core/types/int_types.h>

namespace imp::protocol
{
    enum class MessageType : u16
    {
        Control = 0,
        MemoryTelemetry,
        ProfilerFrame,
        ConsoleCommand,
        ConsoleResponse,
        WorldSnapshot,
        EntityCommand,
        EntityCommandResult,
    };

    enum class MessageMask : u32
    {
        None = 0,
        MemoryTelemetry = 1u << 0,
        ProfilerFrame = 1u << 1,
        ConsoleCommand = 1u << 2,
        ConsoleResponse = 1u << 3,
        WorldSnapshot = 1u << 4,
        EntityCommand = 1u << 5,
        EntityCommandResult = 1u << 6,
    };

    [[nodiscard]] constexpr MessageMask maskFor(MessageType type)
    {
        return static_cast<MessageMask>(1u << (static_cast<u16>(type) - 1));
    }
}
