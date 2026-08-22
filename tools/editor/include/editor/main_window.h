#pragma once
#include <QMainWindow>
#include <editor/engine_connection.h>

class QLineEdit;
class QSpinBox;
class QPushButton;
class QLabel;
class QPlainTextEdit;
class QModelIndex;

namespace imp::editor
{
	class HierarchyPanel;
	class InspectorPanel;
	class WorldModel;

	class MainWindow final : public QMainWindow
	{
		Q_OBJECT

	public:
		explicit MainWindow(QWidget* parent = nullptr);

	private slots:
		void onConnectClicked();
		void onConnectionStateChanged(ConnectionState state);
		void onFrameReceived(imp::protocol::MessageType type, std::vector<u8> payload);
		void onEntitySelected(const QModelIndex& index);
		void onSelectionCleared();
		void onCommandRequested(imp::protocol::EntityCommandPayload cmd);

	private:
		void buildUi();
		QWidget* buildConnectionBar();
		void buildLogDock();
		void updateConnectionUi();

		EngineConnection* m_connection = nullptr;
		WorldModel* m_worldModel = nullptr;
		HierarchyPanel* m_hierarchy = nullptr;
		InspectorPanel* m_inspector = nullptr;

		QLineEdit* m_hostEdit = nullptr;
		QSpinBox* m_portSpin = nullptr;
		QPushButton* m_connectButton = nullptr;
		QLabel* m_statusLabel = nullptr;
		QPlainTextEdit* m_logView = nullptr;

		static constexpr int kWidth = 1600;
		static constexpr int kHeight = 900;
	};
}
