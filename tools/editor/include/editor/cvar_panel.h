#pragma once

#include <QWidget>
#include <protocol/cvar_command.h>

#include <vector>


class QTableWidget;
class QTableWidgetItem;
class QLineEdit;
class QPushButton;
class QLabel;

namespace imp::editor
{
    class CVarPanel final : public QWidget
    {
        Q_OBJECT

    public:
        explicit CVarPanel(QWidget *parent = nullptr);

        void refresh();

        void applyListResult(const protocol::CVarCommandResultPayload& result);
        void applyCommandResult(const protocol::CVarCommandResultPayload& result);

        void setConnected(bool connected);

    signals:
        void listRequested();
        void setRequested(imp::protocol::CVarCommandPayload cmd);
        void errorReported(QString message);

    private slots:
        void onRefreshClicked();
        void onFilterTextChanged(const QString& text);
        void onItemChanged(QTableWidgetItem* item);

    private:
        void rebuildTable();
        void applyFilter();
        int rowForName(const QString& name) const;
        void setRowFromEntry(int row, const protocol::CVarEntryPayload& entry);

        QLineEdit* m_filterEdit = nullptr;
        QPushButton* m_refreshButton = nullptr;
        QLabel* m_countLabel = nullptr;
        QTableWidget* m_table = nullptr;

        std::vector<protocol::CVarEntryPayload> m_entries;

        bool m_updatingProgrammatically = false;

        static constexpr int kColumnName = 0;
        static constexpr int kColumnType = 1;
        static constexpr int kColumnValue = 2;
    };
}
