#pragma once

#include <QWidget>

#include "ExperimentConfig.hpp"

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace qi::ui {

class BoundingBoxWidget;
class QuadricParamsWidget;
class TransformWidget;
class TriangulationParamsWidget;

// Composes type combo + QuadricParamsWidget + TransformWidget +
// TriangulationParamsWidget into a single editor that produces a
// SurfaceConfig.
class SurfaceEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit SurfaceEditorWidget(QWidget* parent = nullptr);

    qi::experiment::SurfaceConfig config() const;
    void setConfig(const qi::experiment::SurfaceConfig& sc);

signals:
    void configChanged();

private:
    QComboBox* typeCombo_;
    QuadricParamsWidget* paramsWidget_;
    TransformWidget* transformWidget_;
    TriangulationParamsWidget* triParamsWidget_;
};

}  // namespace qi::ui
