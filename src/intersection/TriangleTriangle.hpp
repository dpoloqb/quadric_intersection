#pragma once

#include <optional>

#include "Polyline.hpp"
#include "Vec3.hpp"

namespace qi::intersection {

// Computes the intersection segment of two 3D triangles.
//
// Implementation follows Möller 1997 "A Fast Triangle-Triangle Intersection
// Test": tests for separation by each triangle's plane, then collects the
// edge-crossings and finds the overlap of the two resulting collinear segments.
//
// Returns std::nullopt when:
//   - the triangles do not intersect,
//   - they are coplanar (degenerate case, marked TODO in PLAN.md),
//   - either triangle is degenerate (zero area).
//
// Otherwise returns the intersection Segment.  The segment can be a single
// point (zero length) when the triangles touch at a vertex or along an edge.
std::optional<qi::mesh::Segment> intersectTriangles(const qi::geometry::Vec3& a1,
                                                    const qi::geometry::Vec3& b1,
                                                    const qi::geometry::Vec3& c1,
                                                    const qi::geometry::Vec3& a2,
                                                    const qi::geometry::Vec3& b2,
                                                    const qi::geometry::Vec3& c2);

}  // namespace qi::intersection
