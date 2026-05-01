#pragma once

#include "MeshIntersector.hpp"

namespace qi::intersection {

// Bounding-Volume-Hierarchy-accelerated intersector. Builds an AABB tree over
// the triangles of mesh A, then for each triangle of mesh B queries the tree
// for overlapping AABBs and only tests those pairs with the
// triangle-triangle predicate. Asymptotically O((n + m) log n) on uniformly
// distributed meshes vs Naive's O(n·m).
class BvhIntersector final : public MeshIntersector {
public:
    std::vector<qi::mesh::Segment> findSegments(const qi::mesh::Mesh& a,
                                                const qi::mesh::Mesh& b) override;

    std::string methodName() const override { return "bvh"; }
};

}  // namespace qi::intersection
