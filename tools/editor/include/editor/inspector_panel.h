#pragma once

#include <QWidget>

#include <protocol/entity_command.h>
#include <protocol/world_snapshot.h>

class QLabel;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QDoubleSpinBox;
class QPushButton;

namespace imp::editor
{
	class InspectorPanel final : public QWidget
	{
		Q_OBJECT

	public:
		explicit InspectorPanel(QWidget* parent = nullptr);

		void showEmptyState();
		void showEntity(const protocol::EntitySnapshotPayload& entity);

	signals:
		void commandRequested(imp::protocol::EntityCommandPayload cmd);

	private:
		QGroupBox* buildTransformSection();
		QGroupBox* buildRenderableSection();
		QGroupBox* buildLightSection();

		void emitTransformCommand();
		void emitVisibleCommand(bool visible);
		void emitLightColourCommand();
		void emitLightIntensityCommand(double intensity);
		void emitNameCommand();

		bool m_applyingSnapshot = false;
		protocol::EntitySnapshotPayload m_current;
		bool m_hasSelection = false;

		QLabel* m_titleLabel = nullptr;
		QLineEdit* m_nameEdit = nullptr;

		QGroupBox* m_transformGroup = nullptr;

		QDoubleSpinBox* m_posX = nullptr;
		QDoubleSpinBox* m_posY = nullptr;
		QDoubleSpinBox* m_posZ = nullptr;

		QDoubleSpinBox* m_rotX = nullptr;
		QDoubleSpinBox* m_rotY = nullptr;
		QDoubleSpinBox* m_rotZ = nullptr;

		QDoubleSpinBox* m_scaleX = nullptr;
		QDoubleSpinBox* m_scaleY = nullptr;
		QDoubleSpinBox* m_scaleZ = nullptr;

		QGroupBox* m_renderableGroup = nullptr;
		QCheckBox* m_visibleCheck = nullptr;
		QLabel* m_modelLabel = nullptr;

		QGroupBox* m_lightGroup = nullptr;
		QDoubleSpinBox* m_lightR = nullptr;
		QDoubleSpinBox* m_lightG = nullptr;
		QDoubleSpinBox* m_lightB = nullptr;
		QDoubleSpinBox* m_lightIntensity = nullptr;
	private:
		QGroupBox* buildScriptSection();
		void emitAttachScriptCommand();
		void emitRemoveScriptCommand();

		QGroupBox* m_scriptGroup = nullptr;
		QLineEdit* m_scriptPathEdit = nullptr;
		QCheckBox* m_wantsTickCheck = nullptr;
		QPushButton* m_attachScriptButton = nullptr;
		QPushButton* m_removeScriptButton = nullptr;
	};
}
