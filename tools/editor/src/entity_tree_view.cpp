#include <editor/entity_tree_view.h>
#include <editor/world_model.h>

#include <QDropEvent>
#include <QKeyEvent>

namespace imp::editor
{
	EntityTreeView::EntityTreeView(QWidget* parent) : QTreeView(parent) {}

    void EntityTreeView::startDrag(Qt::DropActions supportedActions)
    {
        emit dragStateChanged(true);
        QTreeView::startDrag(supportedActions);
        emit dragStateChanged(false);
    }

    void EntityTreeView::dropEvent(QDropEvent* event)
    {
        const QModelIndex target = indexAt(event->position().toPoint());
        if (auto* worldModel = qobject_cast<WorldModel*>( model() ))
        {
            if (worldModel->handleDrop(event->mimeData(), target))
            {
                event->acceptProposedAction();
                return;
            }
        }

        event->ignore();
    }

    void EntityTreeView::keyPressEvent(QKeyEvent *event)
	{
	    if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)
	    {
	        emit destroyRequested();
	        event->accept();
	    	return;
	    }

		QTreeView::keyPressEvent(event);
	}
}
