#pragma once

#include <QWidget>

#include "Transform.hpp"

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
QT_END_NAMESPACE

namespace qi::ui {

// Translation as three (x, y, z) spinboxes; rotation entered as
// extrinsic Euler angles in degrees (rotX → rotY → rotZ), converted to
// a quaternion on read.
class TransformWidget : public QWidget {
    Q_OBJECT
public:
    explicit TransformWidget(QWidget* parent = nullptr);

    qi::geometry::Transform transform() const;
    void setTransform(const qi::geometry::Transform& t);

signals:
    void transformChanged(const qi::geometry::Transform& t);

private:
    void emitChanged();

    QDoubleSpinBox* tx_;
    QDoubleSpinBox* ty_;
    QDoubleSpinBox* tz_;
    QDoubleSpinBox* rxDeg_;
    QDoubleSpinBox* ryDeg_;
    QDoubleSpinBox* rzDeg_;
};

}  // namespace qi::ui
