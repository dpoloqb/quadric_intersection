#pragma once

#include "Quadric.hpp"

namespace qi::geometry {

// Implicit: x^2/a^2 - y^2/b^2 - 1 = 0. Axis along Z.
// Two branches: x >= a (u >= 0) and x <= -a (u < 0).
class HyperbolicCylinder final : public Quadric {
public:
    HyperbolicCylinder(double a, double b);

    double a() const { return a_; }
    double b() const { return b_; }

    double implicit(const Vec3& p) const override;
    Vec3 parametric(double u, double v) const override;
    std::pair<double, double> uRange() const override;
    std::pair<double, double> vRange() const override;
    std::string typeName() const override { return "hyperbolic_cylinder"; }
    std::unique_ptr<Quadric> clone() const override;

private:
    double a_;
    double b_;
};

}  // namespace qi::geometry
