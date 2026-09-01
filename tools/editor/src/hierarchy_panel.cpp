#include <editor/hierarchy_panel.h>
#include <editor/entity_tree_view.h>
#include <editor/world_model.h>

#include <QAbstractItemView>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QTreeView>
#include <QVBoxLayout>

#include <functional>

namespace imp::editor
{
	HierarchyPanel::HierarchyPanel(QWidget* parent) : QWidget(parent) 
	{
		m_tree = new EntityTreeView(this);
		m_tree->header()->setStretchLastSection(true);
		m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
		m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tree->setDragEnabled(true);
        m_tree->setAcceptDrops(true);
        m_tree->setDropIndicatorShown(true);
        m_tree->setDragDropMode(QAbstractItemView::DragDrop);
        m_tree->setDefaultDropAction(Qt::MoveAction);

        connect(m_tree, &EntityTreeView::dragStateChanged, this, &HierarchyPanel::dragStateChanged);
	    connect(m_tree, &EntityTreeView::destroyRequested, this, &HierarchyPanel::destroyRequested);

		auto* layout = new QVBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->addWidget(m_tree);
	}

    void HierarchyPanel::setModel(WorldModel* model)
    {
        m_model = model;
        m_tree->setModel(model);

        if (auto* selection = m_tree->selectionModel())
        {
            connect(selection, &QItemSelectionModel::currentRowChanged, this,
                [this](const QModelIndex& current, const QModelIndex& /*previous*/)
                {
                    if (current.isValid())
                        emit entitySelected(current);
                    else
                        emit selectionCleared();
                });
        }
    }

    void HierarchyPanel::applySnapshot(std::vector<protocol::EntitySnapshotPayload> entities)
    {
        if (!m_model)
            return;

        const auto expandedKeys = captureExpandedKeys();
        const auto selectedKey = captureSelectedKey();

        m_model->setSnapshot(std::move(entities));

        restoreExpandedKeys(expandedKeys);
        restoreSelectedKey(selectedKey);
    }

    void HierarchyPanel::clearSelection()
    {
        if (auto* selection = m_tree->selectionModel())
            selection->clear();
    }

    std::optional<std::pair<quint32, quint32>> HierarchyPanel::selectedEntity() const
	{
	    if (!m_model)
	        return std::nullopt;

	    const auto* selection = m_tree->selectionModel();
	    if (!selection)
	        return std::nullopt;

	    const auto rows = selection->selectedRows();
	    if (rows.isEmpty())
	        return std::nullopt;

	    if (const auto* entity = m_model->entityAt(rows.first()))
	        return std::make_pair(entity->index, entity->generation);

	    return std::nullopt;
	}

    QSet<quint64> HierarchyPanel::captureExpandedKeys() const
    {
        QSet<quint64> keys;
        if (m_model)
            collectExpandedKeys({}, keys);
        return keys;
    }

    void HierarchyPanel::collectExpandedKeys(const QModelIndex& parent, QSet<quint64>& out) const
    {
        const int rows = m_model->rowCount(parent);
        for (int r = 0; r < rows; ++r)
        {
            const QModelIndex idx = m_model->index(r, 0, parent);

            if (m_tree->isExpanded(idx))
            {
                if (const auto* e = m_model->entityAt(idx))
                    out.insert(WorldModel::entityKey(e->index, e->generation));
            }
            collectExpandedKeys(idx, out);
        }
    }

    void HierarchyPanel::restoreExpandedKeys(const QSet<quint64>& keys)
    {
        if (!keys.isEmpty())
            restoreExpandedKeysRecursive({}, keys);
    }

    void HierarchyPanel::restoreExpandedKeysRecursive(const QModelIndex& parent, const QSet<quint64>& keys)
    {
        const int rows = m_model->rowCount(parent);
        for (int r = 0; r < rows; ++r)
        {
            const QModelIndex idx = m_model->index(r, 0, parent);

            if (const auto* e = m_model->entityAt(idx))
            {
                if (keys.contains(WorldModel::entityKey(e->index, e->generation)))
                    m_tree->setExpanded(idx, true);
            }

            restoreExpandedKeysRecursive(idx, keys);
        }
    }

    std::optional<quint64> HierarchyPanel::captureSelectedKey() const
    {
        if (!m_model)
            return std::nullopt;

        const auto* selection = m_tree->selectionModel();
        if (!selection)
            return std::nullopt;

        const auto rows = selection->selectedRows();
        if (rows.isEmpty())
            return std::nullopt;

        if (const auto* e = m_model->entityAt(rows.first()))
            return WorldModel::entityKey(e->index, e->generation);

        return std::nullopt;
    }

    void HierarchyPanel::restoreSelectedKey(const std::optional<quint64>& key)
    {
        if (!key || !m_model)
            return;

        std::function<QModelIndex(const QModelIndex&)> find = [&](const QModelIndex& parent) -> QModelIndex
            {
                const int rows = m_model->rowCount(parent);
                for (int r = 0; r < rows; ++r)
                {
                    const QModelIndex idx = m_model->index(r, 0, parent);

                    if (const auto* e = m_model->entityAt(idx))
                    {
                        if (WorldModel::entityKey(e->index, e->generation) == *key)
                            return idx;
                    }

                    if (const QModelIndex found = find(idx); found.isValid())
                        return found;
                }
                return {};
            };

        if (const QModelIndex found = find({}); found.isValid())
        {
            if (auto* selection = m_tree->selectionModel())
            {
                const bool wasAutoScroll = m_tree->hasAutoScroll();
                m_tree->setAutoScroll(false);
                selection->setCurrentIndex(found, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                m_tree->setAutoScroll(wasAutoScroll);
            }
        }
    }
}
