#include "protocol/tcp_helpers.h"
#include "protocol/tcp_listener.h"
#include "protocol/tcp_socket.h"

namespace imp::protocol
{
    TCPListener::TCPListener() :m_handle(kInvalidSocket)
    {
        hidden::initialiseNetworkStack();
    }

    TCPListener::~TCPListener()
    {
        if (m_handle != kInvalidSocket)
        {
#ifdef _WIN32
            ::closesocket(static_cast<SocketType>(m_handle));
#else
            ::close(static_cast<SocketType>(m_handle));
#endif
        }
    }

    bool TCPListener::bind(u16 port)
    {
        SocketType sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == kInvalidSocket) return false;
        m_handle = static_cast<u64>(sock);

        hidden::setSocketNonBlocking(m_handle, true);

        int reuse = 1;
        ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&reuse), sizeof(reuse));

        sockaddr_in hint{};
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        hint.sin_addr.s_addr = INADDR_ANY;

        if (::bind(sock, reinterpret_cast<sockaddr*>(&hint), sizeof(hint)) < 0)
            return false;

        return true;
    }

    bool TCPListener::listen() const
    {
        if (m_handle == kInvalidSocket) return false;
        return ::listen(static_cast<SocketType>(m_handle), SOMAXCONN) == 0;
    }

    TCPSocket TCPListener::accept() const
    {
        if (m_handle == kInvalidSocket) return TCPSocket{};

        sockaddr_in clientAddr{};
        LengthType clientSize = sizeof(clientAddr);

        const SocketType clientHandle = ::accept(
            static_cast<SocketType>(m_handle),
            reinterpret_cast<sockaddr*>(&clientAddr),
            &clientSize
        );

        if (clientHandle == kInvalidSocket)
            return TCPSocket{};

        TCPSocket clientSocket{static_cast<u64>(clientHandle)};
        clientSocket.setNonBlocking(true);
        return clientSocket;
    }
}
