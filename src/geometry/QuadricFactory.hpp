#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Quadric.hpp"

namespace qi::geometry {

/// Generic parameter bag for any of the 9 canonical quadrics.
///
/// Each derived class consumes the subset it needs:
///   - `ellipsoid`, `hyperboloid_one_sheet`, `hyperboloid_two_sheet`, `cone`:
///     `a`, `b`, `c`
///   - `elliptic_paraboloid`, `hyperbolic_paraboloid`, `elliptic_cylinder`,
///     `hyperbolic_cylinder`: `a`, `b`
///   - `parabolic_cylinder`: `p`
struct QuadricParams {
    double a = 1.0;
    double b = 1.0;
    double c = 1.0;
    double p = 1.0;
};

/// Construct a `Quadric` by type name. Accepted names match `Quadric::typeName()`:
/// `"ellipsoid"`, `"hyperboloid_one_sheet"`, `"hyperboloid_two_sheet"`,
/// `"elliptic_paraboloid"`, `"hyperbolic_paraboloid"`, `"cone"`,
/// `"elliptic_cylinder"`, `"hyperbolic_cylinder"`, `"parabolic_cylinder"`.
///
/// @throws std::invalid_argument when `type` is not in that list.
std::unique_ptr<Quadric> createQuadric(const std::string& type, const QuadricParams& params);

/// All type names supported by `createQuadric`, in canonical order.
std::vector<std::string> knownQuadricTypes();

}  // namespace qi::geometry
