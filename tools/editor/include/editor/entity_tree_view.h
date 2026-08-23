#pragma once
#include <QTreeView>

namespace imp::editor
{
	class EntityTreeView final : public QTreeView
	{
		Q_OBJECT

	public:
		explicit EntityTreeView(QWidget* parent = nullptr);

	signals:
		void dragStateChanged(bool active);

	protected:
		void startDrag(Qt::DropActions supportedActions) override;
	};
}
