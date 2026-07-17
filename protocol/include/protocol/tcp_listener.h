#pragma once
#include "int_types.h"
#include "tcp_socket.h"

namespace imp::protocol
{
    class TCPListener
    {
    public:
        TCPListener();
        ~TCPListener();

        bool bind(u16 port);
        [[nodiscard]] bool listen() const;

        // Returns a valid TCPSocket if a new tool is connected, invalid otherwise
        [[nodiscard]] TCPSocket accept() const;

    private:
        u64 m_handle;
    };
}
