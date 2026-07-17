#include "protocol/control.h"

namespace imp::protocol
{
    std::vector<u8> encodeControl(ControlOp op, MessageMask mask)
    {
        std::vector<u8> out(5);
        out[0] = static_cast<u8>(op);
        const u32 m = static_cast<u32>(mask);
        out[1] = static_cast<u8>(m & 0xFF);
        out[2] = static_cast<u8>((m >> 8) & 0xFF);
        out[3] = static_cast<u8>((m >> 16) & 0xFF);
        out[4] = static_cast<u8>((m >> 24) & 0xFF);
        return out;
    }

    std::optional<ControlMessage> decodeControl(std::span<const u8> payload)
    {
        if (payload.size() < 5)
            return std::nullopt;

        ControlMessage msg{};
        msg.op = static_cast<ControlOp>(payload[0]);
        const u32 m = static_cast<u32>(payload[1]) | (static_cast<u32>(payload[2]) << 8) |
                      (static_cast<u32>(payload[3]) << 16) | (static_cast<u32>(payload[4]) << 24);
        msg.mask = static_cast<MessageMask>(m);
        return msg;
    }

}
