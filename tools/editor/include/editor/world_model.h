#pragma once
#include <QAbstractItemModel>
#include <QKeyEvent>
#include <protocol/world_snapshot.h>
#include <vector>

class QMimeData;

namespace imp::editor
{
	class WorldModel final : public QAbstractItemModel
	{
		Q_OBJECT

	public:
		explicit WorldModel(QObject* parent = nullptr);

		void setSnapshot(std::vector<protocol::EntitySnapshotPayload> entities);
		void clear();

		[[nodiscard]] static quint64 entityKey(quint32 index, quint32 generation);

		[[nodiscard]] const protocol::EntitySnapshotPayload* entityAt(const QModelIndex& index) const;

		QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
		QModelIndex parent(const QModelIndex& child) const override;
		int rowCount(const QModelIndex& parent = {}) const override;
		int columnCount(const QModelIndex& parent = {}) const override;
		QVariant data(const QModelIndex& index, int role) const override;
		QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

		Qt::ItemFlags flags(const QModelIndex& index) const override;
		QStringList mimeTypes() const override;
		Qt::DropActions supportedDropActions() const override;
		QMimeData* mimeData(const QModelIndexList& indexes) const override;
		bool canDropMimeData(const QMimeData* data, Qt::DropAction action,
			int row, int column, const QModelIndex& parent) const override;
		bool dropMimeData(const QMimeData* data, Qt::DropAction action,
			int row, int column, const QModelIndex& parent) override;

		bool removeRows(int rows, int count, const QModelIndex& parent = {}) override;

		bool handleDrop(const QMimeData* data, const QModelIndex& target);

	signals:
		void reparentRequested(quint32 childIndex, quint32 childGeneration,
			bool hasNewParent, quint32 newParentIndex, quint32 newParentGeneration);
		void createRequested(QString modelPath, bool hasParent, quint32 parentIndex, quint32 parentGeneration);

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
