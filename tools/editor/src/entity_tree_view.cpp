#include <editor/entity_tree_view.h>

namespace imp::editor
{
	EntityTreeView::EntityTreeView(QWidget* parent) : QTreeView(parent) {}

    void EntityTreeView::startDrag(Qt::DropActions supportedActions)
    {
        emit dragStateChanged(true);
        QTreeView::startDrag(supportedActions);
        emit dragStateChanged(false);
    }
}
