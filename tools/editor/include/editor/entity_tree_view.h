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
		void destroyRequested();

	protected:
		void startDrag(Qt::DropActions supportedActions) override;
		void dropEvent(QDropEvent* event) override;
		void keyPressEvent(QKeyEvent* event) override;
	};
}
