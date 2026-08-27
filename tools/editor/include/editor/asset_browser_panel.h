#pragma once

#include <QWidget>
#include <QDateTime>

#include <protocol/asset_command.h>
#include <protocol/script_status.h>

#include <deque>
#include <vector>

class QTreeView;
class QListWidget;
class QListWidgetItem;
class QModelIndex;
class QPushButton;

namespace imp::editor
{
	class AssetModel;

	class AssetBrowserPanel final : public QWidget
	{
		Q_OBJECT

	public:
		explicit AssetBrowserPanel(QWidget* parent = nullptr);

		void setModel(AssetModel* model);
		void refresh();
		void applyListResult(const protocol::AssetCommandResultPayload& result);
		void addScriptStatus(const protocol::ScriptStatusPayload& status);

	signals:
		void listRequested(QString prefix, bool recursive);
		void sceneEntryActivated(QString virtualPath);

	private:
		void onRefreshClicked();
		void onEntryDoubleClicked(const QModelIndex& index);

		AssetModel* m_model = nullptr;
		QTreeView* m_tree = nullptr;
		QPushButton* m_refreshButton = nullptr;

		QListWidget* m_scriptStatusList = nullptr;
		std::vector<protocol::AssetEntryPayload> m_accumulatedEntries;

		static constexpr int kMaxScriptStatusEntries = 200;
	};
}
