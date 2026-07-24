#pragma once

#include <protocol/tcp_socket.h>
#include <protocol/frame.h>

#include <array>
#include <chrono>
#include <string>
#include <vector>

namespace imp::tools::memv
{
    std::string formatBytes(u64 bytes);

    struct AllocatorSnapshot
    {
        std::string name;

        u64 currentUsed = 0;
        u64 peakUsed = 0;
        u64 totalAllocated = 0;
        u64 totalFreed = 0;
        u32 allocationCount = 0;
        u32 freeCount = 0;

        std::vector<u64> tagBytes;

        static constexpr size_t kHistoryLen = 300; // 300 samples at 100ms = 30s window
        std::array<float, kHistoryLen> usedHistoryMB{};
        size_t historyWritePos = 0;
        size_t historyFilled = 0;

        void pushHistorySample(float usedMB);
        float historySample(size_t stepsAgo) const;
    };

    enum class ConnectionState
    {
        Disconnected,
        Connecting,
        Connected,
        Errored,
    };

    class MemoryViewerApp
    {
    public:
        void connect(const std::string& host, u16 port);
        void disconnect();

        void pollTelemetry();

        [[nodiscard]] ConnectionState state() const { return m_state; }
        [[nodiscard]] const std::string& lastError() const { return m_lastError; }
        [[nodiscard]] const std::string& host() const { return m_host; }
        [[nodiscard]] u16 port() const { return m_port; }

        [[nodiscard]] const std::vector<AllocatorSnapshot>& allocators() const { return m_allocators; }
        [[nodiscard]] u64 totalUsedBytes() const;

        [[nodiscard]] std::chrono::steady_clock::time_point lastSnapshotTime() const { return m_lastSnapshotTime; }

    private:
        void handleMemoryTelemetryFrame(const protocol::FrameReader::Frame& frame);
        void tickReconnect();
        void setState(ConnectionState state, std::string error = {});

        protocol::TCPSocket m_socket;
        protocol::FrameReader m_reader;
        std::vector<u8> m_recvChunk;

        std::string m_host;
        u16 m_port = 0;
        bool m_wantsConnection = false;

        ConnectionState m_state = ConnectionState::Disconnected;
        std::string m_lastError;

        std::chrono::steady_clock::time_point m_lastReconnectAttempt{};
        std::chrono::milliseconds m_reconnectInterval{1000};

        std::vector<AllocatorSnapshot> m_allocators;
        std::chrono::steady_clock::time_point m_lastSnapshotTime{};

        std::chrono::steady_clock::time_point m_lastHistorySample{};
        std::chrono::milliseconds m_historySampleInterval{100};
    };
}
