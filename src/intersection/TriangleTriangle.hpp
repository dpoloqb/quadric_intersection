#pragma once

#include <optional>

#include "Polyline.hpp"
#include "Vec3.hpp"

namespace qi::intersection {

/// Compute the intersection segment of two 3D triangles.
///
/// Implementation follows Devillers & Guigue 2002, "Faster Triangle-Triangle
/// Intersection Tests" (INRIA RR-4488). All branching decisions are signs of
/// the degree-3 `orient3d` predicate — significantly more numerically stable
/// than Möller 1997's degree-8 interval test on borderline configurations.
///
/// Pipeline:
///   1. AABB rejection (Step 0; numerical safeguard against false positives
///      on far-apart triangles).
///   2. Six `orient3d` calls classify each triangle's vertices against the
///      other plane. Reject if all on one side.
///   3. Coplanar (all six = 0) → `nullopt` (TODO per PLAN.md §4 footnote).
///   4. Canonical permutation places the lone vertex of each triangle at
///      position 0; possibly swaps q ↔ r of the OTHER triangle to flip the
///      plane normal. Special case for `(0, ±, ±)` sign pattern (vertex on
///      plane, two on the same strict side) is also handled.
///   5. Two final `orient3d` checks (eq. 1 of the paper) decide intersection.
///   6. If yes, the segment endpoints are constructed by linear interpolation
///      along the four crossing edges and projected onto the line of
///      intersection.
///
/// Returns `std::nullopt` when:
///   - the triangles do not intersect (Steps 0 / 2 reject),
///   - they are coplanar (Step 3),
///   - either triangle is degenerate (zero cross-product magnitude).
///
/// Otherwise returns the `Segment`. It can be zero-length when triangles
/// touch at a single vertex.
std::optional<qi::mesh::Segment> intersectTriangles(const qi::geometry::Vec3& a1,
                                                    const qi::geometry::Vec3& b1,
                                                    const qi::geometry::Vec3& c1,
                                                    const qi::geometry::Vec3& a2,
                                                    const qi::geometry::Vec3& b2,
                                                    const qi::geometry::Vec3& c2);

}  // namespace qi::intersection
