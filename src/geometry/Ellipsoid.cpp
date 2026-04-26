#include "Ellipsoid.hpp"

#include <cmath>

namespace qi::geometry {

namespace {
constexpr double kPi = 3.14159265358979323846;
}  // namespace

Ellipsoid::Ellipsoid(double a, double b, double c) : a_(a), b_(b), c_(c) {}

double Ellipsoid::implicit(const Vec3& p) const {
    const Vec3 q = transform_.applyInverse(p);
    const double xa = q.x() / a_;
    const double yb = q.y() / b_;
    const double zc = q.z() / c_;
    return xa * xa + yb * yb + zc * zc - 1.0;
}

Vec3 Ellipsoid::parametric(double u, double v) const {
    const double x = a_ * std::sin(v) * std::cos(u);
    const double y = b_ * std::sin(v) * std::sin(u);
    const double z = c_ * std::cos(v);
    return transform_.apply(Vec3(x, y, z));
}

std::pair<double, double> Ellipsoid::uRange() const { return {0.0, 2.0 * kPi}; }
std::pair<double, double> Ellipsoid::vRange() const { return {0.0, kPi}; }

std::unique_ptr<Quadric> Ellipsoid::clone() const {
    auto copy = std::make_unique<Ellipsoid>(a_, b_, c_);
    copy->setTransform(transform_);
    return copy;
}

}  // namespace qi::geometry
