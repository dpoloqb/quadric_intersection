#include "HyperbolicParaboloid.hpp"

namespace qi::geometry {

namespace {
constexpr double kExtent = 5.0;
}  // namespace

HyperbolicParaboloid::HyperbolicParaboloid(double a, double b) : a_(a), b_(b) {}

double HyperbolicParaboloid::implicit(const Vec3& p) const {
    const Vec3 q = transform_.applyInverse(p);
    const double xa = q.x() / a_;
    const double yb = q.y() / b_;
    return xa * xa - yb * yb - q.z();
}

Vec3 HyperbolicParaboloid::parametric(double u, double v) const {
    const double x = a_ * u;
    const double y = b_ * v;
    const double z = u * u - v * v;
    return transform_.apply(Vec3(x, y, z));
}

std::pair<double, double> HyperbolicParaboloid::uRange() const { return {-kExtent, kExtent}; }
std::pair<double, double> HyperbolicParaboloid::vRange() const { return {-kExtent, kExtent}; }

std::unique_ptr<Quadric> HyperbolicParaboloid::clone() const {
    auto copy = std::make_unique<HyperbolicParaboloid>(a_, b_);
    copy->setTransform(transform_);
    return copy;
}

}  // namespace qi::geometry
