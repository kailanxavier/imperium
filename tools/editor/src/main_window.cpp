#include <editor/main_window.h>

#include <editor/hierarchy_panel.h>
#include <editor/inspector_panel.h>
#include <editor/world_model.h>

#include <protocol/message_type.h>
#include <protocol/world_snapshot.h>
#include <protocol/entity_command.h>

#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

namespace imp::editor
{
	namespace
	{
		const char* connectionStateLabel(ConnectionState state)
		{
			switch (state)
			{
			case ConnectionState::Disconnected: return "Disconnected";
			case ConnectionState::Connecting: return "Connecting...";
			case ConnectionState::Connected: return "Connected";
			case ConnectionState::Errored: return "Error";
			}
			return "Unknown";
		}

		const char* messageTypeLabel(protocol::MessageType type)
		{
			switch (type)
			{
			case protocol::MessageType::Control: return "Control";
			case protocol::MessageType::MemoryTelemetry: return "MemoryTelemetry";
			case protocol::MessageType::ProfilerFrame: return "ProfilerFrame";
			case protocol::MessageType::ConsoleCommand: return "ConsoleCommand";
			case protocol::MessageType::ConsoleResponse: return "ConsoleResponse";
			case protocol::MessageType::WorldSnapshot: return "WorldSnapshot";
			case protocol::MessageType::EntityCommand: return "EntityCommand";
			case protocol::MessageType::EntityCommandResult: return "EntityCommandResult";
			}
			return "Unknown";
		}
	}

	MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
	{
		setWindowTitle("impEditor");

		m_connection = new EngineConnection(this);

		connect(m_connection, &EngineConnection::stateChanged, this, &MainWindow::onConnectionStateChanged);
		connect(m_connection, &EngineConnection::frameReceived, this, &MainWindow::onFrameReceived);

		m_worldModel = new WorldModel(this);
		connect(m_worldModel, &WorldModel::reparentRequested, this, &MainWindow::onReparentRequested);

		buildUi();
		updateConnectionUi();
	}

	void MainWindow::buildUi()
	{
		auto* central = new QWidget(this);
		auto* rootLayout = new QVBoxLayout(central);

		rootLayout->addWidget(buildConnectionBar());

		// hierarchy and inspector
		auto* splitter = new QSplitter(Qt::Horizontal, this);

		m_hierarchy = new HierarchyPanel(this);
		m_hierarchy->setModel(m_worldModel);
		connect(m_hierarchy, &HierarchyPanel::entitySelected, this, &MainWindow::onEntitySelected);
		connect(m_hierarchy, &HierarchyPanel::selectionCleared, this, &MainWindow::onSelectionCleared);
		connect(m_hierarchy, &HierarchyPanel::dragStateChanged, this, &MainWindow::onDragStateChanged);
		splitter->addWidget(m_hierarchy);

		m_inspector = new InspectorPanel(this);
		connect(m_inspector, &InspectorPanel::commandRequested, this, &MainWindow::onCommandRequested);
		splitter->addWidget(m_inspector);

		splitter->setStretchFactor(0, 1);
		splitter->setStretchFactor(1, 2);

		rootLayout->addWidget(splitter, 1);

		setCentralWidget(central);
		buildLogDock();
		resize(kWidth, kHeight);
	}

	QWidget* MainWindow::buildConnectionBar()
	{
		auto* connectionBar = new QWidget(this);
		auto* connectionLayout = new QHBoxLayout(connectionBar);
		connectionLayout->setContentsMargins(4, 4, 4, 4);

		connectionLayout->addWidget(new QLabel("Host:", this));
		m_hostEdit = new QLineEdit("127.0.0.1", this);
		m_hostEdit->setFixedWidth(140);
		connectionLayout->addWidget(m_hostEdit);

		connectionLayout->addWidget(new QLabel("Port:", this));
		m_portSpin = new QSpinBox(this);
		m_portSpin->setRange(1, 65535);
		m_portSpin->setValue(47810);
		connectionLayout->addWidget(m_portSpin);

		m_connectButton = new QPushButton("Connect", this);
		connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
		connectionLayout->addWidget(m_connectButton);

		m_statusLabel = new QLabel(this);
		connectionLayout->addWidget(m_statusLabel);
		connectionLayout->addStretch();

		return connectionBar;
	}

	void MainWindow::buildLogDock()
	{
		auto* dock = new QDockWidget("Frame Log", this);
		m_logView = new QPlainTextEdit(dock);
		m_logView->setReadOnly(true);
		m_logView->setMaximumBlockCount(1000);
		dock->setWidget(m_logView);
		addDockWidget(Qt::BottomDockWidgetArea, dock);
	}

	void MainWindow::onConnectClicked()
	{
		const auto state = m_connection->state();

		const bool busy =
			state == ConnectionState::Connected ||
			state == ConnectionState::Connecting;

		busy ? m_connection->disconnectFromEngine()
			: m_connection->connectToEngine(m_hostEdit->text(),
				static_cast<quint16>( m_portSpin->value() ));
	}

	void MainWindow::onConnectionStateChanged(ConnectionState /*state*/)
	{
		updateConnectionUi();
	}

	void MainWindow::updateConnectionUi()
	{
		const auto state = m_connection->state();

		m_statusLabel->setText(connectionStateLabel(state));

		const bool busy = state == ConnectionState::Connected || state == ConnectionState::Connecting;
		m_connectButton->setText(busy ? "Disconnect" : "Connect");

		m_hostEdit->setEnabled(!busy);
		m_portSpin->setEnabled(!busy);

		if (state == ConnectionState::Errored && !m_connection->lastError().isEmpty())
			m_logView->appendPlainText("[ERROR] " + m_connection->lastError());

		if (state != ConnectionState::Connected)
		{
			m_worldModel->clear();
			m_inspector->showEmptyState();
		}
	}

	void MainWindow::onFrameReceived(protocol::MessageType type, std::vector<u8> payload)
	{
		m_logView->appendPlainText(QString("[FRAME] %1 (%2 bytes)")
			.arg(messageTypeLabel(type))
			.arg(payload.size()));

		if (type == protocol::MessageType::WorldSnapshot)
		{
			if (auto entities = protocol::deserialiseWorldSnapshot(payload))
			{
				if (m_dragActive)
					m_pendingSnapshot = std::move(*entities);
				else
					m_hierarchy->applySnapshot(std::move(*entities));
			}
			else
				m_logView->appendPlainText("[WARNING] Failed to parse WorldSnapshot frame.");
		}
		else if (type == protocol::MessageType::EntityCommandResult)
		{
			if (auto result = protocol::deserialiseEntityCommandResult(payload); result && !result->success)
			{
				m_logView->appendPlainText(QString("[COMMAND FAILED] Entity (%1, %2): %3")
					.arg(result->targetIndex)
					.arg(result->targetGeneration)
					.arg(QString::fromStdString(result->error)));
			}
		}
	}

	void MainWindow::onEntitySelected(const QModelIndex& index)
	{
		if (const auto* entity = m_worldModel->entityAt(index))
			m_inspector->showEntity(*entity);
		else
			m_inspector->showEmptyState();
	}

	void MainWindow::onSelectionCleared()
	{
		m_inspector->showEmptyState();
	}

	void MainWindow::onCommandRequested(protocol::EntityCommandPayload cmd)
	{
		m_connection->sendCommand(cmd);
	}

	void MainWindow::onReparentRequested(quint32 childIndex, quint32 childGeneration, bool hasNewParent, quint32 newParentIndex, quint32 newParentGeneration)
	{
		protocol::EntityCommandPayload cmd;
		cmd.op = protocol::EntityCommandOp::Reparent;
		cmd.targetIndex = childIndex;
		cmd.targetGeneration = childGeneration;

		if (hasNewParent)
		{
			cmd.refIndex = newParentIndex;
			cmd.refGeneration = newParentGeneration;
		}

		m_connection->sendCommand(cmd);
	}

	void MainWindow::onDragStateChanged(bool active)
	{
		m_dragActive = active;

		if (!active && m_pendingSnapshot)
		{
			m_hierarchy->applySnapshot(std::move(*m_pendingSnapshot));
			m_pendingSnapshot.reset();
		}
	}
}
