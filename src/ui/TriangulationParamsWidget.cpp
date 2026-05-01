#include "TriangulationParamsWidget.hpp"

#include <QComboBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace qi::ui {

namespace {

QSpinBox* makeIntSpin(int initial, int low = 1, int high = 1024) {
    auto* s = new QSpinBox;
    s->setRange(low, high);
    s->setValue(initial);
    return s;
}

}  // namespace

TriangulationParamsWidget::TriangulationParamsWidget(QWidget* parent) : QWidget(parent) {
    methodCombo_ = new QComboBox;
    methodCombo_->addItem(tr("Parametric"), "parametric");
    methodCombo_->addItem(tr("Marching Cubes"), "marching_cubes");

    stack_ = new QStackedWidget;

    auto* parametricPage = new QWidget;
    auto* parametricLayout = new QFormLayout(parametricPage);
    uSteps_ = makeIntSpin(40);
    vSteps_ = makeIntSpin(40);
    parametricLayout->addRow(tr("uSteps"), uSteps_);
    parametricLayout->addRow(tr("vSteps"), vSteps_);
    stack_->addWidget(parametricPage);

    auto* mcPage = new QWidget;
    auto* mcLayout = new QFormLayout(mcPage);
    mcResolution_ = makeIntSpin(32);
    mcLayout->addRow(tr("resolution"), mcResolution_);
    stack_->addWidget(mcPage);

    auto* main = new QVBoxLayout(this);
    main->addWidget(methodCombo_);
    main->addWidget(stack_);
    main->setContentsMargins(0, 0, 0, 0);

    connect(methodCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int idx) {
                stack_->setCurrentIndex(idx);
                emit parametersChanged();
            });
    connect(mcResolution_, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int) { emit parametersChanged(); });
    connect(uSteps_, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int) { emit parametersChanged(); });
    connect(vSteps_, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int) { emit parametersChanged(); });
}

QString TriangulationParamsWidget::method() const {
    return methodCombo_->currentData().toString();
}

int TriangulationParamsWidget::mcResolution() const { return mcResolution_->value(); }
int TriangulationParamsWidget::uSteps() const { return uSteps_->value(); }
int TriangulationParamsWidget::vSteps() const { return vSteps_->value(); }

void TriangulationParamsWidget::setMethod(const QString& method) {
    const int idx = methodCombo_->findData(method);
    if (idx >= 0) {
        methodCombo_->setCurrentIndex(idx);
        stack_->setCurrentIndex(idx);
    }
}

void TriangulationParamsWidget::setMcResolution(int r) { mcResolution_->setValue(r); }
void TriangulationParamsWidget::setUSteps(int u) { uSteps_->setValue(u); }
void TriangulationParamsWidget::setVSteps(int v) { vSteps_->setValue(v); }

}  // namespace qi::ui
