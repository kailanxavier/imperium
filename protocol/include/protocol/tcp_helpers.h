#pragma once
#include "int_types.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketType = SOCKET;
using LengthType = int;
constexpr u64 kInvalidSocket = INVALID_SOCKET;
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
using SocketType = int;
using LengthType = socklen_t;
constexpr u64 kInvalidSocket = static_cast<u64>(-1);
#endif

#include <cstring>

namespace imp::protocol::hidden
{
    inline bool initialiseNetworkStack()
    {
#ifdef _WIN32
        static bool initialised = false;
        if (!initialised)
        {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
                return false;

            initialised = true;
        }
#endif
        return true;
    }

    inline bool wouldBlock()
    {
#ifdef _WIN32
        return WSAGetLastError() == WSAEWOULDBLOCK;
# else
        return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
    }
}
