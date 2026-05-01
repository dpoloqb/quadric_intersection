#pragma once

#include <string>
#include <vector>

#include "Mesh.hpp"
#include "Polyline.hpp"

namespace qi::intersection {

/// Strategy interface for "find every triangle-triangle intersection between
/// two meshes". Concrete implementations differ only in how they iterate the
/// triangle pairs (`NaiveIntersector` is O(n·m); `BvhIntersector` is
/// O((n+m) log n) on uniform inputs).
///
/// Output is an unordered list of segments, one per intersecting pair. Use
/// `qi::intersection::buildPolylines` to stitch them into chains.
class MeshIntersector {
public:
    virtual ~MeshIntersector() = default;

    /// Compute all intersection segments between the triangles of `a` and `b`.
    virtual std::vector<qi::mesh::Segment> findSegments(const qi::mesh::Mesh& a,
                                                        const qi::mesh::Mesh& b) = 0;

    /// Stable identifier (`"naive"` / `"bvh"`) used in `IntersectionRecord`
    /// and the SQLite `intersections.intersection_method` column.
    virtual std::string methodName() const = 0;
};

}  // namespace qi::intersection
