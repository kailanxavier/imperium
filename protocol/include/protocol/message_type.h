#pragma once
#include "int_types.h"

namespace imp::protocol
{
    enum class MessageType : u16
    {
        Control = 0,
        MemoryTelemetry,
        ProfilerFrame,
        ConsoleCommand,
        ConsoleResponse,
    };

    enum class MessageMask : u32
    {
        None = 0,
        MemoryTelemetry = 1u << 0,
        ProfilerFrame = 1u << 1,
        ConsoleCommand = 1u << 2,
        ConsoleResponse = 1u << 3,
    };

    [[nodiscard]] constexpr MessageMask maskFor(MessageType type)
    {
        return static_cast<MessageMask>(1u << (static_cast<u16>(type) - 1));
    }
}
