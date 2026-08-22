#include "memv/memv_app.h"

#include <protocol/control.h>
#include "memory_generated.h"

#include <algorithm>
#include <cstdio>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#include <fwk/window.h>
#include <core/memory/allocator_types.h>


namespace imp::tools::memv
{
    namespace
    {
        const char* connectionStateLabel(tools::memv::ConnectionState s)
        {
            switch (s)
            {
            case tools::memv::ConnectionState::Disconnected: return "Disconnected";
            case tools::memv::ConnectionState::Connecting: return "Connecting...";
            case tools::memv::ConnectionState::Connected: return "Connected";
            case tools::memv::ConnectionState::Errored: return "Error";
            }
            return "Unknown";
        }

        ImVec4 connectionStateColor(tools::memv::ConnectionState s)
        {
            switch (s)
            {
            case tools::memv::ConnectionState::Connected: return {0.35f, 0.85f, 0.35f, 1.0f};
            case tools::memv::ConnectionState::Connecting: return {0.9f, 0.75f, 0.2f, 1.0f};
            case tools::memv::ConnectionState::Disconnected: return {0.7f, 0.7f, 0.7f, 1.0f};
            case tools::memv::ConnectionState::Errored: return {0.9f, 0.35f, 0.35f, 1.0f};
            }
            return {1, 1, 1, 1};
        }

        void drawAllocatorDetail(const tools::memv::AllocatorSnapshot &a)
        {
            ImGui::TextUnformatted(a.name.c_str());
            ImGui::Separator();

            ImGui::Text("Current: %s", tools::memv::formatBytes(a.currentUsed).c_str());
            ImGui::SameLine();
            ImGui::Text("  Peak: %s", tools::memv::formatBytes(a.peakUsed).c_str());
            ImGui::Text("Total allocated: %s", tools::memv::formatBytes(a.totalAllocated).c_str());
            ImGui::SameLine();
            ImGui::Text("  Total freed: %s", tools::memv::formatBytes(a.totalFreed).c_str());

            // History graph: reconstruct a chronological buffer for ImGui::PlotLines,
            // which expects oldest-first.
            static std::vector<float> plotBuf;
            plotBuf.resize(tools::memv::AllocatorSnapshot::kHistoryLen);
            for (size_t i = 0; i < plotBuf.size(); ++i)
                plotBuf[i] = a.historySample(plotBuf.size() - 1 - i);

            char overlay[64];
            std::snprintf(overlay, sizeof(overlay), "%s used", tools::memv::formatBytes(a.currentUsed).c_str());
            ImGui::PlotLines("##history", plotBuf.data(), static_cast<int>(plotBuf.size()), 0,
                overlay, 0.0f, FLT_MAX, ImVec2(-1, 100));

            ImGui::Spacing();
            ImGui::TextDisabled("Tag breakdown");

            // Sort tags by bytes descending so the biggest consumers are on top.
            struct TagRow
            {
                int tagId;
                u64 bytes;
            };
            std::vector<TagRow> rows;
            rows.reserve(a.tagBytes.size());
            for (size_t i = 0; i < a.tagBytes.size(); ++i)
                if (a.tagBytes[i] > 0)
                    rows.push_back({static_cast<int>(i), a.tagBytes[i]});

            std::sort(rows.begin(), rows.end(), [](const TagRow &l, const TagRow &r) { return l.bytes > r.bytes; });

            if (ImGui::BeginTable("tags", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 180)))
            {
                ImGui::TableSetupColumn("Tag", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Bytes", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                ImGui::TableSetupColumn("Share", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                ImGui::TableHeadersRow();

                for (const auto &row: rows)
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    const auto tag = static_cast<imp::memory::MemTag > ( row.tagId );
                    ImGui::TextUnformatted(std::string(imp::memory::toString(tag)).c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(tools::memv::formatBytes(row.bytes).c_str());

                    ImGui::TableSetColumnIndex(2);
                    const float frac = a.currentUsed > 0
                        ? static_cast<float>(row.bytes) / static_cast<float>(a.currentUsed)
                        : 0.0f;

                    ImGui::ProgressBar(frac, ImVec2(-1, 0));
                }

                ImGui::EndTable();
            }
        }
    }

    void MemoryViewerApp::drawUI()
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin("Memory Viewer", nullptr, flags);

        drawConnectionBar();
        ImGui::Spacing();

        ImGui::Text("Total tracked: %s across %d allocators",
            tools::memv::formatBytes(totalUsedBytes()).c_str(),
            static_cast<int>(allocators().size()));

        ImGui::Spacing();

        drawAllocatorTable();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const auto &alloc = allocators();
        if (m_ui.selectedAllocator >= 0 && m_ui.selectedAllocator<static_cast<int>(alloc.size()))
            drawAllocatorDetail(alloc[m_ui.selectedAllocator]);
        else
            ImGui::TextDisabled("Select an allocator above to see its tag breakdown and history.");

        ImGui::End();
    }

    void MemoryViewerApp::drawConnectionBar()
    {
        ImGui::SetNextItemWidth(180);
        ImGui::InputText("Host", m_ui.hostBuf, sizeof(m_ui.hostBuf));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90);
        ImGui::InputInt("Port", &m_ui.port);
        m_ui.port = std::clamp(m_ui.port, 1, 65535);

        ImGui::SameLine();
        const bool connected = state() == tools::memv::ConnectionState::Connected ||
            state() == tools::memv::ConnectionState::Connecting;

        if (connected)
        {
            if (ImGui::Button("Disconnect"))
                disconnect();
        }
        else
        {
            if (ImGui::Button("Connect"))
                connect(m_ui.hostBuf, static_cast<u16>(m_ui.port));
        }

        ImGui::SameLine();
        ImGui::TextColored(connectionStateColor(state()), "%s", connectionStateLabel(state()));

        if (state() == tools::memv::ConnectionState::Errored && !lastError().empty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", lastError().c_str());
        }
    }

    void MemoryViewerApp::drawAllocatorTable()
    {
        const auto &alloc = allocators();

        ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
            ImGuiTableFlags_ScrollY;

        if (!ImGui::BeginTable("allocators", 6, flags, ImVec2(0, 220)))
            return;

        ImGui::TableSetupColumn("Allocator", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Used", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Peak", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Allocs", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Frees", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Live", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (int i = 0; i < static_cast<int>(alloc.size()); ++i)
        {
            const auto &a = alloc[i];
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            const bool selected = (m_ui.selectedAllocator == i);
            if (ImGui::Selectable(a.name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
                m_ui.selectedAllocator = i;

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(tools::memv::formatBytes(a.currentUsed).c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(tools::memv::formatBytes(a.peakUsed).c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%u", a.allocationCount);

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%u", a.freeCount);

            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%u", a.allocationCount - a.freeCount);
        }

        ImGui::EndTable();
    }

    bool MemoryViewerApp::onInit(app::AppContext& ctx)
    {
        return true;
    }

    void MemoryViewerApp::onUpdate(app::AppContext& ctx, float deltaSeconds)
    {
        pollTelemetry();
    }

    void MemoryViewerApp::onRender(app::AppContext& ctx, gfx::ICommandList& cmd)
    {
        drawUI();
    }

    void MemoryViewerApp::onShutdown(app::AppContext& ctx)
    {
    }

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
        const auto *telemetry = memv::memory::GetMemoryTelemetry(frame.payload.data());
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
            snap.allocationCount = static_cast<u32>( alloc->allocation_count() );
            snap.freeCount = static_cast<u32>( alloc->free_count() );

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
