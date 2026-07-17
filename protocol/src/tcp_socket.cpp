#include "protocol/tcp_socket.h"
#include "protocol/tcp_helpers.h"

namespace imp::protocol
{
    TCPSocket::TCPSocket() : m_handle(kInvalidSocket)
    {
        hidden::initialiseNetworkStack();
    }

    TCPSocket::~TCPSocket()
    {
        close();
    }

    bool TCPSocket::connect(const char* address, u16 port)
    {
        close();

        SocketType sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == kInvalidSocket)
            return false;

        m_handle = static_cast<u64>(sock);

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);

        if (::inet_pton(AF_INET, address, &serverAddr.sin_addr) <= 0)
        {
            close();
            return false;
        }

        if (::connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0)
        {
            close();
            return false;
        }

        return true;
    }

    bool TCPSocket::send(const std::span<const u8> data)
    {
        if (!isValid()) return false;

        if (m_sendBuffer.size() + data.size() > kMaxSendBufferBytes)
        {
            close();
            return false;
        }

        m_sendBuffer.insert(m_sendBuffer.end(), data.begin(), data.end());
        return flush();
    }

    bool TCPSocket::flush()
    {
        if (!isValid()) return m_sendBuffer.empty();

        size_t sent = 0;
        const size_t pending = m_sendBuffer.size();

        while (sent < pending)
        {
            int flags = 0;
#ifdef __linux__
            flags |= MSG_NOSIGNAL;
#endif
            const auto result = ::send(
                static_cast<SocketType>(m_handle),
                reinterpret_cast<const char*>(m_sendBuffer.data() + sent),
                static_cast<int>(pending - sent),
                flags
            );

            if (result < 0)
            {
                if (hidden::wouldBlock())
                    break; // stop for now; remainder stays buffered for next flush()

                close(); // any other error is fatal for this connection
                return false;
            }

            sent += static_cast<size_t>(result);
        }

        if (sent > 0)
            m_sendBuffer.erase(m_sendBuffer.begin(), m_sendBuffer.begin() + static_cast<long>(sent));

        return true;
    }

    bool TCPSocket::recv(std::vector<u8> &outBuffer)
    {
        if (!isValid()) return false;
        u8 tempBuffer[4096];

        while (true)
        {
            const auto result = ::recv(
                static_cast<SocketType>(m_handle),
                reinterpret_cast<char*>(tempBuffer),
                sizeof(tempBuffer),
                0
            );

            if (result > 0)
            {
                outBuffer.insert(outBuffer.end(), tempBuffer, tempBuffer + result);
            }
            else if (result == 0)
            {
                // client gracefully closed the connection
                return false;
            }
            else
            {
                if (hidden::wouldBlock())
                {
                    return true;
                }
                return false;
            }
        }
    }

    void TCPSocket::close()
    {
        if (isValid())
        {
#ifdef _WIN32
            ::closesocket(static_cast<SocketType>(m_handle));
#else
            ::close(static_cast<SocketType>(m_handle));
#endif

            m_handle = kInvalidSocket;
        }
    }

    bool TCPSocket::isValid() const
    {
        return m_handle != kInvalidSocket;
    }

    void TCPSocket::setNonBlocking(const bool nonBlocking) const
    {
        hidden::setSocketNonBlocking(m_handle, nonBlocking);
    }

}
