#include "memv/memv_app.h"

#include <protocol/control.h>
#include "memory_generated.h"

#include <algorithm>
#include <cstdio>

namespace imp::tools::memv
{
    using namespace imp::protocol;
    std::string formatBytes(u64 bytes)
    {
        static const char* kUnits[] = { "B", "KB", "MB", "GB", "TB" };
        double value = static_cast<double>(bytes);
        int unit = 0;
        while (value >= 1024.0 && unit < 4)
        {
            value /= 1024.0;
            ++unit;
        }

        char buf[64];
        if (unit == 0)
            std::snprintf(buf, sizeof(buf), "%llu %s", static_cast<unsigned long long>(bytes), kUnits[unit]);
        else
            std::snprintf(buf, sizeof(buf), "%.2f %s", value, kUnits[unit]);

        return buf;
    }

    void AllocatorSnapshot::pushHistorySample(float usedMB)
    {
        usedHistoryMB[historyWritePos] = usedMB;
        historyWritePos = (historyWritePos + 1) % kHistoryLen;
        historyFilled = std::min(historyFilled + 1, kHistoryLen);
    }

    float AllocatorSnapshot::historySample(size_t stepsAgo) const
    {
        if (stepsAgo >= historyFilled)
            return 0.0f;

        const size_t idx = (historyWritePos + kHistoryLen - 1 - stepsAgo) % kHistoryLen;
        return usedHistoryMB[idx];
    }

    void MemoryViewerApp::setState(ConnectionState state, std::string error)
    {
        m_state = state;
        m_lastError = std::move(error);
    }

    void MemoryViewerApp::connect(const std::string &host, u16 port)
    {
        m_host = host;
        m_port = port;
        m_wantsConnection = true;

        m_socket = TCPSocket{};
        m_reader = FrameReader{};

        setState(ConnectionState::Connecting);

        if (!m_socket.connect(m_host.c_str(), m_port))
        {
            setState(ConnectionState::Errored, "failed to connect");
            m_lastReconnectAttempt = std::chrono::steady_clock::now();
            return;
        }
        m_socket.setNonBlocking(true);

        if (!m_socket.send(encodeFrame(MessageType::Control,
            encodeControl(ControlOp::Subscribe, MessageMask::MemoryTelemetry))))
        {
            setState(ConnectionState::Errored, "failed to send subscribe request");
            m_lastReconnectAttempt = std::chrono::steady_clock::now();
            return;
        }

        setState(ConnectionState::Connected);
    }

    void MemoryViewerApp::disconnect()
    {
        m_wantsConnection = false;
        m_socket = TCPSocket{};
        setState(ConnectionState::Disconnected);
    }

    void MemoryViewerApp::tickReconnect()
    {
        if (!m_wantsConnection) return;

        const auto now = std::chrono::steady_clock::now();
        if (now - m_lastReconnectAttempt < m_reconnectInterval) return;

        m_lastReconnectAttempt = now;
        connect(m_host, m_port); // cheap enough to just retry the entire handshake
    }

    void MemoryViewerApp::pollTelemetry()
    {
        if (m_state != ConnectionState::Connected)
        {
            tickReconnect();
            return;
        }

        m_recvChunk.clear();
        if (!m_socket.recv(m_recvChunk))
        {
            setState(ConnectionState::Disconnected, "engine disconnected");
            m_lastReconnectAttempt = std::chrono::steady_clock::now();
            return;
        }

        if (!m_recvChunk.empty())
            m_reader.append(m_recvChunk);

        while (auto frame = m_reader.tryExtract())
        {
            if (frame->type != MessageType::MemoryTelemetry)
                continue;

            handleMemoryTelemetryFrame(*frame);
        }

        if (m_reader.isPoisoned())
        {
            setState(ConnectionState::Errored, "malformed stream, dropping connection");
            m_socket = TCPSocket{};
            m_lastReconnectAttempt = std::chrono::steady_clock::now();
        }
    }

    void MemoryViewerApp::handleMemoryTelemetryFrame(const protocol::FrameReader::Frame& frame)
    {
        const auto *telemetry = memory::GetMemoryTelemetry(frame.payload.data());
        if (!telemetry || !telemetry->allocators())
            return;

        const auto now = std::chrono::steady_clock::now();
        const bool sampleHistory = (now - m_lastHistorySample) >= m_historySampleInterval;

        for (const auto *alloc: *telemetry->allocators())
        {
            const std::string name = alloc->allocator_name()->c_str();

            auto it = std::find_if(m_allocators.begin(), m_allocators.end(),
                                   [&](const AllocatorSnapshot &s) { return s.name == name; });
            if (it == m_allocators.end())
            {
                m_allocators.push_back(AllocatorSnapshot{});
                it = m_allocators.end() - 1;
                it->name = name;
            }

            AllocatorSnapshot &snap = *it;
            snap.currentUsed = alloc->current_used();
            snap.peakUsed = alloc->peak_used();
            snap.totalAllocated = alloc->total_allocated();
            snap.totalFreed = alloc->total_freed();
            snap.allocationCount = alloc->allocation_count();
            snap.freeCount = alloc->free_count();

            if (const auto *tagBytes = alloc->tag_bytes())
            {
                snap.tagBytes.assign(tagBytes->size(), 0);
                for (flatbuffers::uoffset_t i = 0; i < tagBytes->size(); ++i)
                    snap.tagBytes[i] = tagBytes->Get(i);
            }

            if (sampleHistory)
                snap.pushHistorySample(static_cast<float>(snap.currentUsed) / (1024.0f * 1024.0f));
        }

        if (sampleHistory)
            m_lastHistorySample = now;

        m_lastSnapshotTime = now;
    }

    u64 MemoryViewerApp::totalUsedBytes() const
    {
        u64 total = 0;
        for (const auto &a: m_allocators)
            total += a.currentUsed;
        return total;
    }

}
