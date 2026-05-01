#pragma once

#include "MeshIntersector.hpp"

namespace qi::intersection {

// Brute-force O(n·m) triangle-triangle intersector. Tests every pair of
// triangles between the two meshes. Suitable as a baseline and for small
// meshes; for production use prefer BvhIntersector.
class NaiveIntersector final : public MeshIntersector {
public:
    std::vector<qi::mesh::Segment> findSegments(const qi::mesh::Mesh& a,
                                                const qi::mesh::Mesh& b) override;

    std::string methodName() const override { return "naive"; }
};

}  // namespace qi::intersection
