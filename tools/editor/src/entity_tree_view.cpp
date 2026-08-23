#include <editor/entity_tree_view.h>
#include <editor/world_model.h>

#include <QDropEvent>

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
}
