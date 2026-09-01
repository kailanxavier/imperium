#pragma once
#include <QAbstractItemModel>
#include <protocol/asset_command.h>
#include <vector>

class QMimeData;

namespace imp::editor
{
	class AssetModel final : public QAbstractItemModel
	{
		Q_OBJECT

	public:
		explicit AssetModel(QObject* parent = nullptr);

		void setEntries(std::vector<protocol::AssetEntryPayload> entries);
		void clear();

		[[nodiscard]] QString virtualPathAt(const QModelIndex& index) const;
		[[nodiscard]] bool isDirectoryAt(const QModelIndex& index) const;
		[[nodiscard]] const protocol::AssetEntryPayload* entryAt(const QModelIndex& index) const;

		QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
		QModelIndex parent(const QModelIndex& child) const override;
		int rowCount(const QModelIndex& parent = {}) const override;
		int columnCount(const QModelIndex& parent = {}) const override;
		QVariant data(const QModelIndex& index, int role) const override;
		QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

		Qt::ItemFlags flags(const QModelIndex& index) const override;
		QStringList mimeTypes() const override;
		QMimeData* mimeData(const QModelIndexList &indexes) const override;

	private:
		struct Node
		{
			QString name;
			bool isDirectory = false;
			protocol::AssetEntryPayload entry;
			int parent = -1;
			std::vector<int> children;
		};

		int findOrCreateChildDir(int parentNode, const QString& name);

		std::vector<Node> m_nodes;
		std::vector<int> m_roots;
	};
}
