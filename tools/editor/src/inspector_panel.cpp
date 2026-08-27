#include <editor/inspector_panel.h>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QPushButton>

#include <core/math/math.h>

namespace imp::editor
{
    namespace
    {
        QDoubleSpinBox* makeAxisSpin(double min, double max, double step)
        {
            auto* spin = new QDoubleSpinBox();
            spin->setRange(min, max);
            spin->setSingleStep(step);
            spin->setDecimals(3);
            return spin;
        }
    }

    InspectorPanel::InspectorPanel(QWidget* parent) : QWidget(parent)
    {
        m_titleLabel = new QLabel(this);
        QFont titleFont = m_titleLabel->font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 1);
        m_titleLabel->setFont(titleFont);

        m_nameEdit = new QLineEdit(this);
        connect(m_nameEdit, &QLineEdit::editingFinished, this, &InspectorPanel::emitNameCommand);

        auto* nameForm = new QFormLayout();
        nameForm->addRow("Name", m_nameEdit);

        auto* layout = new QVBoxLayout(this);
        layout->addWidget(m_titleLabel);
        layout->addLayout(nameForm);
        layout->addWidget(buildTransformSection());
        layout->addWidget(buildRenderableSection());
        layout->addWidget(buildLightSection());
        layout->addWidget(buildScriptSection());
        layout->addStretch();

        showEmptyState();
    }

    QGroupBox* InspectorPanel::buildTransformSection()
    {
        m_transformGroup = new QGroupBox("Transform", this);
        auto* form = new QFormLayout(m_transformGroup);

        auto axisRow = [&](QDoubleSpinBox*& x, QDoubleSpinBox*& y, QDoubleSpinBox*& z,
            double min, double max, double step)
            {
                x = makeAxisSpin(min, max, step);
                y = makeAxisSpin(min, max, step);
                z = makeAxisSpin(min, max, step);

                auto* row = new QWidget();
                auto* rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->addWidget(x);
                rowLayout->addWidget(y);
                rowLayout->addWidget(z);
                return row;
            };

        form->addRow("Position", axisRow(m_posX, m_posY, m_posZ, -1e6, 1e6, 0.1));
        form->addRow("Rotation", axisRow(m_rotX, m_rotY, m_rotZ, -360.0, 360.0, 1.0));
        form->addRow("Scale", axisRow(m_scaleX, m_scaleY, m_scaleZ, -1e6, 1e6, 0.1));

        for (auto* spin : { m_posX, m_posY, m_posZ, m_rotX, m_rotY, m_rotZ, m_scaleX, m_scaleY, m_scaleZ })
        {
            connect(spin, &QDoubleSpinBox::valueChanged, this, [this](double)
                {
                    emitTransformCommand();
                });
        }

        return m_transformGroup;
    }

    QGroupBox* InspectorPanel::buildRenderableSection()
    {
        m_renderableGroup = new QGroupBox("Renderable", this);
        auto* form = new QFormLayout(m_renderableGroup);

        m_modelLabel = new QLabel(this);
        form->addRow("Model", m_modelLabel);

        m_visibleCheck = new QCheckBox(this);
        form->addRow("Visible", m_visibleCheck);
        connect(m_visibleCheck, &QCheckBox::toggled, this, &InspectorPanel::emitVisibleCommand);

        return m_renderableGroup;
    }

    QGroupBox* InspectorPanel::buildLightSection()
    {
        m_lightGroup = new QGroupBox("Light", this);
        auto* form = new QFormLayout(m_lightGroup);

        m_lightR = makeAxisSpin(0.0, 1.0, 0.05);
        m_lightG = makeAxisSpin(0.0, 1.0, 0.05);
        m_lightB = makeAxisSpin(0.0, 1.0, 0.05);

        auto* colourRow = new QWidget();
        auto* colourLayout = new QHBoxLayout(colourRow);
        colourLayout->setContentsMargins(0, 0, 0, 0);
        colourLayout->addWidget(m_lightR);
        colourLayout->addWidget(m_lightG);
        colourLayout->addWidget(m_lightB);
        form->addRow("Colour", colourRow);

        for (auto* spin : { m_lightR, m_lightG, m_lightB })
        {
            connect(spin, &QDoubleSpinBox::valueChanged, this, [this](double)
                {
                    emitLightColourCommand();
                });
        }

        m_lightIntensity = makeAxisSpin(0.0, 1e6, 0.1);
        form->addRow("Intensity", m_lightIntensity);
        connect(m_lightIntensity, &QDoubleSpinBox::valueChanged, this, &InspectorPanel::emitLightIntensityCommand);

        return m_lightGroup;
    }

    QGroupBox* InspectorPanel::buildScriptSection()
    {
        m_scriptGroup = new QGroupBox("Script", this);
        auto* layout = new QVBoxLayout(m_scriptGroup);

        auto* form = new QFormLayout();
        m_scriptPathEdit = new QLineEdit(this);
        m_scriptPathEdit->setPlaceholderText("assets/scripts/script.lua");
        form->addRow("Path", m_scriptPathEdit);

        m_wantsTickCheck = new QCheckBox("Wants Tick", this);
        form->addRow(m_wantsTickCheck);

        layout->addLayout(form);

        auto* buttonLayout = new QHBoxLayout();
        m_attachScriptButton = new QPushButton("Add Script", this);
        m_removeScriptButton = new QPushButton("Remove", this);

        buttonLayout->addWidget(m_attachScriptButton);
        buttonLayout->addWidget(m_removeScriptButton);
        layout->addLayout(buttonLayout);

        connect(m_attachScriptButton, &QPushButton::clicked, this, &InspectorPanel::emitAttachScriptCommand);
        connect(m_removeScriptButton, &QPushButton::clicked, this, &InspectorPanel::emitRemoveScriptCommand);

        connect(m_scriptPathEdit, &QLineEdit::textEdited, this, [this](const QString&)
        {
            m_scriptDirty = true;
        });

        connect(m_wantsTickCheck, &QCheckBox::clicked, this, [this](bool)
        {
            m_scriptDirty = true;
        });

        return m_scriptGroup;
    }

    void InspectorPanel::showEmptyState()
    {
        m_hasSelection = false;
        m_scriptDirty = false;
        m_titleLabel->setText("No selection");

        m_nameEdit->setEnabled(false);
        m_nameEdit->clear();

        m_transformGroup->setVisible(false);
        m_renderableGroup->setVisible(false);
        m_lightGroup->setVisible(false);
        m_scriptGroup->setVisible(false);
    }

    void InspectorPanel::showEntity(const protocol::EntitySnapshotPayload& entity)
    {
        m_applyingSnapshot = true;

        const bool isDifferentEntity = !m_hasSelection
                || m_current.index != entity.index
                || m_current.generation != entity.generation;

        if (isDifferentEntity)
            m_scriptDirty = false;

        m_current = entity;
        m_hasSelection = true;

        m_titleLabel->setText(entity.name.empty()
            ? QString("Entity (%1, %2)").arg(entity.index).arg(entity.generation)
            : QString::fromStdString(entity.name));

        m_nameEdit->setEnabled(true);
        m_nameEdit->setText(QString::fromStdString(entity.name));

        if (entity.transform)
        {
            m_transformGroup->setVisible(true);
            const auto& t = *entity.transform;

            m_posX->setValue(t.localPosition.x);
            m_posY->setValue(t.localPosition.y);
            m_posZ->setValue(t.localPosition.z);

            const auto euler = math::toEuler(t.localRotation);
            m_rotX->setValue(math::toDegrees(euler.pitch));
            m_rotY->setValue(math::toDegrees(euler.yaw));
            m_rotZ->setValue(math::toDegrees(euler.roll));

            m_scaleX->setValue(t.localScale.x);
            m_scaleY->setValue(t.localScale.y);
            m_scaleZ->setValue(t.localScale.z);
        }
        else
        {
            m_transformGroup->setVisible(false);
        }

        if (entity.renderable)
        {
            m_renderableGroup->setVisible(true);
            const auto& r = *entity.renderable;

            m_modelLabel->setText(r.modelIndex == 0xFFFFFFFFu ? "None" : QString("Model #%1").arg(r.modelIndex));
            m_visibleCheck->setChecked(r.visible);
        }
        else
        {
            m_renderableGroup->setVisible(false);
        }

        if (entity.light)
        {
            m_lightGroup->setVisible(true);
            const auto& l = *entity.light;

            m_lightR->setValue(l.colour.x);
            m_lightG->setValue(l.colour.y);
            m_lightB->setValue(l.colour.z);
            m_lightIntensity->setValue(l.intensity);
        }
        else
        {
            m_lightGroup->setVisible(false);
        }
        if (entity.script)
        {
            m_scriptGroup->setVisible(true);
            const auto& s = *entity.script;

            if (!m_scriptDirty)
            {
                m_scriptPathEdit->setText(QString::fromStdString(s.path));
                m_wantsTickCheck->setChecked(s.wantsTick);
            }

            m_attachScriptButton->setText("Update Script");
        }
        else
        {
            m_scriptGroup->setVisible(true);

            if (!m_scriptDirty)
            {
                m_scriptPathEdit->clear();
                m_wantsTickCheck->setChecked(false);
            }

            m_attachScriptButton->setText("Add Script");
        }

        m_applyingSnapshot = false;
    }

    void InspectorPanel::emitTransformCommand()
    {
        if (m_applyingSnapshot || !m_hasSelection)
            return;

        protocol::EntityCommandPayload cmd;
        cmd.op = protocol::EntityCommandOp::SetLocalTransform;
        cmd.targetIndex = m_current.index;
        cmd.targetGeneration = m_current.generation;

        cmd.vec3A = math::Vec3f(
            static_cast<float>(m_posX->value()),
            static_cast<float>(m_posY->value()),
            static_cast<float>(m_posZ->value()));

        cmd.quatA = math::Quaternionf::fromEuler(
            math::toRadians(static_cast<float>(m_rotX->value())),
            math::toRadians(static_cast<float>(m_rotY->value())),
            math::toRadians(static_cast<float>(m_rotZ->value())));

        cmd.vec3B = math::Vec3f(
            static_cast<float>(m_scaleX->value()),
            static_cast<float>(m_scaleY->value()),
            static_cast<float>(m_scaleZ->value()));

        emit commandRequested(cmd);
    }

    void InspectorPanel::emitVisibleCommand(bool visible)
    {
        if (m_applyingSnapshot || !m_hasSelection)
            return;

        protocol::EntityCommandPayload cmd;
        cmd.op = protocol::EntityCommandOp::SetRenderableVisible;
        cmd.targetIndex = m_current.index;
        cmd.targetGeneration = m_current.generation;
        cmd.boolA = visible;

        emit commandRequested(cmd);
    }

    void InspectorPanel::emitLightColourCommand()
    {
        if (m_applyingSnapshot || !m_hasSelection)
            return;

        protocol::EntityCommandPayload cmd;
        cmd.op = protocol::EntityCommandOp::SetLightColour;
        cmd.targetIndex = m_current.index;
        cmd.targetGeneration = m_current.generation;

        cmd.vec3A = math::Vec3f(
            static_cast<float>(m_lightR->value()),
            static_cast<float>(m_lightG->value()),
            static_cast<float>(m_lightB->value()));

        emit commandRequested(cmd);
    }

    void InspectorPanel::emitLightIntensityCommand(double intensity)
    {
        if (m_applyingSnapshot || !m_hasSelection)
            return;

        protocol::EntityCommandPayload cmd;
        cmd.op = protocol::EntityCommandOp::SetLightIntensity;
        cmd.targetIndex = m_current.index;
        cmd.targetGeneration = m_current.generation;
        cmd.floatA = static_cast<float>(intensity);

        emit commandRequested(cmd);
    }

    void InspectorPanel::emitNameCommand()
    {
        if (m_applyingSnapshot || !m_hasSelection)
            return;

        protocol::EntityCommandPayload cmd;
        cmd.op = protocol::EntityCommandOp::SetName;
        cmd.targetIndex = m_current.index;
        cmd.targetGeneration = m_current.generation;
        cmd.stringA = m_nameEdit->text().toStdString();

        emit commandRequested(cmd);
    }

    void InspectorPanel::emitAttachScriptCommand()
    {
        if (m_applyingSnapshot || !m_hasSelection) return;

        protocol::EntityCommandPayload cmd;
        cmd.op = protocol::EntityCommandOp::AttachScript;
        cmd.targetIndex = m_current.index;
        cmd.targetGeneration = m_current.generation;
        cmd.stringA = m_scriptPathEdit->text().toStdString();
        cmd.boolA = m_wantsTickCheck->isChecked();

        m_scriptDirty = false;

        emit commandRequested(cmd);
    }

    void InspectorPanel::emitRemoveScriptCommand()
    {
        if (m_applyingSnapshot || !m_hasSelection) return;

        protocol::EntityCommandPayload cmd;
        cmd.op = protocol::EntityCommandOp::AttachScript;
        cmd.targetIndex = m_current.index;
        cmd.targetGeneration = m_current.generation;
        cmd.stringA.clear();
        cmd.boolA = false;

        m_scriptDirty = false;

        emit commandRequested(cmd);
    }
}
