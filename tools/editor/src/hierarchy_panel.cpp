#include <editor/hierarchy_panel.h>

#include <QAbstractItemView>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace imp::editor
{
	HierarchyPanel::HierarchyPanel(QWidget* parent) : QWidget(parent) 
	{
		m_tree = new QTreeWidget(this);
		m_tree->setHeaderLabel("Entity");
		m_tree->setSelectionMode(QAbstractItemView::SingleSelection);

		auto* layout = new QVBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->addWidget(m_tree);

		connect(m_tree, &QTreeWidget::itemSelectionChanged, this,
			[this] {
				const auto selected = m_tree->selectedItems();

				if (selected.isEmpty())
					emit selectionCleared();
				else
					emit entitySelected(m_tree->indexOfTopLevelItem(selected.first()));
			});
	}

	void HierarchyPanel::clear()
	{
		m_tree->clear();
	}

	void HierarchyPanel::setPlaceholderEntities(const QStringList& names)
	{
		m_tree->clear();

		for (const auto& name : names)
			m_tree->addTopLevelItem(new QTreeWidgetItem(QStringList{ name }));
	}
}
