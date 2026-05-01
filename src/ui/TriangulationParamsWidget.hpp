#pragma once

#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QSpinBox;
class QStackedWidget;
QT_END_NAMESPACE

namespace qi::ui {

// Method combo + stacked widget with two pages: marching_cubes (single
// resolution spinbox) and parametric (uSteps / vSteps).
class TriangulationParamsWidget : public QWidget {
    Q_OBJECT
public:
    explicit TriangulationParamsWidget(QWidget* parent = nullptr);

    QString method() const;
    int mcResolution() const;
    int uSteps() const;
    int vSteps() const;

    void setMethod(const QString& method);
    void setMcResolution(int r);
    void setUSteps(int u);
    void setVSteps(int v);

signals:
    void parametersChanged();

private:
    QComboBox* methodCombo_;
    QStackedWidget* stack_;
    QSpinBox* mcResolution_;
    QSpinBox* uSteps_;
    QSpinBox* vSteps_;
};

}  // namespace qi::ui
