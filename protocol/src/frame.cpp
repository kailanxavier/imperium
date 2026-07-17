#include "protocol/frame.h"
#include <cstring>

#include "protocol/control.h"

namespace imp::protocol
{
    namespace
    {
        void putU32LE(std::vector<u8>& out, u32 v)
        {
            out.push_back(static_cast<u8>(v & 0xFF));
            out.push_back(static_cast<u8>((v >> 8) & 0xFF));
            out.push_back(static_cast<u8>((v >> 16) & 0xFF));
            out.push_back(static_cast<u8>((v >> 24) & 0xFF));
        }

        void putU16LE(std::vector<u8>& out, u16 v)
        {
            out.push_back(static_cast<u8>(v & 0xFF));
            out.push_back(static_cast<u8>((v >> 8) & 0xFF));
        }

        u32 getU32LE(const u8* p)
        {
            return static_cast<u32>(p[0]) | (static_cast<u32>(p[1]) << 8) |
                   (static_cast<u32>(p[2]) << 16) | (static_cast<u32>(p[3]) << 24);
        }

        u16 getU16LE(const u8* p)
        {
            return static_cast<u16>(p[0]) | static_cast<u16>(p[1] << 8);
        }
    }

    std::vector<u8> encodeFrame(MessageType type, std::span<const u8> payload)
    {
        std::vector<u8> out;
        out.reserve(kFrameHeaderSize + payload.size());
        putU32LE(out, static_cast<u32>(payload.size()));
        putU16LE(out, static_cast<u16>(type));
        out.insert(out.end(), payload.begin(), payload.end());
        return out;
    }

    void FrameReader::append(std::span<const u8> data)
    {
        if (m_poisoned) return;
        m_buffer.insert(m_buffer.end(), data.begin(), data.end());
    }

    std::optional<FrameReader::Frame> FrameReader::tryExtract()
    {
        if (m_poisoned || m_buffer.size() < kFrameHeaderSize)
            return std::nullopt;

        const u32 payloadSize = getU32LE(m_buffer.data());
        const u16 typeRaw     = getU16LE(m_buffer.data() + sizeof(u32));

        if (payloadSize > kMaxFramePayload)
        {
            m_poisoned = true; // corrupt or malicious frame (caller should drop the connection)
            return std::nullopt;
        }

        const size_t total = kFrameHeaderSize + payloadSize;
        if (m_buffer.size() < total)
            return std::nullopt; // wait for more bytes

        Frame frame;
        frame.type = static_cast<MessageType>(typeRaw);
        frame.payload.assign(m_buffer.begin() + kFrameHeaderSize, m_buffer.begin() + total);

        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + static_cast<long>(total));
        return frame;
    }
}
