#include "BoundingBoxWidget.hpp"

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>

namespace qi::ui {

using qi::geometry::BoundingBox;
using qi::geometry::Vec3;

namespace {

QDoubleSpinBox* makeSpin(double initial) {
    auto* s = new QDoubleSpinBox;
    s->setRange(-1e6, 1e6);
    s->setDecimals(3);
    s->setSingleStep(0.5);
    s->setValue(initial);
    return s;
}

}  // namespace

BoundingBoxWidget::BoundingBoxWidget(QWidget* parent) : QWidget(parent) {
    minX_ = makeSpin(-3.0);
    minY_ = makeSpin(-3.0);
    minZ_ = makeSpin(-3.0);
    maxX_ = makeSpin(3.0);
    maxY_ = makeSpin(3.0);
    maxZ_ = makeSpin(3.0);

    auto* layout = new QGridLayout(this);
    layout->addWidget(new QLabel(tr("min")), 0, 0);
    layout->addWidget(minX_, 0, 1);
    layout->addWidget(minY_, 0, 2);
    layout->addWidget(minZ_, 0, 3);
    layout->addWidget(new QLabel(tr("max")), 1, 0);
    layout->addWidget(maxX_, 1, 1);
    layout->addWidget(maxY_, 1, 2);
    layout->addWidget(maxZ_, 1, 3);

    using DSB = QDoubleSpinBox;
    auto pair = [&](DSB* lo, DSB* hi) {
        connect(lo, qOverload<double>(&DSB::valueChanged), this, [this, hi](double v) {
            hi->setMinimum(v);
            emitChanged();
        });
        connect(hi, qOverload<double>(&DSB::valueChanged), this, [this, lo](double v) {
            lo->setMaximum(v);
            emitChanged();
        });
    };
    pair(minX_, maxX_);
    pair(minY_, maxY_);
    pair(minZ_, maxZ_);
}

BoundingBox BoundingBoxWidget::boundingBox() const {
    return BoundingBox(Vec3(minX_->value(), minY_->value(), minZ_->value()),
                       Vec3(maxX_->value(), maxY_->value(), maxZ_->value()));
}

void BoundingBoxWidget::setBoundingBox(const BoundingBox& bbox) {
    // Block signals while loading values so we emit bboxChanged once.
    const QSignalBlocker b1(minX_), b2(minY_), b3(minZ_);
    const QSignalBlocker b4(maxX_), b5(maxY_), b6(maxZ_);
    minX_->setMinimum(-1e6); minX_->setMaximum(bbox.max().x());
    minY_->setMinimum(-1e6); minY_->setMaximum(bbox.max().y());
    minZ_->setMinimum(-1e6); minZ_->setMaximum(bbox.max().z());
    maxX_->setMinimum(bbox.min().x()); maxX_->setMaximum(1e6);
    maxY_->setMinimum(bbox.min().y()); maxY_->setMaximum(1e6);
    maxZ_->setMinimum(bbox.min().z()); maxZ_->setMaximum(1e6);
    minX_->setValue(bbox.min().x());
    minY_->setValue(bbox.min().y());
    minZ_->setValue(bbox.min().z());
    maxX_->setValue(bbox.max().x());
    maxY_->setValue(bbox.max().y());
    maxZ_->setValue(bbox.max().z());
    emitChanged();
}

void BoundingBoxWidget::emitChanged() {
    emit bboxChanged(boundingBox());
}

}  // namespace qi::ui
