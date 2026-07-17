#pragma once
#include "int_types.h"

namespace imp::protocol
{
    class TCPSocket;
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
