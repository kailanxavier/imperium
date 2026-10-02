#pragma once

#include <QPointer>
#include <QWidget>

#include <protocol/viewport_attach.h>

class QLabel;
class QStackedLayout;
class QWindow;

namespace imp::editor
{
	class ViewportPanel final : public QWidget
	{
		Q_OBJECT

	public:
		explicit ViewportPanel(QWidget* parent = nullptr);
		~ViewportPanel() override;

		void attach(const protocol::ViewportAttachPayload& payload);
		void detach(const QString& message);

		void showMessage(const QString& message);
		[[nodiscard]] bool isAttached() const { return m_container != nullptr; }

	private:
		QStackedLayout* m_layout = nullptr;
		QLabel* m_placeholder = nullptr;

		QPointer<QWindow> m_foreign;
		QWidget* m_container = nullptr;
		quint64 m_handle = 0;
	};
}
