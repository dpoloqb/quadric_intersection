#include "Cone.hpp"

#include <cmath>

namespace qi::geometry {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kVExtent = 5.0;
}  // namespace

Cone::Cone(double a, double b, double c) : a_(a), b_(b), c_(c) {}

double Cone::implicit(const Vec3& p) const {
    const Vec3 q = transform_.applyInverse(p);
    const double xa = q.x() / a_;
    const double yb = q.y() / b_;
    const double zc = q.z() / c_;
    return xa * xa + yb * yb - zc * zc;
}

Vec3 Cone::parametric(double u, double v) const {
    const double x = a_ * v * std::cos(u);
    const double y = b_ * v * std::sin(u);
    const double z = c_ * v;
    return transform_.apply(Vec3(x, y, z));
}

std::pair<double, double> Cone::uRange() const { return {0.0, 2.0 * kPi}; }
std::pair<double, double> Cone::vRange() const { return {-kVExtent, kVExtent}; }

std::unique_ptr<Quadric> Cone::clone() const {
    auto copy = std::make_unique<Cone>(a_, b_, c_);
    copy->setTransform(transform_);
    return copy;
}

}  // namespace qi::geometry
