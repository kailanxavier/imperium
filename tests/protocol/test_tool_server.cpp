#include <gtest/gtest.h>
#include <protocol/tool_server.h>
#include <protocol/tcp_socket.h>
#include <protocol/frame.h>
#include <protocol/control.h>
#include <thread>
#include <chrono>

using namespace imp::protocol;

namespace
{
	constexpr u16 kTestPort = 47812; // arbitrary, shouldnt collide
    template <typename Pred>
    bool waitUntil(Pred pred, std::chrono::milliseconds timeout = std::chrono::milliseconds(500))
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (pred()) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }
}

class ToolServerTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        ToolServer::instance().stop();
    }
};

TEST_F(ToolServerTest, ClientCanConnectAndSubscribe)
{
    ASSERT_TRUE(ToolServer::instance().start(kTestPort));

    EXPECT_FALSE(ToolServer::instance().hasSubscribers(MessageType::MemoryTelemetry));

    TCPSocket client;
    ASSERT_TRUE(client.connect("127.0.0.1", kTestPort));
    client.setNonBlocking(true);

    const auto controlPayload = encodeControl(ControlOp::Subscribe, MessageMask::MemoryTelemetry);
    ASSERT_TRUE(client.send(encodeFrame(MessageType::Control, controlPayload)));

    const bool subscribed = waitUntil([] {
        return ToolServer::instance().hasSubscribers(MessageType::MemoryTelemetry);
        });
    EXPECT_TRUE(subscribed);
}

TEST_F(ToolServerTest, PublishedTelemetryReachesSubscribedClient)
{
    ASSERT_TRUE(ToolServer::instance().start(kTestPort));

    TCPSocket client;
    ASSERT_TRUE(client.connect("127.0.0.1", kTestPort));
    client.setNonBlocking(true);

    ASSERT_TRUE(client.send(encodeFrame(
        MessageType::Control,
        encodeControl(ControlOp::Subscribe, MessageMask::MemoryTelemetry))));

    ASSERT_TRUE(waitUntil([] {
        return ToolServer::instance().hasSubscribers(MessageType::MemoryTelemetry);
        }));

    const std::vector<u8> payload = {1, 2, 3, 4, 5};
    ToolServer::instance().publish(MessageType::MemoryTelemetry, payload);

    FrameReader reader;
    std::optional<FrameReader::Frame> received;

    const bool gotFrame = waitUntil([&] {
        std::vector<u8> chunk;
        if (!client.recv(chunk))
            return false; // connection dropped

        if (!chunk.empty())
            reader.append(chunk);

        received = reader.tryExtract();
        return received.has_value();
        });

    ASSERT_TRUE(gotFrame);
    EXPECT_EQ(received->type, MessageType::MemoryTelemetry);
    EXPECT_EQ(received->payload, payload);
}

TEST_F(ToolServerTest, DisconnectDropsSubscription)
{
    ASSERT_TRUE(ToolServer::instance().start(kTestPort));

    {
        TCPSocket client;
        ASSERT_TRUE(client.connect("127.0.0.1", kTestPort));
        client.setNonBlocking(true);
        ASSERT_TRUE(client.send(encodeFrame(
            MessageType::Control,
            encodeControl(ControlOp::Subscribe, MessageMask::MemoryTelemetry))));

        ASSERT_TRUE(waitUntil([] {
            return ToolServer::instance().hasSubscribers(MessageType::MemoryTelemetry);
            }));

        client.close(); // out of scope
    }

    const bool unsubscribed = waitUntil([] {
        return !ToolServer::instance().hasSubscribers(MessageType::MemoryTelemetry);
        });
    EXPECT_TRUE(unsubscribed);
}
