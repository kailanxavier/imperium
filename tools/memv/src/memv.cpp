#include "memv/memv_app.h"
#include "memory_generated.h"

#include <fwk/window.h>
#include <gfx/gfx.h>
#include <core/log/log.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#include "core/memory/allocator_types.h"

using namespace imp;

namespace
{
    struct UIState
    {
        char hostBuf[128] = "127.0.0.1";
        int port = 47810;
        int selectedAllocator = -1; // index into app.allocators()
    };

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

    void drawConnectionBar(tools::memv::MemoryViewerApp &app, UIState& ui)
    {
        ImGui::SetNextItemWidth(180);
        ImGui::InputText("Host", ui.hostBuf, sizeof(ui.hostBuf));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90);
        ImGui::InputInt("Port", &ui.port);
        ui.port = std::clamp(ui.port, 1, 65535);

        ImGui::SameLine();
        const bool connected = app.state() == tools::memv::ConnectionState::Connected ||
            app.state() == tools::memv::ConnectionState::Connecting;

        if (connected)
        {
            if (ImGui::Button("Disconnect"))
                app.disconnect();
        }
        else
        {
            if (ImGui::Button("Connect"))
                app.connect(ui.hostBuf, static_cast<u16>(ui.port));
        }

        ImGui::SameLine();
        ImGui::TextColored(connectionStateColor(app.state()), "%s", connectionStateLabel(app.state()));

        if (app.state() == tools::memv::ConnectionState::Errored && !app.lastError().empty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", app.lastError().c_str());
        }
    }

    void drawAllocatorTable(tools::memv::MemoryViewerApp& app, UIState& ui)
    {
        const auto &allocators = app.allocators();

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

        for (int i = 0; i < static_cast<int>(allocators.size()); ++i)
        {
            const auto &a = allocators[i];
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            const bool selected = (ui.selectedAllocator == i);
            if (ImGui::Selectable(a.name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
                ui.selectedAllocator = i;

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
                const auto tag = static_cast<memory::MemTag>(row.tagId);
                ImGui::TextUnformatted(std::string(memory::toString(tag)).c_str());

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

    void drawUI(tools::memv::MemoryViewerApp &app, UIState &ui)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin("Memory Viewer", nullptr, flags);

        drawConnectionBar(app, ui);
        ImGui::Spacing();

        ImGui::Text("Total tracked: %s across %d allocators",
            tools::memv::formatBytes(app.totalUsedBytes()).c_str(),
            static_cast<int>(app.allocators().size()));

        ImGui::Spacing();

        drawAllocatorTable(app, ui);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const auto &allocators = app.allocators();
        if (ui.selectedAllocator >= 0 && ui.selectedAllocator<static_cast<int>(allocators.size()))
            drawAllocatorDetail(allocators[ui.selectedAllocator]);
        else
            ImGui::TextDisabled("Select an allocator above to see its tag breakdown and history.");

        ImGui::End();
    }
}

int main(int argc, char** argv)
{
    log::Logger::get().initialise();

    fwk::Window window;
    fwk::WindowDesc windowDesc{};
    windowDesc.title  = "Memory Viewer";
    windowDesc.width  = 800;
    windowDesc.height = 800;

    if (!window.create(windowDesc))
    {
        LOG_ERROR("Memory Viewer", "Failed to create window");
        return 1;
    }

    std::unique_ptr<gfx::IDevice> gfx;
    for (gfx::GraphicsApi api : gfx::availableApis())
    {
        std::unique_ptr<gfx::IDevice> candidate = gfx::createDevice(api);
        if (!candidate)
            continue;

        gfx::DeviceDesc desc;
        desc.window = &window;
        desc.appName = "memv";
#ifndef NDEBUG
        desc.enableValidation = true;
#endif

        if (candidate->initialise(desc))
        {
            gfx = std::move(candidate);
            break;
        }
    }

    if (!gfx)
    {
        LOG_FATAL("Memory Viewer", "No suitable graphics backend could be found");
        window.destroy();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOther(window.getNativeHandle(), true);

    if (!gfx->initImGui())
    {
        std::fprintf(stderr, "memv: failed to initialise ImGui renderer for %s\n", gfx->apiName());
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        gfx->shutdown();
        window.destroy();
        return 1;
    }

    tools::memv::MemoryViewerApp app;
    UIState ui;
    ui.port = argc > 1 ? std::atoi(argv[1]) : 47810;
    app.connect(ui.hostBuf, static_cast<u16>(ui.port));

    while (!window.shouldClose())
    {
        window.pollEvents();
        if (window.isMinimised())
            continue;

        app.pollTelemetry();

        gfx::ICommandList* cmd = gfx->beginFrame();
        if (!cmd)
            continue;

        gfx->newImGuiFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        drawUI(app, ui);

        ImGui::Render();

        gfx::RenderPassDesc passDesc;
        passDesc.colourTarget = &gfx->backBuffer();
        passDesc.depthTarget = nullptr; // pure 2D UI, no depth needed
        passDesc.clearColourValue = { 0.023153f, 0.000911f, 0.004391f, 1.f };

        cmd->beginRenderPass(passDesc);
        gfx->renderImGui(*cmd);
        cmd->endRenderPass();

        gfx->endFrame();
    }

    gfx->shutdownImGui();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    gfx->shutdown();
    window.destroy();

    log::Logger::get().shutdown();
    return 0;
}
