#pragma once

#include <vector>

#include "BoundingBox.hpp"
#include "Mesh.hpp"
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

// Visualization-only view of one BVH node: its world-space AABB plus where it
// sits in the tree. Depth 0 is the root; `isLeaf` follows from the same
// build parameters as `BvhIntersector::findSegments`.
struct BvhVizNode {
    qi::geometry::BoundingBox bbox;
    int depth = 0;
    bool isLeaf = false;
};

// Build a BVH over `mesh` (same parameters as the intersector — median split,
// leaf size 8) and return its full node list. Empty mesh → empty result.
std::vector<BvhVizNode> buildBvhForVisualization(const qi::mesh::Mesh& mesh);

}  // namespace qi::intersection
