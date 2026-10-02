#include <editor/viewport_panel.h>
#include <QLabel>
#include <QStackedLayout>
#include <QWindow>

namespace imp::editor
{
	ViewportPanel::ViewportPanel(QWidget* parent) : QWidget(parent)
	{
		setMinimumSize(640, 480);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		m_layout = new QStackedLayout(this);
		m_layout->setContentsMargins(0, 0, 0, 0);

		m_placeholder = new QLabel(this);
		m_placeholder->setAlignment(Qt::AlignCenter);
		m_placeholder->setWordWrap(true);
		m_placeholder->setStyleSheet("background-color: #151414; color: #1D1C1F;");
		m_layout->addWidget(m_placeholder);

		showMessage("Waiting for game...");
	}

	ViewportPanel::~ViewportPanel()
	{
		detach({});
	}

	void ViewportPanel::showMessage(const QString& message)
	{
		m_placeholder->setText(message);
		m_layout->setCurrentWidget(m_placeholder);
	}

	void ViewportPanel::attach(const protocol::ViewportAttachPayload& payload)
	{
		if (payload.nativeHandle == 0)
			return;

		if (m_container && payload.nativeHandle == m_handle)
			return; // already embedded

		if (m_container)
			detach("Game restarted...");

		QWindow* foreign = QWindow::fromWinId(static_cast<WId>( payload.nativeHandle ));
		if (!foreign)
		{
			showMessage("Failed to wrap game window.");
			return;
		}

		m_foreign = foreign;
		m_handle = payload.nativeHandle;

		m_container = QWidget::createWindowContainer(foreign, this);
		m_container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		m_container->setMinimumSize(320, 180);
		m_container->setFocusPolicy(Qt::StrongFocus);

		m_layout->addWidget(m_container);
		m_layout->setCurrentWidget(m_container);
	}

	void ViewportPanel::detach(const QString& message)
	{
		if (m_container)
		{
			if (m_foreign)
			{
				m_foreign->hide();
				m_foreign->setParent(nullptr);
			}

			m_layout->removeWidget(m_container);
			delete m_container; // also deletes m_foreign
			m_container = nullptr;
		}

		m_foreign = nullptr;
		m_handle = 0;

		if (!message.isEmpty())
			showMessage(message);
	}
}
