#pragma once
#include <QMainWindow>
#include <editor/engine_connection.h>
#include <protocol/world_snapshot.h>

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
		void onReparentRequested(quint32 childIndex, quint32 childGeneration, 
			bool hasNewParent, quint32 newParentIndex, quint32 newParentGeneration);
		void onDragStateChanged(bool active);
		void onSceneBrowseClicked();
		void onSceneSaveClicked();
		void onSceneLoadClicked();

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

		QLineEdit* m_scenePathEdit = nullptr;
		QPushButton* m_sceneBrowseButton = nullptr;
		QPushButton* m_sceneSaveButton = nullptr;
		QPushButton* m_sceneLoadButton = nullptr;

		bool m_dragActive = false;
		std::optional<std::vector<protocol::EntitySnapshotPayload>> m_pendingSnapshot;

		static constexpr bool kDebugFrameReceived = false;

		static constexpr int kWidth = 1600;
		static constexpr int kHeight = 900;
	};
}
