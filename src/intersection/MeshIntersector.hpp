#pragma once

#include <string>
#include <vector>

#include "Mesh.hpp"
#include "Polyline.hpp"

namespace qi::intersection {

class MeshIntersector {
public:
    virtual ~MeshIntersector() = default;

    // Returns the unordered set of intersection segments, one per pair of
    // crossing triangles between meshes a and b. Stitching into polylines is
    // a separate step (see PolylineBuilder).
    virtual std::vector<qi::mesh::Segment> findSegments(const qi::mesh::Mesh& a,
                                                        const qi::mesh::Mesh& b) = 0;

    virtual std::string methodName() const = 0;
};

}  // namespace qi::intersection
