#pragma once
#include <vector>
#include <span>
#include <optional>
#include "int_types.h"
#include "message_type.h"

namespace imp::protocol
{
    constexpr size_t kFrameHeaderSize = sizeof(u32) + sizeof(u16);
    constexpr size_t kMaxFramePayload = 16 * 1024 * 1024; // 16 MiB sanity cap

    std::vector<u8> encodeFrame(MessageType type, std::span<const u8> payload);

    class FrameReader
    {
    public:
        void append(std::span<const u8> data);
        struct Frame { MessageType type; std::vector<u8> payload; };
        [[nodiscard]] std::optional<Frame> tryExtract();

        [[nodiscard]] bool isPoisoned() const { return m_poisoned; }

    private:
        std::vector<u8> m_buffer;
        bool m_poisoned = false;
    };
}
