#include "SurfaceEditorWidget.hpp"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QVBoxLayout>

#include "QuadricFactory.hpp"
#include "QuadricParamsWidget.hpp"
#include "TransformWidget.hpp"
#include "TriangulationParamsWidget.hpp"

namespace qi::ui {

using qi::experiment::SurfaceConfig;
using qi::geometry::knownQuadricTypes;

SurfaceEditorWidget::SurfaceEditorWidget(QWidget* parent) : QWidget(parent) {
    typeCombo_ = new QComboBox;
    for (const auto& t : knownQuadricTypes()) {
        const QString qt = QString::fromStdString(t);
        typeCombo_->addItem(qt, qt);
    }

    paramsWidget_ = new QuadricParamsWidget;
    transformWidget_ = new TransformWidget;
    triParamsWidget_ = new TriangulationParamsWidget;

    auto* paramsBox = new QGroupBox(tr("Parameters"));
    auto* paramsLayout = new QVBoxLayout(paramsBox);
    paramsLayout->addWidget(paramsWidget_);

    auto* transformBox = new QGroupBox(tr("Transform"));
    auto* transformLayout = new QVBoxLayout(transformBox);
    transformLayout->addWidget(transformWidget_);

    auto* triBox = new QGroupBox(tr("Triangulation"));
    auto* triLayout = new QVBoxLayout(triBox);
    triLayout->addWidget(triParamsWidget_);

    auto* main = new QFormLayout(this);
    main->addRow(tr("Type"), typeCombo_);
    main->addRow(paramsBox);
    main->addRow(transformBox);
    main->addRow(triBox);

    connect(typeCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) {
                paramsWidget_->setType(typeCombo_->currentData().toString());
                emit configChanged();
            });
    connect(paramsWidget_, &QuadricParamsWidget::parametersChanged, this,
            &SurfaceEditorWidget::configChanged);
    connect(transformWidget_, &TransformWidget::transformChanged, this,
            [this](const auto&) { emit configChanged(); });
    connect(triParamsWidget_, &TriangulationParamsWidget::parametersChanged, this,
            &SurfaceEditorWidget::configChanged);

    paramsWidget_->setType(typeCombo_->currentData().toString());
}

SurfaceConfig SurfaceEditorWidget::config() const {
    SurfaceConfig sc;
    sc.type = typeCombo_->currentData().toString();
    sc.params = paramsWidget_->params();
    sc.transform = transformWidget_->transform();
    sc.triangulationMethod = triParamsWidget_->method();
    sc.mcResolution = triParamsWidget_->mcResolution();
    sc.uSteps = triParamsWidget_->uSteps();
    sc.vSteps = triParamsWidget_->vSteps();
    return sc;
}

void SurfaceEditorWidget::setConfig(const SurfaceConfig& sc) {
    const int idx = typeCombo_->findData(sc.type);
    if (idx >= 0) typeCombo_->setCurrentIndex(idx);
    paramsWidget_->setType(sc.type);
    paramsWidget_->setParams(sc.params);
    transformWidget_->setTransform(sc.transform);
    triParamsWidget_->setMethod(sc.triangulationMethod);
    triParamsWidget_->setMcResolution(sc.mcResolution);
    triParamsWidget_->setUSteps(sc.uSteps);
    triParamsWidget_->setVSteps(sc.vSteps);
    emit configChanged();
}

}  // namespace qi::ui
