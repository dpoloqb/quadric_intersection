#include "QuadricParamsWidget.hpp"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace qi::ui {

using qi::geometry::QuadricParams;

namespace {

QDoubleSpinBox* makeParamSpin(double initial) {
    auto* s = new QDoubleSpinBox;
    s->setRange(0.001, 1e3);
    s->setDecimals(3);
    s->setSingleStep(0.5);
    s->setValue(initial);
    return s;
}

}  // namespace

QuadricParamsWidget::QuadricParamsWidget(QWidget* parent) : QWidget(parent) {
    stack_ = new QStackedWidget;
    auto* main = new QVBoxLayout(this);
    main->addWidget(stack_);
    main->setContentsMargins(0, 0, 0, 0);

    // a, b, c parameters used by ellipsoid, hyperboloids, cone.
    addAbcPage("ellipsoid", /*useC=*/true);
    addAbcPage("hyperboloid_one_sheet", true);
    addAbcPage("hyperboloid_two_sheet", true);
    addAbcPage("cone", true);

    // a, b only.
    addAbcPage("elliptic_paraboloid", false);
    addAbcPage("hyperbolic_paraboloid", false);
    addAbcPage("elliptic_cylinder", false);
    addAbcPage("hyperbolic_cylinder", false);

    // p only.
    addPParamPage("parabolic_cylinder");

    currentType_ = "ellipsoid";
    stack_->setCurrentIndex(pages_[currentType_.toStdString()].index);
}

void QuadricParamsWidget::addAbcPage(const QString& type, bool useC) {
    auto* page = new QWidget;
    auto* layout = new QFormLayout(page);
    Page p;
    p.a = makeParamSpin(1.0);
    p.b = makeParamSpin(1.0);
    layout->addRow("a", p.a);
    layout->addRow("b", p.b);
    if (useC) {
        p.c = makeParamSpin(1.0);
        layout->addRow("c", p.c);
    }
    p.index = stack_->addWidget(page);
    pages_[type.toStdString()] = p;

    using DSB = QDoubleSpinBox;
    auto signalize = [this](DSB* s) {
        if (!s) return;
        connect(s, qOverload<double>(&DSB::valueChanged), this,
                [this](double) { emit parametersChanged(); });
    };
    signalize(p.a);
    signalize(p.b);
    signalize(p.c);
}

void QuadricParamsWidget::addPParamPage(const QString& type) {
    auto* page = new QWidget;
    auto* layout = new QFormLayout(page);
    Page p;
    p.p = makeParamSpin(1.0);
    layout->addRow("p", p.p);
    p.index = stack_->addWidget(page);
    pages_[type.toStdString()] = p;

    using DSB = QDoubleSpinBox;
    connect(p.p, qOverload<double>(&DSB::valueChanged), this,
            [this](double) { emit parametersChanged(); });
}

QuadricParams QuadricParamsWidget::params() const {
    QuadricParams out;
    auto it = pages_.find(currentType_.toStdString());
    if (it == pages_.end()) return out;
    const Page& p = it->second;
    if (p.a) out.a = p.a->value();
    if (p.b) out.b = p.b->value();
    if (p.c) out.c = p.c->value();
    if (p.p) out.p = p.p->value();
    return out;
}

void QuadricParamsWidget::setType(const QString& type) {
    auto it = pages_.find(type.toStdString());
    if (it == pages_.end()) return;
    currentType_ = type;
    stack_->setCurrentIndex(it->second.index);
    emit parametersChanged();
}

void QuadricParamsWidget::setParams(const QuadricParams& p) {
    auto it = pages_.find(currentType_.toStdString());
    if (it == pages_.end()) return;
    Page& page = it->second;
    if (page.a) {
        const QSignalBlocker b(page.a);
        page.a->setValue(p.a);
    }
    if (page.b) {
        const QSignalBlocker b(page.b);
        page.b->setValue(p.b);
    }
    if (page.c) {
        const QSignalBlocker b(page.c);
        page.c->setValue(p.c);
    }
    if (page.p) {
        const QSignalBlocker b(page.p);
        page.p->setValue(p.p);
    }
    emit parametersChanged();
}

}  // namespace qi::ui
