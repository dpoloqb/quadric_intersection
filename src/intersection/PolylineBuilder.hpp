#pragma once

#include <vector>

#include "Polyline.hpp"

namespace qi::intersection {

// Stitches an unordered collection of 3D line segments into polylines by
// connecting segments that share an endpoint (within `epsilon` distance).
//
// Each connected component of the segment graph becomes one Polyline. The
// `closed` flag is set to true if the component forms a cycle (the chain
// returns to its starting point); for closed polylines the first point is
// repeated at the end so the points list visits every edge exactly once.
//
// Degenerate segments (a == b within epsilon) are silently skipped.
//
// Default epsilon = kEpsLoose-equivalent (1e-6) — appropriate for points
// derived from triangle-triangle intersections of typical-scale meshes.
std::vector<qi::mesh::Polyline> buildPolylines(
    const std::vector<qi::mesh::Segment>& segments,
    double epsilon = 1e-6);

}  // namespace qi::intersection
