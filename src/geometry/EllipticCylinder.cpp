#include "EllipticCylinder.hpp"

#include <cmath>

namespace qi::geometry {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kVExtent = 10.0;
}  // namespace

EllipticCylinder::EllipticCylinder(double a, double b) : a_(a), b_(b) {}

double EllipticCylinder::implicit(const Vec3& p) const {
    const Vec3 q = transform_.applyInverse(p);
    const double xa = q.x() / a_;
    const double yb = q.y() / b_;
    return xa * xa + yb * yb - 1.0;
}

Vec3 EllipticCylinder::parametric(double u, double v) const {
    const double x = a_ * std::cos(u);
    const double y = b_ * std::sin(u);
    const double z = v;
    return transform_.apply(Vec3(x, y, z));
}

std::pair<double, double> EllipticCylinder::uRange() const { return {0.0, 2.0 * kPi}; }
std::pair<double, double> EllipticCylinder::vRange() const { return {-kVExtent, kVExtent}; }

std::unique_ptr<Quadric> EllipticCylinder::clone() const {
    auto copy = std::make_unique<EllipticCylinder>(a_, b_);
    copy->setTransform(transform_);
    return copy;
}

}  // namespace qi::geometry
