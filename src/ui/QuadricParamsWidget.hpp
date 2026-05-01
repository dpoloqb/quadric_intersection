#pragma once

#include <QString>
#include <QWidget>
#include <unordered_map>

#include "QuadricFactory.hpp"

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
class QStackedWidget;
QT_END_NAMESPACE

namespace qi::ui {

// QStackedWidget with one page per quadric type (9 total). Each page has
// the spinboxes for the parameters that type uses (a/b/c for ellipsoid-like;
// a/b for cylinders/paraboloids; p for parabolic_cylinder).
class QuadricParamsWidget : public QWidget {
    Q_OBJECT
public:
    explicit QuadricParamsWidget(QWidget* parent = nullptr);

    QString type() const { return currentType_; }
    qi::geometry::QuadricParams params() const;

    void setType(const QString& type);
    void setParams(const qi::geometry::QuadricParams& p);

signals:
    void parametersChanged();

private:
    struct Page {
        int index = -1;
        QDoubleSpinBox* a = nullptr;
        QDoubleSpinBox* b = nullptr;
        QDoubleSpinBox* c = nullptr;
        QDoubleSpinBox* p = nullptr;
    };

    void addAbcPage(const QString& type, bool useC);   // a, b, [c] page
    void addPParamPage(const QString& type);            // single p

    QStackedWidget* stack_;
    QString currentType_;
    std::unordered_map<std::string, Page> pages_;  // by type name
};

}  // namespace qi::ui
