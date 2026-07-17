#pragma once
#include <span>
#include <vector>
#include <thread>
#include <atomic>
#include <deque>
#include <mutex>

#include "int_types.h"
#include "message_type.h"
#include "spsc_queue.h"

namespace imp::protocol
{
    class ToolServer
    {
    public:
        static ToolServer& instance();

        bool start(u16 port);
        void stop();

        // Engine thread side, called before doing any (possibly expensive)
        // serialisation work - skip entirely if nobody's listening
        [[nodiscard]] bool hasSubscribers(MessageType type) const;

        void publish(MessageType type, std::span<const u8> payload);

        struct InboundCommand { MessageType type; std::vector<u8> payload; };
        [[nodiscard]] bool pollCommand(InboundCommand& out);

        static constexpr size_t kTelemetryDepth = 4;

    private:
        ToolServer() = default;
        void networkThreadLoop(u16 port);

        static constexpr size_t kTypeCount = 4;
        static size_t typeIndex(MessageType type) { return static_cast<size_t>(type) - 1; }

        struct TelemetryChannel
        {
            std::mutex mutex;
            std::deque<std::vector<u8>> frames;
        };

        std::thread m_thread;
        std::atomic<bool> m_running{ false };
        std::atomic<bool> m_listening{ false };
        std::atomic<u32> m_subscriberCounts[kTypeCount]{};
        TelemetryChannel m_channels[kTypeCount];

        SPSCQueue<InboundCommand, 256> m_inboundCommands;
    };
}
