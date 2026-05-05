#include "HyperbolicCylinder.hpp"

#include <cmath>

namespace qi::geometry {

namespace {
// Half-extent of the t-parameter on a single branch (sinh(2.5) ≈ 6.05).
// uRange is twice that since u packs both branches end-to-end.
constexpr double kTExtent = 2.5;
constexpr double kVExtent = 10.0;
}  // namespace

HyperbolicCylinder::HyperbolicCylinder(double a, double b) : a_(a), b_(b) {}

double HyperbolicCylinder::implicit(const Vec3& p) const {
    const Vec3 q = transform_.applyInverse(p);
    const double xa = q.x() / a_;
    const double yb = q.y() / b_;
    return xa * xa - yb * yb - 1.0;
}

// u ∈ [-2T, 2T]. We pack two disjoint branches into one parameter:
//   u ∈ [-2T, 0)  → left branch:  t = u + T,   x = -a·cosh(t),  y = b·sinh(t)
//   u ∈ [0,  2T]  → right branch: t = u - T,   x = +a·cosh(t),  y = b·sinh(t)
// Each branch is a single curve in the xy-plane that sweeps y from -b·sinh(T)
// to +b·sinh(T) as t ranges over [-T, T]; using `t = abs(u)` (the previous
// parametrization) collapsed the lower halves and only rendered y ≥ 0.
// The branch switch at u=0 is reported via uDiscontinuities() so the
// triangulator skips the bridging quad.
Vec3 HyperbolicCylinder::parametric(double u, double v) const {
    const double signX = (u >= 0.0) ? 1.0 : -1.0;
    const double t = (u >= 0.0) ? u - kTExtent : u + kTExtent;
    const double x = signX * a_ * std::cosh(t);
    const double y = b_ * std::sinh(t);
    const double z = v;
    return transform_.apply(Vec3(x, y, z));
}

std::pair<double, double> HyperbolicCylinder::uRange() const {
    return {-2.0 * kTExtent, 2.0 * kTExtent};
}
std::pair<double, double> HyperbolicCylinder::vRange() const { return {-kVExtent, kVExtent}; }

std::unique_ptr<Quadric> HyperbolicCylinder::clone() const {
    auto copy = std::make_unique<HyperbolicCylinder>(a_, b_);
    copy->setTransform(transform_);
    return copy;
}

}  // namespace qi::geometry
