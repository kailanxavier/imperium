#pragma once

#include <QWidget>

class QTreeView;
class QAbstractItemModel;
class QModelIndex;

namespace imp::editor
{
	class HierarchyPanel final : public QWidget
	{
		Q_OBJECT

	public:
		explicit HierarchyPanel(QWidget* parent = nullptr);

		void setModel(QAbstractItemModel* model);

	signals:
		void selectionCleared();
		void entitySelected(const QModelIndex& index);

	private:
		QTreeView* m_tree = nullptr;
	};
}
