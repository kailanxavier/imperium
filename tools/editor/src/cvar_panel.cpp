#include <editor/cvar_panel.h>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <algorithm>

namespace imp::editor
{
    namespace
    {
        const char* typeLabel(protocol::CVarType type)
        {
            switch (type)
            {
                case protocol::CVarType::Float: return "float";
                case protocol::CVarType::Int: return "int";
                case protocol::CVarType::Bool: return "bool";
            }
            return "?";
        }

        QString valueText(const protocol::CVarEntryPayload& entry)
        {
            switch (entry.type)
            {
                case protocol::CVarType::Float: return QString::number(entry.floatValue, 'g', 6);
                case protocol::CVarType::Int: return QString::number(entry.intValue);
                case protocol::CVarType::Bool: return {};
            }
            return {};
        }
    }

    CVarPanel::CVarPanel(QWidget* parent) : QWidget(parent)
    {
        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(4, 4, 4, 4);

        auto* toolbar = new QHBoxLayout(this);

        m_filterEdit = new QLineEdit(this);
        m_filterEdit->setPlaceholderText("Filter");
        connect(m_filterEdit, &QLineEdit::textChanged, this, &CVarPanel::onFilterTextChanged);
        toolbar->addWidget(m_filterEdit, 1);

        m_refreshButton = new QPushButton("Refresh", this);
        connect(m_refreshButton, &QPushButton::clicked, this, &CVarPanel::onRefreshClicked);

        toolbar->addWidget(m_refreshButton);

        rootLayout->addLayout(toolbar);

        m_table = new QTableWidget(0, 3, this);
        m_table->setHorizontalHeaderLabels({ "Name", "Type", "Value" });
        m_table->horizontalHeader()->setStretchLastSection(true);
        m_table->horizontalHeader()->setSectionResizeMode(kColumnName, QHeaderView::Stretch);
        m_table->verticalHeader()->setVisible(false);
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        m_table->setAlternatingRowColors(true);
        connect(m_table, &QTableWidget::itemChanged, this, &CVarPanel::onItemChanged);

        rootLayout->addWidget(m_table, 1);

        m_countLabel = new QLabel(this);
        rootLayout->addWidget(m_countLabel);

        setConnected(false);
    }

    void CVarPanel::setConnected(bool connected)
    {
        m_refreshButton->setEnabled(connected);
        m_table->setEnabled(connected);

        if (!connected)
        {
            m_entries.clear();
            rebuildTable();
        }
    }

    void CVarPanel::refresh()
    {
        emit listRequested();
    }

    void CVarPanel::onRefreshClicked()
    {
        refresh();
    }

    void CVarPanel::onFilterTextChanged(const QString&)
    {
        applyFilter();
    }

    void CVarPanel::applyListResult(const protocol::CVarCommandResultPayload& result)
    {
        if (!result.success)
        {
            emit errorReported(QString("Failed to list cvars: %1").arg(QString::fromStdString(result.error)));
            return;
        }

        m_entries = result.entries;
        std::sort(m_entries.begin(), m_entries.end(),
            [](const protocol::CVarEntryPayload& a, const protocol::CVarEntryPayload& b)
            {
                return a.name < b.name;
            });

        rebuildTable();
    }

    void CVarPanel::applyCommandResult(const protocol::CVarCommandResultPayload& result)
    {
        if (!result.success)
        {
            emit errorReported(QString("Failed to set '%1': %2")
                .arg(QString::fromStdString(result.name))
                .arg(QString::fromStdString(result.error)));

            refresh();
            return;
        }

        if (!result.entry)
            return;

        const auto& entry = *result.entry;
        const QString name = QString::fromStdString(entry.name);

        const auto it = std::find_if(m_entries.begin(), m_entries.end(),
            [&](const protocol::CVarEntryPayload& e) { return e.name == entry.name; });

        if (it != m_entries.end())
            *it = entry;
        else
            m_entries.push_back(entry);

        const int row = rowForName(name);
        if (row >= 0)
            setRowFromEntry(row, entry);
    }

    void CVarPanel::rebuildTable()
    {
        m_updatingProgrammatically = true;

        m_table->setRowCount(static_cast<int>( m_entries.size() ));
        for (int row = 0; row < static_cast<int>( m_entries.size() ); ++row)
            setRowFromEntry(row, m_entries[static_cast<size_t>(row)]);

        m_updatingProgrammatically = false;

        applyFilter();
        m_countLabel->setText(QString("%1 cvars").arg(m_entries.size()));
    }

    void CVarPanel::applyFilter()
    {
        const QString filter = m_filterEdit->text().trimmed();

        for (int row = 0; row < m_table->rowCount(); ++row)
        {
            const auto* nameItem = m_table->item(row, kColumnName);
            const bool matches = filter.isEmpty() ||
                (nameItem && nameItem->text().contains(filter, Qt::CaseInsensitive));

            m_table->setRowHidden(row, !matches);
        }
    }

    int CVarPanel::rowForName(const QString& name) const
    {
        for (int row = 0; row < m_table->rowCount(); ++row)
        {
            const auto* nameItem = m_table->item(row, kColumnName);
            if (nameItem && nameItem->text() == name)
                return row;
        }
        return -1;
    }

    void CVarPanel::setRowFromEntry(int row, const protocol::CVarEntryPayload& entry)
	{
		const bool wasUpdating = m_updatingProgrammatically;
		m_updatingProgrammatically = true;

		const QString name = QString::fromStdString(entry.name);

		auto* nameItem = m_table->item(row, kColumnName);
		if (!nameItem)
		{
			nameItem = new QTableWidgetItem();
			nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
			m_table->setItem(row, kColumnName, nameItem);
		}
		nameItem->setText(name);

		auto* typeItem = m_table->item(row, kColumnType);
		if (!typeItem)
		{
			typeItem = new QTableWidgetItem();
			typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
			m_table->setItem(row, kColumnType, typeItem);
		}
		typeItem->setText(typeLabel(entry.type));

		auto* valueItem = m_table->item(row, kColumnValue);
		if (!valueItem)
		{
			valueItem = new QTableWidgetItem();
			m_table->setItem(row, kColumnValue, valueItem);
		}

		if (entry.type == protocol::CVarType::Bool)
		{
			valueItem->setFlags((valueItem->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
			valueItem->setCheckState(entry.boolValue ? Qt::Checked : Qt::Unchecked);
			valueItem->setText({});
		}
		else
		{
			valueItem->setFlags((valueItem->flags() | Qt::ItemIsEditable) & ~Qt::ItemIsUserCheckable);
			valueItem->setText(valueText(entry));
		}

		valueItem->setData(Qt::UserRole, static_cast<int>( entry.type ));

		m_updatingProgrammatically = wasUpdating;
	}

	void CVarPanel::onItemChanged(QTableWidgetItem* item)
	{
		if (m_updatingProgrammatically || !item || item->column() != kColumnValue)
			return;

		const int row = item->row();
		auto* nameItem = m_table->item(row, kColumnName);
		if (!nameItem)
			return;

		const auto type = static_cast<protocol::CVarType>( item->data(Qt::UserRole).toInt() );

		protocol::CVarCommandPayload cmd;
		cmd.op = protocol::CVarCommandOp::Set;
		cmd.name = nameItem->text().toStdString();
		cmd.type = type;

		bool valid = true;
		switch (type)
		{
		case protocol::CVarType::Float:
		{
			cmd.floatValue = item->text().toFloat(&valid);
			break;
		}
		case protocol::CVarType::Int:
		{
			cmd.intValue = item->text().toInt(&valid);
			break;
		}
		case protocol::CVarType::Bool:
		{
			cmd.boolValue = (item->checkState() == Qt::Checked);
			break;
		}
		}

		if (!valid)
		{
			emit errorReported(QString("'%1' is not a valid value for '%2'.")
				.arg(item->text(), nameItem->text()));

			const auto it = std::find_if(m_entries.begin(), m_entries.end(),
				[&](const protocol::CVarEntryPayload& e) { return e.name == cmd.name; });
			if (it != m_entries.end())
				setRowFromEntry(row, *it);
			return;
		}

		emit setRequested(cmd);
	}
}
