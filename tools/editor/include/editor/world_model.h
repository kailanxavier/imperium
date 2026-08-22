#pragma once
#include <QAbstractItemModel>
#include <protocol/world_snapshot.h>
#include <vector>

namespace imp::editor
{
	class WorldModel final : public QAbstractItemModel
	{
		Q_OBJECT

	public:
		explicit WorldModel(QObject* parent = nullptr);

		void setSnapshot(std::vector<protocol::EntitySnapshotPayload> entities);
		void clear();

		[[nodiscard]] const protocol::EntitySnapshotPayload* entityAt(const QModelIndex& index) const;

		QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
		QModelIndex parent(const QModelIndex& child) const override;
		int rowCount(const QModelIndex& parent = {}) const override;
		int columnCount(const QModelIndex& parent = {}) const override;
		QVariant data(const QModelIndex& index, int role) const override;
		QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	private:
		struct Node
		{
			protocol::EntitySnapshotPayload entity;
			int parent = -1;
			std::vector<int> children;
		};

		std::vector<Node> m_nodes;
		std::vector<int> m_roots;
	};
}
