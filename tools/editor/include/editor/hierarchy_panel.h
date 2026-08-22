#pragma once

#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;

namespace imp::editor
{
	class HierarchyPanel final : public QWidget
	{
		Q_OBJECT

	public:
		explicit HierarchyPanel(QWidget* parent = nullptr);

		void clear();
		void setPlaceholderEntities(const QStringList& name);

	signals:
		void selectionCleared();
		void entitySelected(int row);

	private:
		QTreeWidget* m_tree = nullptr;
	};
}
