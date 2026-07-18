#pragma once

#include <vector>
#include "int_types.h"
#include <span>

namespace imp::protocol
{
    class TCPSocket
    {
    public:
        TCPSocket();
        ~TCPSocket();

        TCPSocket(const TCPSocket&) = delete;
        TCPSocket& operator=(const TCPSocket&) = delete;

        TCPSocket(TCPSocket&& other) noexcept;
        TCPSocket& operator=(TCPSocket&& other) noexcept;

        // Used by tools to connect to engine
        bool connect(const char* address, u16 port);

        // Non-blocking send & receive
        [[nodiscard]] bool send(std::span<const u8> data);
        bool flush();

        [[nodiscard]] size_t pendingSendBytes() const { return m_sendBuffer.size(); }

        bool recv(std::vector<u8>& outBuffer);

        void close();
        [[nodiscard]] bool isValid() const;

        // Set OS socket to non blocking mode
        void setNonBlocking(bool nonBlocking) const;

        static constexpr size_t kMaxSendBufferBytes = 4 * 1024 * 1024; // 4 MiB backlog cap

    private:
        friend class TCPListener;
        explicit TCPSocket(u64 handle) : m_handle(handle) {}

        u64 m_handle;
        std::vector<u8> m_sendBuffer;
    };
}
