#include <editor/asset_model.h>
#include <QStringList>

namespace imp::editor
{
	AssetModel::AssetModel(QObject* parent) : QAbstractItemModel(parent) {}

	void AssetModel::clear()
	{
		beginResetModel();
		m_nodes.clear();
		m_roots.clear();
		endResetModel();
	}

	int AssetModel::findOrCreateChildDir(int parentNode, const QString& name)
	{
		{
			const auto& siblings = (parentNode < 0) ? m_roots : m_nodes[parentNode].children;
			for (int childIdx : siblings)
			{
				if (m_nodes[childIdx].isDirectory && m_nodes[childIdx].name == name)
					return childIdx;
			}
		}

		Node node;
		node.name = name;
		node.isDirectory = true;
		node.parent = parentNode;

		const int newIndex = static_cast<int>(m_nodes.size());
		m_nodes.push_back(std::move(node));

		auto& siblings = (parentNode < 0) ? m_roots : m_nodes[parentNode].children;
		siblings.push_back(newIndex);

		return newIndex;
	}

	void AssetModel::setEntries(std::vector<protocol::AssetEntryPayload> entries)
	{
		beginResetModel();

		m_nodes.clear();
		m_roots.clear();

		for (auto& e : entries)
		{
			const QString virtualPath = QString::fromStdString(e.virtualPath);
			const QStringList segments = virtualPath.split('/', Qt::SkipEmptyParts);

			if (segments.isEmpty())
				continue;

			int parentNode = -1;
			for (int i = 0; i < segments.size() - 1; ++i)
				parentNode = findOrCreateChildDir(parentNode, segments[i]);

			Node fileNode;
			fileNode.name = segments.last();
			fileNode.isDirectory = false;
			fileNode.entry = std::move(e);
			fileNode.parent = parentNode;

			const int newIndex = static_cast<int>(m_nodes.size());
			m_nodes.push_back(std::move(fileNode));

			auto& siblings = (parentNode < 0) ? m_roots : m_nodes[parentNode].children;
			siblings.push_back(newIndex);
		}

		endResetModel();
	}

	QModelIndex AssetModel::index(int row, int column, const QModelIndex& parent) const
	{
		if (row < 0 || column < 0 || column >= columnCount())
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

	QModelIndex AssetModel::parent(const QModelIndex& child) const
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

		int row = 0;
		for (int i = 0; i < static_cast<int>(siblings.size()); ++i)
		{
			if (siblings[i] == parentNode) { row = i; break; }
		}

		return createIndex(row, 0, parentNode);
	}

	int AssetModel::rowCount(const QModelIndex& parent) const
	{
		if (!parent.isValid())
			return static_cast<int>(m_roots.size());

		const int nodeIdx = static_cast<int>(parent.internalId());
		if (nodeIdx < 0 || nodeIdx >= static_cast<int>(m_nodes.size()))
			return 0;

		return static_cast<int>(m_nodes[nodeIdx].children.size());
	}

	int AssetModel::columnCount(const QModelIndex& /*parent*/) const
	{
		return 2; // name and size
	}

	QVariant AssetModel::data(const QModelIndex& index, int role) const
	{
		if (!index.isValid())
			return {};

		const int nodeIdx = static_cast<int>(index.internalId());
		if (nodeIdx < 0 || nodeIdx >= static_cast<int>(m_nodes.size()))
			return {};

		const auto& node = m_nodes[nodeIdx];

		if (role == Qt::DisplayRole)
		{
			if (index.column() == 0)
				return node.name;

			if (index.column() == 1 && !node.isDirectory)
				return QString("%1 B").arg(node.entry.sizeBytes);

			return {};
		}

		return {};
	}

	QVariant AssetModel::headerData(int section, Qt::Orientation orientation, int role) const
	{
		if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		{
			if (section == 0) return "Name";
			if (section == 1) return "Size";
		}
		return {};
	}

	QString AssetModel::virtualPathAt(const QModelIndex& index) const
	{
		if (const auto* e = entryAt(index))
			return QString::fromStdString(e->virtualPath);
		return {};
	}

	bool AssetModel::isDirectoryAt(const QModelIndex& index) const
	{
		if (!index.isValid())
			return false;

		const int nodeIdx = static_cast<int>(index.internalId());
		if (nodeIdx < 0 || nodeIdx >= static_cast<int>(m_nodes.size()))
			return false;

		return m_nodes[nodeIdx].isDirectory;
	}

	const protocol::AssetEntryPayload* AssetModel::entryAt(const QModelIndex& index) const
	{
		if (!index.isValid())
			return nullptr;

		const int nodeIdx = static_cast<int>(index.internalId());
		if (nodeIdx < 0 || nodeIdx >= static_cast<int>(m_nodes.size()))
			return nullptr;

		const auto& node = m_nodes[nodeIdx];
		if (node.isDirectory)
			return nullptr;

		return &node.entry;
	}
}
