#pragma once

#include "Quadric.hpp"

namespace qi::geometry {

// Implicit: x^2/a^2 - y^2/b^2 - z = 0 (saddle).
class HyperbolicParaboloid final : public Quadric {
public:
    HyperbolicParaboloid(double a, double b);

    double a() const { return a_; }
    double b() const { return b_; }

    double implicit(const Vec3& p) const override;
    Vec3 parametric(double u, double v) const override;
    std::pair<double, double> uRange() const override;
    std::pair<double, double> vRange() const override;
    std::string typeName() const override { return "hyperbolic_paraboloid"; }
    std::unique_ptr<Quadric> clone() const override;

private:
    double a_;
    double b_;
};

}  // namespace qi::geometry
