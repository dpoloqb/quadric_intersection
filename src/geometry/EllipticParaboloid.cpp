#include "EllipticParaboloid.hpp"

#include <cmath>

namespace qi::geometry {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kVExtent = 5.0;
}  // namespace

EllipticParaboloid::EllipticParaboloid(double a, double b) : a_(a), b_(b) {}

double EllipticParaboloid::implicit(const Vec3& p) const {
    const Vec3 q = transform_.applyInverse(p);
    const double xa = q.x() / a_;
    const double yb = q.y() / b_;
    return xa * xa + yb * yb - q.z();
}

Vec3 EllipticParaboloid::parametric(double u, double v) const {
    const double x = a_ * v * std::cos(u);
    const double y = b_ * v * std::sin(u);
    const double z = v * v;
    return transform_.apply(Vec3(x, y, z));
}

std::pair<double, double> EllipticParaboloid::uRange() const { return {0.0, 2.0 * kPi}; }
std::pair<double, double> EllipticParaboloid::vRange() const { return {0.0, kVExtent}; }

std::unique_ptr<Quadric> EllipticParaboloid::clone() const {
    auto copy = std::make_unique<EllipticParaboloid>(a_, b_);
    copy->setTransform(transform_);
    return copy;
}

}  // namespace qi::geometry
