#include "ParabolicCylinder.hpp"

namespace qi::geometry {

namespace {
constexpr double kUExtent = 5.0;
constexpr double kVExtent = 10.0;
}  // namespace

ParabolicCylinder::ParabolicCylinder(double p) : p_(p) {}

double ParabolicCylinder::implicit(const Vec3& p) const {
    const Vec3 q = transform_.applyInverse(p);
    return q.y() * q.y() - 2.0 * p_ * q.x();
}

Vec3 ParabolicCylinder::parametric(double u, double v) const {
    const double x = (u * u) / (2.0 * p_);
    const double y = u;
    const double z = v;
    return transform_.apply(Vec3(x, y, z));
}

std::pair<double, double> ParabolicCylinder::uRange() const { return {-kUExtent, kUExtent}; }
std::pair<double, double> ParabolicCylinder::vRange() const { return {-kVExtent, kVExtent}; }

std::unique_ptr<Quadric> ParabolicCylinder::clone() const {
    auto copy = std::make_unique<ParabolicCylinder>(p_);
    copy->setTransform(transform_);
    return copy;
}

}  // namespace qi::geometry
