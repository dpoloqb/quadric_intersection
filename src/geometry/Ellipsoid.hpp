#pragma once

#include "Quadric.hpp"

namespace qi::geometry {

class Ellipsoid final : public Quadric {
public:
    Ellipsoid(double a, double b, double c);

    double a() const { return a_; }
    double b() const { return b_; }
    double c() const { return c_; }

    double implicit(const Vec3& p) const override;
    Vec3 parametric(double u, double v) const override;
    std::pair<double, double> uRange() const override;
    std::pair<double, double> vRange() const override;
    std::string typeName() const override { return "ellipsoid"; }
    std::unique_ptr<Quadric> clone() const override;

private:
    double a_;
    double b_;
    double c_;
};

}  // namespace qi::geometry
