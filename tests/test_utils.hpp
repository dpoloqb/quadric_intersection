#pragma once

#include <gtest/gtest.h>

#include "Quadric.hpp"
#include "Vec3.hpp"

namespace qi::test {

constexpr double kEpsExact = 1e-12;
constexpr double kEpsTight = 1e-9;
constexpr double kEpsLoose = 1e-6;
constexpr double kEpsGeom = 1e-3;

constexpr double kPi = 3.14159265358979323846;

// Sweeps a (uSteps+1) × (vSteps+1) grid in (uRange, vRange), and checks that
// every parametric point satisfies |implicit(p)| < tolerance. Used to validate
// that parametric() and implicit() agree for any quadric.
inline void expectParametricLiesOnImplicit(const qi::geometry::Quadric& q,
                                           double tolerance,
                                           int uSteps = 10,
                                           int vSteps = 10) {
    const auto [uMin, uMax] = q.uRange();
    const auto [vMin, vMax] = q.vRange();
    for (int i = 0; i <= uSteps; ++i) {
        for (int j = 0; j <= vSteps; ++j) {
            const double u = uMin + (uMax - uMin) * static_cast<double>(i) / uSteps;
            const double v = vMin + (vMax - vMin) * static_cast<double>(j) / vSteps;
            const qi::geometry::Vec3 p = q.parametric(u, v);
            EXPECT_NEAR(q.implicit(p), 0.0, tolerance)
                << q.typeName() << " u=" << u << " v=" << v
                << " p=" << p.transpose();
        }
    }
}

}  // namespace qi::test
