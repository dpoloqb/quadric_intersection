#pragma once

#include "Quadric.hpp"

namespace qi::geometry {

// Implicit: -x^2/a^2 - y^2/b^2 + z^2/c^2 - 1 = 0
// Two sheets: z >= c (upper) and z <= -c (lower).
// Parametric uses sign(v) as sheet selector: v >= 0 -> upper, v < 0 -> lower.
class HyperboloidTwoSheet final : public Quadric {
public:
    HyperboloidTwoSheet(double a, double b, double c);

    double a() const { return a_; }
    double b() const { return b_; }
    double c() const { return c_; }

    double implicit(const Vec3& p) const override;
    Vec3 parametric(double u, double v) const override;
    std::pair<double, double> uRange() const override;
    std::pair<double, double> vRange() const override;
    std::string typeName() const override { return "hyperboloid_two_sheet"; }
    std::unique_ptr<Quadric> clone() const override;

private:
    double a_;
    double b_;
    double c_;
};

}  // namespace qi::geometry
