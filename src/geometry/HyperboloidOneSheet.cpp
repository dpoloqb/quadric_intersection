#include "HyperboloidOneSheet.hpp"

#include <cmath>

namespace qi::geometry {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kVExtent = 2.5;  // sinh(2.5) ≈ 6.05; large enough for typical bboxes.
}  // namespace

HyperboloidOneSheet::HyperboloidOneSheet(double a, double b, double c)
    : a_(a), b_(b), c_(c) {}

double HyperboloidOneSheet::implicit(const Vec3& p) const {
    const Vec3 q = transform_.applyInverse(p);
    const double xa = q.x() / a_;
    const double yb = q.y() / b_;
    const double zc = q.z() / c_;
    return xa * xa + yb * yb - zc * zc - 1.0;
}

Vec3 HyperboloidOneSheet::parametric(double u, double v) const {
    const double cv = std::cosh(v);
    const double sv = std::sinh(v);
    const double x = a_ * cv * std::cos(u);
    const double y = b_ * cv * std::sin(u);
    const double z = c_ * sv;
    return transform_.apply(Vec3(x, y, z));
}

std::pair<double, double> HyperboloidOneSheet::uRange() const { return {0.0, 2.0 * kPi}; }
std::pair<double, double> HyperboloidOneSheet::vRange() const { return {-kVExtent, kVExtent}; }

std::unique_ptr<Quadric> HyperboloidOneSheet::clone() const {
    auto copy = std::make_unique<HyperboloidOneSheet>(a_, b_, c_);
    copy->setTransform(transform_);
    return copy;
}

}  // namespace qi::geometry
