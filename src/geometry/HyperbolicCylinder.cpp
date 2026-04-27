#include "HyperbolicCylinder.hpp"

#include <cmath>

namespace qi::geometry {

namespace {
constexpr double kUExtent = 2.5;   // sinh(2.5) ≈ 6.05.
constexpr double kVExtent = 10.0;
}  // namespace

HyperbolicCylinder::HyperbolicCylinder(double a, double b) : a_(a), b_(b) {}

double HyperbolicCylinder::implicit(const Vec3& p) const {
    const Vec3 q = transform_.applyInverse(p);
    const double xa = q.x() / a_;
    const double yb = q.y() / b_;
    return xa * xa - yb * yb - 1.0;
}

Vec3 HyperbolicCylinder::parametric(double u, double v) const {
    const double t = std::abs(u);
    const double signX = (u >= 0.0) ? 1.0 : -1.0;
    const double x = signX * a_ * std::cosh(t);
    const double y = b_ * std::sinh(t);
    const double z = v;
    return transform_.apply(Vec3(x, y, z));
}

std::pair<double, double> HyperbolicCylinder::uRange() const { return {-kUExtent, kUExtent}; }
std::pair<double, double> HyperbolicCylinder::vRange() const { return {-kVExtent, kVExtent}; }

std::unique_ptr<Quadric> HyperbolicCylinder::clone() const {
    auto copy = std::make_unique<HyperbolicCylinder>(a_, b_);
    copy->setTransform(transform_);
    return copy;
}

}  // namespace qi::geometry
