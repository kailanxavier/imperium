#include <editor/world_model.h>
#include <QHash>
#include <algorithm>

namespace imp::editor
{
	namespace
	{
		quint64 entityKey(quint32 index, quint32 generation)
		{
			return ( static_cast<quint64>( index ) << 32 ) | generation;
		}
	}

	WorldModel::WorldModel(QObject* parent) : QAbstractItemModel(parent)
	{ }

	void WorldModel::clear()
	{
		beginResetModel();
		m_nodes.clear();
		m_roots.clear();
		endResetModel();
	}

    void WorldModel::setSnapshot(std::vector<protocol::EntitySnapshotPayload> entities)
    {
        beginResetModel();

        m_nodes.clear();
        m_roots.clear();
        m_nodes.reserve(entities.size());

        QHash<quint64, int> keyToNode;
        keyToNode.reserve(static_cast<int>(entities.size()));

        for (auto& e : entities)
        {
            const int nodeIndex = static_cast<int>(m_nodes.size());
            keyToNode.insert(entityKey(e.index, e.generation), nodeIndex);
            m_nodes.push_back(Node{ std::move(e), -1, {} });
        }

        for (int i = 0; i < static_cast<int>(m_nodes.size()); ++i)
        {
            const auto& entity = m_nodes[i].entity;
            bool linked = false;

            if (entity.parentIndex != 0xFFFFFFFFu)
            {
                const auto it = keyToNode.find(entityKey(entity.parentIndex, entity.parentGeneration));
                if (it != keyToNode.end())
                {
                    m_nodes[i].parent = it.value();
                    m_nodes[it.value()].children.push_back(i);
                    linked = true;
                }
                // else: parent referenced but not present, shouldn't normally happen
                // but something to keep an eye on if we ever come across this happening
            }

            if (!linked)
                m_roots.push_back(i);
        }

        endResetModel();
    }

    QModelIndex WorldModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (row < 0 || column != 0)
            return {};

        if (!parent.isValid())
        {
            if (row >= static_cast<int>(m_roots.size()))
                return {};
            return createIndex(row, column, m_roots[row]);
        }

        const int parentNode = static_cast<int>(parent.internalId());
        if (parentNode < 0 || parentNode >= static_cast<int>(m_nodes.size()))
            return {};

        const auto& children = m_nodes[parentNode].children;
        if (row >= static_cast<int>(children.size()))
            return {};

        return createIndex(row, column, children[row]);
    }

    QModelIndex WorldModel::parent(const QModelIndex& child) const
    {
        if (!child.isValid())
            return {};

        const int nodeIdx = static_cast<int>(child.internalId());
        if (nodeIdx < 0 || nodeIdx >= static_cast<int>(m_nodes.size()))
            return {};

        const int parentNode = m_nodes[nodeIdx].parent;
        if (parentNode < 0)
            return {};

        const int grandparentNode = m_nodes[parentNode].parent;
        const auto& siblings = (grandparentNode < 0) ? m_roots : m_nodes[grandparentNode].children;

        const auto it = std::find(siblings.begin(), siblings.end(), parentNode);
        const int row = (it == siblings.end()) ? 0 : static_cast<int>(std::distance(siblings.begin(), it));

        return createIndex(row, 0, parentNode);
    }

    int WorldModel::rowCount(const QModelIndex& parent) const
    {
        if (!parent.isValid())
            return static_cast<int>(m_roots.size());

        const int nodeIdx = static_cast<int>(parent.internalId());
        if (nodeIdx < 0 || nodeIdx >= static_cast<int>(m_nodes.size()))
            return 0;

        return static_cast<int>(m_nodes[nodeIdx].children.size());
    }

    int WorldModel::columnCount(const QModelIndex& /*parent*/) const
    {
        return 1;
    }

    QVariant WorldModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || role != Qt::DisplayRole)
            return {};

        const int nodeIdx = static_cast<int>(index.internalId());
        if (nodeIdx < 0 || nodeIdx >= static_cast<int>(m_nodes.size()))
            return {};

        const auto& e = m_nodes[nodeIdx].entity;
        if (!e.name.empty())
            return QString::fromStdString(e.name);

        return QString("Entity (%1, %2)").arg(e.index).arg(e.generation);
    }

    QVariant WorldModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && section == 0 && role == Qt::DisplayRole)
            return "Entity";
        return {};
    }

    const protocol::EntitySnapshotPayload* WorldModel::entityAt(const QModelIndex& index) const
    {
        if (!index.isValid())
            return nullptr;

        const int nodeIdx = static_cast<int>(index.internalId());
        if (nodeIdx < 0 || nodeIdx >= static_cast<int>(m_nodes.size()))
            return nullptr;

        return &m_nodes[nodeIdx].entity;
    }
}
