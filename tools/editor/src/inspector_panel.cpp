#include <editor/inspector_panel.h>

#include <QFont>
#include <QLabel>
#include <QVBoxLayout>

namespace imp::editor
{
	InspectorPanel::InspectorPanel(QWidget* parent) : QWidget(parent)
	{
        m_titleLabel = new QLabel(this);

        QFont titleFont = m_titleLabel->font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 1);
        m_titleLabel->setFont(titleFont);

        m_bodyLabel = new QLabel(this);
        m_bodyLabel->setWordWrap(true);
        m_bodyLabel->setStyleSheet("color: palette(mid);");

        auto* layout = new QVBoxLayout(this);
        layout->addWidget(m_titleLabel);
        layout->addWidget(m_bodyLabel);
        layout->addStretch();

        showEmptyState();
	}

    void InspectorPanel::showEmptyState()
    {
        m_titleLabel->setText("No selection");
        m_bodyLabel->setText("Select an entity in the hierarchy to inspect its components.");
    }

    void InspectorPanel::showPlaceholderFor(const QString& entityName)
    {
        m_titleLabel->setText(entityName);
        m_bodyLabel->setText("Component inspection isn't implemented yet");
    }
}
