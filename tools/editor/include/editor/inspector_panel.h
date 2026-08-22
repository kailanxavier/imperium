#pragma once

#include <QWidget>

class QLabel;
class QFormLayout;

namespace imp::editor
{
	class InspectorPanel final : public QWidget
	{
		Q_OBJECT

	public:
		explicit InspectorPanel(QWidget* parent = nullptr);

		void showEmptyState();
		void showPlaceholderFor(const QString& entityName);

	private:
		QLabel* m_titleLabel = nullptr;
		QLabel* m_bodyLabel = nullptr;
	};
}
