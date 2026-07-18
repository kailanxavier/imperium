#pragma once
#include <optional>
#include <span>
#include <vector>
#include "int_types.h"
#include "message_type.h"

namespace imp::protocol
{
    enum class ControlOp : u8
    {
        Subscribe = 0,
        Unsubscribe,
        Hello,
    };

    struct ControlMessage
    {
        ControlOp op;
        MessageMask mask;
    };

    std::vector<u8> encodeControl(ControlOp op, MessageMask mask);

    // Returns nullopt if payload is wrong size
    std::optional<ControlMessage> decodeControl(std::span<const u8> payload);
}
