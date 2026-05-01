#include "TransformWidget.hpp"

#include <Eigen/Geometry>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <cmath>

#include "Vec3.hpp"

namespace qi::ui {

using qi::geometry::Quat;
using qi::geometry::Transform;
using qi::geometry::Vec3;

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDegToRad = kPi / 180.0;
constexpr double kRadToDeg = 180.0 / kPi;

QDoubleSpinBox* makeTransSpin() {
    auto* s = new QDoubleSpinBox;
    s->setRange(-1e6, 1e6);
    s->setDecimals(3);
    s->setSingleStep(0.5);
    s->setValue(0.0);
    return s;
}

QDoubleSpinBox* makeAngleSpin() {
    auto* s = new QDoubleSpinBox;
    s->setRange(-360.0, 360.0);
    s->setDecimals(4);
    s->setSingleStep(5.0);
    s->setSuffix("°");
    s->setValue(0.0);
    return s;
}

// Convention: rotation is R = Rz(rz) * Ry(ry) * Rx(rx), i.e. apply X first,
// then Y, then Z. Decomposition formulas derived directly from the resulting
// rotation matrix (no dependency on Eigen's deprecated eulerAngles helper).
Quat eulerXyzDegToQuat(double rxDeg, double ryDeg, double rzDeg) {
    const Eigen::AngleAxisd rx(rxDeg * kDegToRad, Vec3::UnitX());
    const Eigen::AngleAxisd ry(ryDeg * kDegToRad, Vec3::UnitY());
    const Eigen::AngleAxisd rz(rzDeg * kDegToRad, Vec3::UnitZ());
    return (rz * ry * rx);
}

void quatToEulerXyzDeg(const Quat& q, double& rxDeg, double& ryDeg, double& rzDeg) {
    // R = Rz(rz) * Ry(ry) * Rx(rx) gives:
    //   R(2,0) = -sin(ry)
    //   R(2,1) = sin(rx)*cos(ry),    R(2,2) = cos(rx)*cos(ry)
    //   R(1,0) = sin(rz)*cos(ry),    R(0,0) = cos(rz)*cos(ry)
    // Gimbal-lock guard: when |R(2,0)| ≈ 1 (cos(ry) ≈ 0), pick rz = 0 and
    // recover rx from the remaining columns.
    const Eigen::Matrix3d r = q.toRotationMatrix();
    const double sy = -r(2, 0);
    const double clamped = std::clamp(sy, -1.0, 1.0);
    const double ry = std::asin(clamped);
    double rx, rz;
    if (std::abs(clamped) > 1.0 - 1e-9) {
        rz = 0.0;
        rx = std::atan2(-r(0, 1), r(1, 1));
    } else {
        rx = std::atan2(r(2, 1), r(2, 2));
        rz = std::atan2(r(1, 0), r(0, 0));
    }
    rxDeg = rx * kRadToDeg;
    ryDeg = ry * kRadToDeg;
    rzDeg = rz * kRadToDeg;
}

}  // namespace

TransformWidget::TransformWidget(QWidget* parent) : QWidget(parent) {
    tx_ = makeTransSpin();
    ty_ = makeTransSpin();
    tz_ = makeTransSpin();
    rxDeg_ = makeAngleSpin();
    ryDeg_ = makeAngleSpin();
    rzDeg_ = makeAngleSpin();

    auto* layout = new QGridLayout(this);
    layout->addWidget(new QLabel(tr("translation")), 0, 0);
    layout->addWidget(tx_, 0, 1);
    layout->addWidget(ty_, 0, 2);
    layout->addWidget(tz_, 0, 3);
    layout->addWidget(new QLabel(tr("rotation (XYZ)")), 1, 0);
    layout->addWidget(rxDeg_, 1, 1);
    layout->addWidget(ryDeg_, 1, 2);
    layout->addWidget(rzDeg_, 1, 3);

    using DSB = QDoubleSpinBox;
    for (auto* s : {tx_, ty_, tz_, rxDeg_, ryDeg_, rzDeg_}) {
        connect(s, qOverload<double>(&DSB::valueChanged), this,
                [this](double) { emitChanged(); });
    }
}

Transform TransformWidget::transform() const {
    return Transform(Vec3(tx_->value(), ty_->value(), tz_->value()),
                     eulerXyzDegToQuat(rxDeg_->value(), ryDeg_->value(),
                                       rzDeg_->value()));
}

void TransformWidget::setTransform(const Transform& t) {
    const QSignalBlocker bs[6] = {
        QSignalBlocker(tx_), QSignalBlocker(ty_), QSignalBlocker(tz_),
        QSignalBlocker(rxDeg_), QSignalBlocker(ryDeg_), QSignalBlocker(rzDeg_),
    };
    tx_->setValue(t.translation().x());
    ty_->setValue(t.translation().y());
    tz_->setValue(t.translation().z());
    double rx = 0, ry = 0, rz = 0;
    quatToEulerXyzDeg(t.rotation(), rx, ry, rz);
    rxDeg_->setValue(rx);
    ryDeg_->setValue(ry);
    rzDeg_->setValue(rz);
    emitChanged();
}

void TransformWidget::emitChanged() {
    emit transformChanged(transform());
}

}  // namespace qi::ui
