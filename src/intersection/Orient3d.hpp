#pragma once

#include "Vec3.hpp"

namespace qi::intersection {

// Sign of the [a, b, c, d] determinant from Devillers & Guigue 2002.
// Equivalent to sign of (a-d) · ((b-d) × (c-d)).
//
// Geometric interpretation: returns +1 if d is on the side of plane(a,b,c)
// pointed to by the right-hand normal (b-a) × (c-a), -1 if on the opposite
// side, 0 if d is coplanar with (a, b, c) within tolerance.
//
// This is the only branching primitive used by the Devillers-Guigue
// triangle-triangle intersection algorithm. Polynomial degree 3 in the
// inputs — significantly more numerically stable than the degree-8
// expressions that arise from the Möller 1997 interval test.
int orient3d(const qi::geometry::Vec3& a,
             const qi::geometry::Vec3& b,
             const qi::geometry::Vec3& c,
             const qi::geometry::Vec3& d);

}  // namespace qi::intersection
