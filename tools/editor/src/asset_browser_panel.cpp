#include <editor/asset_browser_panel.h>
#include <editor/asset_model.h>

#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QColor>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QTreeView>

#include <algorithm>

namespace imp::editor
{
	namespace
	{
		bool hasSuffix(const QString& path, const char* suffix)
		{
			return path.endsWith(suffix, Qt::CaseInsensitive);
		}
	}

	AssetBrowserPanel::AssetBrowserPanel(QWidget* parent) : QWidget(parent)
	{
		auto* rootLayout = new QVBoxLayout(this);
		rootLayout->setContentsMargins(0, 0, 0, 0);

		auto* tabs = new QTabWidget(this);

		auto* assetsTab = new QWidget(this);
		auto* assetsLayout = new QVBoxLayout(assetsTab);
		assetsLayout->setContentsMargins(4, 4, 4, 4);

		auto* toolbar = new QHBoxLayout();
		m_refreshButton = new QPushButton("Refresh", assetsTab);
		connect(m_refreshButton, &QPushButton::clicked, this, &AssetBrowserPanel::onRefreshClicked);
		toolbar->addWidget(m_refreshButton);
		toolbar->addStretch();
		assetsLayout->addLayout(toolbar);

		m_tree = new QTreeView(assetsTab);
		m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
		m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
		m_tree->header()->setStretchLastSection(true);
		connect(m_tree, &QTreeView::doubleClicked, this, &AssetBrowserPanel::onEntryDoubleClicked);
		assetsLayout->addWidget(m_tree);

		tabs->addTab(assetsTab, "Assets");

		auto* scriptTab = new QWidget(this);
		auto* scriptLayout = new QVBoxLayout(scriptTab);
		scriptLayout->setContentsMargins(4, 4, 4, 4);

		m_scriptStatusList = new QListWidget(scriptTab);
		m_scriptStatusList->setAlternatingRowColors(true);
		scriptLayout->addWidget(m_scriptStatusList);

		tabs->addTab(scriptTab, "Script Status");

		rootLayout->addWidget(tabs);
	}

	void AssetBrowserPanel::setModel(AssetModel* model)
	{
		m_model = model;
		m_tree->setModel(model);
	}

	void AssetBrowserPanel::applyListResult(const protocol::AssetCommandResultPayload& result)
	{
		if (!m_model || !result.success)
			return;

		const QString prefix = QString::fromStdString(result.path);
		auto it = std::remove_if(m_accumulatedEntries.begin(), m_accumulatedEntries.end(),
			[&](const protocol::AssetEntryPayload& e)
			{
				return QString::fromStdString(e.virtualPath).startsWith(prefix);
			});
		m_accumulatedEntries.erase(it, m_accumulatedEntries.end());

		m_accumulatedEntries.insert(m_accumulatedEntries.end(),
			result.entries.begin(), result.entries.end());

		m_model->setEntries(m_accumulatedEntries);
		m_tree->expandAll();
	}

	void AssetBrowserPanel::addScriptStatus(const protocol::ScriptStatusPayload& status)
	{
		const QString path = QString::fromStdString(status.path);
		const QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(
			static_cast<qint64>(status.reloadedAtMs));

		QString text = QString("[%1] %2").arg(timestamp.toString("hh:mm:ss")).arg(path);
		if (!status.success)
			text += QString(" - %1").arg(QString::fromStdString(status.error));

		auto* item = new QListWidgetItem(text);
		item->setForeground(status.success ? QColor(Qt::darkGreen) : QColor(Qt::red));

		m_scriptStatusList->insertItem(0, item);

		while (m_scriptStatusList->count() > kMaxScriptStatusEntries)
			delete m_scriptStatusList->takeItem(m_scriptStatusList->count() - 1);
	}

	void AssetBrowserPanel::refresh()
	{
		emit listRequested("assets/", true);
		emit listRequested("scenes/", true);
	}

	void AssetBrowserPanel::onRefreshClicked()
	{
		refresh();
	}

	void AssetBrowserPanel::onEntryDoubleClicked(const QModelIndex& index)
	{
		if (!m_model || m_model->isDirectoryAt(index))
			return;

		const QString path = m_model->virtualPathAt(index);
		if (path.isEmpty())
			return;

		if (hasSuffix(path, ".scene"))
			emit sceneEntryActivated(path);
	}
}
