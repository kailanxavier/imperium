#include <editor/hierarchy_panel.h>

#include <QAbstractItemView>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QTreeView>
#include <QVBoxLayout>

namespace imp::editor
{
	HierarchyPanel::HierarchyPanel(QWidget* parent) : QWidget(parent) 
	{
		m_tree = new QTreeView(this);
		m_tree->header()->setStretchLastSection(true);
		m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
		m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);

		auto* layout = new QVBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->addWidget(m_tree);
	}

    void HierarchyPanel::setModel(QAbstractItemModel* model)
    {
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
}
