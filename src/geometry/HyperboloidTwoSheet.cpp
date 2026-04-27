#include "HyperboloidTwoSheet.hpp"

#include <cmath>

namespace qi::geometry {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kVExtent = 2.5;
}  // namespace

HyperboloidTwoSheet::HyperboloidTwoSheet(double a, double b, double c)
    : a_(a), b_(b), c_(c) {}

double HyperboloidTwoSheet::implicit(const Vec3& p) const {
    const Vec3 q = transform_.applyInverse(p);
    const double xa = q.x() / a_;
    const double yb = q.y() / b_;
    const double zc = q.z() / c_;
    return -xa * xa - yb * yb + zc * zc - 1.0;
}

Vec3 HyperboloidTwoSheet::parametric(double u, double v) const {
    const double t = std::abs(v);
    const double sign = (v >= 0.0) ? 1.0 : -1.0;
    const double st = std::sinh(t);
    const double ct = std::cosh(t);
    const double x = a_ * st * std::cos(u);
    const double y = b_ * st * std::sin(u);
    const double z = sign * c_ * ct;
    return transform_.apply(Vec3(x, y, z));
}

std::pair<double, double> HyperboloidTwoSheet::uRange() const { return {0.0, 2.0 * kPi}; }
std::pair<double, double> HyperboloidTwoSheet::vRange() const { return {-kVExtent, kVExtent}; }

std::unique_ptr<Quadric> HyperboloidTwoSheet::clone() const {
    auto copy = std::make_unique<HyperboloidTwoSheet>(a_, b_, c_);
    copy->setTransform(transform_);
    return copy;
}

}  // namespace qi::geometry
