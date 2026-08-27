#include <editor/engine_connection.h>
#include <protocol/control.h>

namespace imp::editor
{
	using namespace imp::protocol;

	EngineConnection::EngineConnection(QObject* parent) : QObject(parent)
	{
		connect(&m_pollTimer, &QTimer::timeout, this, &EngineConnection::poll);
		m_pollTimer.start(16); // ~1/60 for 60 updates per second
	}

	void EngineConnection::setState(ConnectionState state, QString error)
	{
		if (m_state == state && m_lastError == error)
			return;

		m_state = state;
		m_lastError = std::move(error);
		emit stateChanged(m_state);
	}

	void EngineConnection::connectToEngine(const QString& host, quint16 port)
	{
		m_host = host;
		m_port = port;
		m_wantsConnection = true;

		m_socket = TCPSocket{};
		m_reader = FrameReader{};

		setState(ConnectionState::Connecting);

		const QByteArray hostUtf8 = m_host.toUtf8();
		m_connectStartedAt = std::chrono::steady_clock::now();

		switch (m_socket.beginConnect(hostUtf8.constData(), m_port))
		{
		case TCPSocket::ConnectResult::Failed:
			setState(ConnectionState::Errored, "Failed to connect");
			m_socket = TCPSocket{};
			m_lastReconnectAttempt = std::chrono::steady_clock::now();
			break;

		case TCPSocket::ConnectResult::Connected:
			finaliseConnected();
			break;

		case TCPSocket::ConnectResult::InProgress:
			break;
		}
	}

	void EngineConnection::disconnectFromEngine()
	{
		m_wantsConnection = false;
		m_socket = TCPSocket{};
		m_reader = FrameReader{};
		setState(ConnectionState::Disconnected);
	}

	void EngineConnection::sendControl(ControlOp op, MessageMask mask)
	{
		sendFrame(MessageType::Control, encodeControl(op, mask));
	}

	void EngineConnection::sendFrame(MessageType type, std::span<const u8> payload)
	{
		if (m_state != ConnectionState::Connected)
			return;

		if (!m_socket.send(encodeFrame(type, payload)))
			setState(ConnectionState::Errored, "Failed to send frame.");
	}

	void EngineConnection::sendEntityCommand(const EntityCommandPayload& cmd)
	{
		sendFrame(MessageType::EntityCommand, serialiseEntityCommand(cmd));
	}

	void EngineConnection::sendSceneCommand(const protocol::SceneCommandPayload& cmd)
	{
		sendFrame(MessageType::SceneCommand, serialiseSceneCommand(cmd));
	}

	void EngineConnection::sendAssetCommand(const protocol::AssetCommandPayload& cmd)
	{
		sendFrame(MessageType::AssetCommand, serialiseAssetCommand(cmd));
	}

	void EngineConnection::tickReconnect()
	{
		if (!m_wantsConnection)
			return;

		const auto now = std::chrono::steady_clock::now();
		if (now - m_lastReconnectAttempt < m_reconnectInterval)
			return;

		m_lastReconnectAttempt = now;
		connectToEngine(m_host, m_port);
	}

	void EngineConnection::tickConnecting()
	{
		switch (m_socket.pollConnect())
		{
		case TCPSocket::ConnectResult::Connected:
			finaliseConnected();
			break;

		case TCPSocket::ConnectResult::Failed:
			setState(ConnectionState::Errored, "Failed to connect.");
			m_socket = TCPSocket{};
			m_lastReconnectAttempt = std::chrono::steady_clock::now();
			break;
		case TCPSocket::ConnectResult::InProgress:
			if (std::chrono::steady_clock::now() - m_connectStartedAt > m_connectTimeout)
			{
				setState(ConnectionState::Errored, "Connection timed out.");
				m_socket = TCPSocket{};
				m_lastReconnectAttempt = std::chrono::steady_clock::now();
			}
			break;
		}
	}

	void EngineConnection::finaliseConnected()
	{
		setState(ConnectionState::Connected);

		sendControl(ControlOp::Subscribe,
			static_cast<MessageMask>(
				static_cast<u32>( MessageMask::WorldSnapshot ) |
				static_cast<u32>( MessageMask::EntityCommandResult ) |
				static_cast<u32>( MessageMask::SceneCommandResult ) |
				static_cast<u32>( MessageMask::AssetCommandResult ) |
				static_cast<u32>( MessageMask::ScriptStatus )
				));
	}

	void EngineConnection::poll()
	{
		if (m_state == ConnectionState::Connecting)
		{
			tickConnecting();
			return;
		}

		if (m_state != ConnectionState::Connected)
		{
			tickReconnect();
			return;
		}

		m_recvScratch.clear();

		if (!m_socket.recv(m_recvScratch))
		{
			setState(ConnectionState::Disconnected, "Engine disconnected.");
			m_lastReconnectAttempt = std::chrono::steady_clock::now();
			return;
		}

		if (!m_recvScratch.empty())
			m_reader.append(m_recvScratch);

		while (auto frame = m_reader.tryExtract())
			emit frameReceived(frame->type, std::move(frame->payload));

		if (m_reader.isPoisoned())
		{
			setState(ConnectionState::Errored, "Malformed stream. Dropping connection.");
			m_socket = TCPSocket{};
			m_lastReconnectAttempt = std::chrono::steady_clock::now();
		}
	}
}
