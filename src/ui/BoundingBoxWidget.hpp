#pragma once

#include <QWidget>

#include "BoundingBox.hpp"

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
QT_END_NAMESPACE

namespace qi::ui {

// Six spinboxes — three for min, three for max — with cross-validation:
// max spinboxes' minimum tracks the corresponding min value, and vice versa,
// so min ≤ max is always preserved.
class BoundingBoxWidget : public QWidget {
    Q_OBJECT
public:
    explicit BoundingBoxWidget(QWidget* parent = nullptr);

    qi::geometry::BoundingBox boundingBox() const;
    void setBoundingBox(const qi::geometry::BoundingBox& bbox);

signals:
    void bboxChanged(const qi::geometry::BoundingBox& bbox);

private:
    void emitChanged();

    QDoubleSpinBox* minX_;
    QDoubleSpinBox* minY_;
    QDoubleSpinBox* minZ_;
    QDoubleSpinBox* maxX_;
    QDoubleSpinBox* maxY_;
    QDoubleSpinBox* maxZ_;
};

}  // namespace qi::ui
