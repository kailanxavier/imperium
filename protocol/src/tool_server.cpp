#include "protocol/tool_server.h"
#include "protocol/tcp_listener.h"
#include "protocol/tcp_socket.h"
#include "protocol/frame.h"
#include "protocol/control.h"
#include <chrono>
#include <mutex>

namespace imp::protocol
{
    ToolServer& ToolServer::instance()
    {
        static ToolServer server;
        return server;
    }

    bool ToolServer::start(u16 port)
    {
        if (m_running.exchange(true))
            return false; // already running

        m_thread = std::thread([this, port] { networkThreadLoop(port); });
        return true;
    }

    void ToolServer::stop()
    {
        if (!m_running.exchange(false))
            return;

        if (m_thread.joinable())
            m_thread.join();
    }

    bool ToolServer::hasSubscribers(MessageType type) const
    {
        if (type == MessageType::Control) return false;
        return m_subscriberCounts[typeIndex(type)].load(std::memory_order_relaxed) > 0;
    }

    void ToolServer::publish(MessageType type, std::span<const u8> payload)
    {
        if (type == MessageType::Control) return;
        if (!hasSubscribers(type)) return; // caller should already check, but let's be safe here

        auto& channel = m_channels[typeIndex(type)];
        std::lock_guard lock(channel.mutex);

        if (channel.frames.size() >= kTelemetryDepth)
            channel.frames.pop_front(); // drop oldest

        channel.frames.emplace_back(payload.begin(), payload.end());
    }

    bool ToolServer::pollCommand(InboundCommand& out)
    {
        return m_inboundCommands.tryPop(out);
    }

    namespace
    {
        struct Connection
        {
            TCPSocket socket;
            FrameReader reader;
            MessageMask subscribed = MessageMask::None;
        };

        bool hasMask(MessageMask value, MessageMask bit)
        {
            return (static_cast<u32>(value) & static_cast<u32>(bit)) != 0;
        }
    }

    void ToolServer::networkThreadLoop(u16 port)
    {
        TCPListener listener;
        if (!listener.bind(port) || !listener.listen())
        {
            m_running.store(false);
            return;
        }

        std::vector<Connection> connections;
        std::vector<u8> recvScratch;

        while (m_running.load(std::memory_order_relaxed))
        {
            // --- accept new tools ---
            if (TCPSocket incoming = listener.accept(); incoming.isValid())
            {
                connections.push_back(Connection{std::move(incoming), FrameReader{}, MessageMask::None});
            }

            // read + process inbound frames
            for (auto it = connections.begin(); it != connections.end(); )
            {
                recvScratch.clear();
                const bool alive = it->socket.recv(recvScratch);
                if (!alive)
                {
                    // connection dropped - release its subscriptions
                    for (size_t i = 0; i < kTypeCount; ++i)
                    {
                        const auto bit = static_cast<MessageMask>(1u << i);
                        if (hasMask(it->subscribed, bit))
                            m_subscriberCounts[i].fetch_sub(1, std::memory_order_relaxed);
                    }
                    it = connections.erase(it);
                    continue;
                }

                if (!recvScratch.empty())
                    it->reader.append(recvScratch);

                while (auto frame = it->reader.tryExtract())
                {
                    if (frame->type == MessageType::Control)
                    {
                        if (auto ctrl = decodeControl(frame->payload))
                        {
                            const auto prevMask = it->subscribed;
                            if (ctrl->op == ControlOp::Subscribe)
                                it->subscribed = static_cast<MessageMask>(
                                    static_cast<u32>(it->subscribed) | static_cast<u32>(ctrl->mask));
                            else if (ctrl->op == ControlOp::Unsubscribe)
                                it->subscribed = static_cast<MessageMask>(
                                    static_cast<u32>(it->subscribed) & ~static_cast<u32>(ctrl->mask));

                            for (size_t i = 0; i < kTypeCount; ++i)
                            {
                                const auto bit = static_cast<MessageMask>(1u << i);
                                const bool was = hasMask(prevMask, bit);
                                const bool now = hasMask(it->subscribed, bit);
                                if (now && !was) m_subscriberCounts[i].fetch_add(1, std::memory_order_relaxed);
                                if (was && !now) m_subscriberCounts[i].fetch_sub(1, std::memory_order_relaxed);
                            }
                        }
                    }
                    else if (frame->type == MessageType::ConsoleCommand)
                    {
                        m_inboundCommands.tryPush({frame->type, std::move(frame->payload)});
                    }
                    // other inbound types ignored for now
                }

                if (it->reader.isPoisoned())
                {
                    it = connections.erase(it); // bad stream, drop it
                    continue;
                }

                ++it;
            }

            // fan out telemetry to subscribed connections
            for (size_t i = 0; i < kTypeCount; ++i)
            {
                const auto type = static_cast<MessageType>(i + 1);
                const auto bit = static_cast<MessageMask>(1u << i);

                if (m_subscriberCounts[i].load(std::memory_order_relaxed) == 0)
                    continue;

                std::vector<std::vector<u8>> pending;
                {
                    auto& channel = m_channels[i];
                    std::lock_guard lock(channel.mutex);
                    pending.assign(channel.frames.begin(), channel.frames.end());
                    channel.frames.clear();
                }

                for (auto& conn : connections)
                {
                    if (!hasMask(conn.subscribed, bit)) continue;
                    for (auto& payload : pending)
                        conn.socket.send(encodeFrame(type, payload));
                }
            }

            // drain anything still buffered from a prior wouldBlock
            for (auto& conn : connections)
                conn.socket.flush();

            // !!!!! PLACEHOLDER !!!!!! FIX ME LATER !!!!!
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
}
