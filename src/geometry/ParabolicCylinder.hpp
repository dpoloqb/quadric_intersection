#pragma once

#include "Quadric.hpp"

namespace qi::geometry {

// Implicit: y^2 - 2*p*x = 0. Axis along Z.
class ParabolicCylinder final : public Quadric {
public:
    explicit ParabolicCylinder(double p);

    double p() const { return p_; }

    double implicit(const Vec3& p) const override;
    Vec3 parametric(double u, double v) const override;
    std::pair<double, double> uRange() const override;
    std::pair<double, double> vRange() const override;
    std::string typeName() const override { return "parabolic_cylinder"; }
    std::unique_ptr<Quadric> clone() const override;

private:
    double p_;
};

}  // namespace qi::geometry
