#pragma once

#include <QWidget>
#include <QSet>

#include <protocol/world_snapshot.h>

#include <optional>
#include <vector>

class QModelIndex;

namespace imp::editor
{
	class EntityTreeView;
	class WorldModel;

	class HierarchyPanel final : public QWidget
	{
		Q_OBJECT

	public:
		explicit HierarchyPanel(QWidget* parent = nullptr);

		void setModel(WorldModel* model);
		void applySnapshot(std::vector<protocol::EntitySnapshotPayload> entities);

	signals:
		void selectionCleared();
		void entitySelected(const QModelIndex& index);
		void dragStateChanged(bool active);

	private:
		[[nodiscard]] QSet<quint64> captureExpandedKeys() const;
		void collectExpandedKeys(const QModelIndex& parent, QSet<quint64>& out) const;
		void restoreExpandedKeys(const QSet<quint64>& keys);
		void restoreExpandedKeysRecursive(const QModelIndex& parent, const QSet<quint64>& keys);

		[[nodiscard]] std::optional<quint64> captureSelectedKey() const;
		void restoreSelectedKey(const std::optional<quint64>& key);

		EntityTreeView* m_tree = nullptr;
		WorldModel* m_model = nullptr;
	};
}
