#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include <protocol/control.h>
#include <protocol/frame.h>
#include <protocol/tcp_socket.h>

#include <chrono>
#include <vector>

#include <span>

namespace imp::editor
{
	enum ConnectionState
	{
		Disconnected,
		Connecting,
		Connected,
		Errored,
	};

	class EngineConnection final : public QObject
	{
		Q_OBJECT

	public:
		explicit EngineConnection(QObject* parent = nullptr);

		void connectToEngine(const QString& host, quint16 port);
		void disconnectFromEngine();

		void sendControl(protocol::ControlOp op, protocol::MessageMask mask);
		void sendFrame(protocol::MessageType type, std::span<const u8> payload);

		[[nodiscard]] ConnectionState state() const { return m_state; }
		[[nodiscard]] const QString& lastError() const { return m_lastError; }
		[[nodiscard]] const QString& host() const { return m_host; }
		[[nodiscard]] quint16 port() const { return m_port; }

	signals:
		void stateChanged(imp::editor::ConnectionState state);
		void frameReceived(imp::protocol::MessageType type, std::vector<u8> payload);

	private slots:
		void poll();

	private:
		void setState(ConnectionState state, QString error = {});
		void tickReconnect();

		protocol::TCPSocket m_socket;
		protocol::FrameReader m_reader;
		std::vector<u8> m_recvScratch;

		QString m_host;
		quint16 m_port = 0;
		bool m_wantsConnection = false;

		ConnectionState m_state = ConnectionState::Disconnected;
		QString m_lastError;

		QTimer m_pollTimer;
		std::chrono::steady_clock::time_point m_lastReconnectAttempt{};
		std::chrono::milliseconds m_reconnectInterval{ 1000 };
	};
}
