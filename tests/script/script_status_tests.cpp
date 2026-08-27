#include "script_test_helper.h"

#include <script/system.h>

#include <protocol/control.h>
#include <protocol/frame.h>
#include <protocol/script_status.h>
#include <protocol/tcp_socket.h>
#include <protocol/tool_server.h>

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using namespace imp;
using namespace imp::protocol;
using namespace imp::script;

namespace
{
	constexpr u16 kTestPort = 47820;
	
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

class ScriptStatusPublishTest : public ScriptFsFixture
{
protected:
	void TearDown() override
	{
		ToolServer::instance().stop();
		ScriptFsFixture::TearDown();
	}

	TCPSocket connectSubscribedClient()
	{
		EXPECT_TRUE(ToolServer::instance().start(kTestPort));

		TCPSocket client;
		if (!client.connect("127.0.0.1", kTestPort))
			return client;
		client.setNonBlocking(true);

		client.send(encodeFrame(MessageType::Control, encodeControl(ControlOp::Subscribe, MessageMask::ScriptStatus)));
		waitUntil([] { return ToolServer::instance().hasSubscribers(MessageType::ScriptStatus); });

		return client;
	}

	std::optional<ScriptStatusPayload> receiveScriptStatus(TCPSocket& client)
	{
		FrameReader reader;
		std::optional<FrameReader::Frame> frame;

		const bool got = waitUntil([&]
			{
				std::vector<u8> chunk;
				if (!client.recv(chunk))
					return false;
				if (!chunk.empty())
					reader.append(chunk);
				frame = reader.tryExtract();
				return frame.has_value();
			});

		if (!got || frame->type != MessageType::ScriptStatus)
			return std::nullopt;

		return deserialiseScriptStatus(frame->payload);
	}
};

TEST_F(ScriptStatusPublishTest, PublishesSuccessStatusOnSuccessfulReload)
{
	const std::string path = writeScript("pickup.lua", "return { OnInit = function() end }");

	TCPSocket client = connectSubscribedClient();
	ASSERT_TRUE(client.isValid());

	ScriptSystem system(vfs);
	system.reloadScript(path);

	const auto status = receiveScriptStatus(client);
	ASSERT_TRUE(status.has_value());
	EXPECT_TRUE(status->success);
	EXPECT_TRUE(status->error.empty());
	EXPECT_EQ(status->path, path);
}

TEST_F(ScriptStatusPublishTest, PublishesFailureStatusWithErrorTextOnSyntaxError)
{
	const std::string path = writeScript("broken.lua", "this is not valid lua ((((");

	TCPSocket client = connectSubscribedClient();
	ASSERT_TRUE(client.isValid());

	ScriptSystem system(vfs);
	system.reloadScript(path);

	const auto status = receiveScriptStatus(client);
	ASSERT_TRUE(status.has_value());
	EXPECT_FALSE(status->success);
	EXPECT_FALSE(status->error.empty());
	EXPECT_EQ(status->path, path);
}

TEST_F(ScriptStatusPublishTest, DoesNotPublishWhenNobodyIsSubscribed)
{
	const std::string path = writeScript("pickup.lua", "return { OnInit = function() end }");

	ASSERT_TRUE(ToolServer::instance().start(kTestPort));
	ASSERT_FALSE(ToolServer::instance().hasSubscribers(MessageType::ScriptStatus));

	ScriptSystem system(vfs);
	EXPECT_NO_FATAL_FAILURE(system.reloadScript(path)); // must not block
}
