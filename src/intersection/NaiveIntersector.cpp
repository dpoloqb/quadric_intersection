#include "NaiveIntersector.hpp"

#include "TriangleTriangle.hpp"

namespace qi::intersection {

using qi::geometry::Vec3;
using qi::mesh::Mesh;
using qi::mesh::Segment;

std::vector<Segment> NaiveIntersector::findSegments(const Mesh& a, const Mesh& b) {
    std::vector<Segment> result;
    result.reserve(a.triangleCount());  // optimistic guess

    for (const auto& tA : a.triangles) {
        const Vec3& a1 = a.vertices[tA[0]];
        const Vec3& b1 = a.vertices[tA[1]];
        const Vec3& c1 = a.vertices[tA[2]];

        for (const auto& tB : b.triangles) {
            const Vec3& a2 = b.vertices[tB[0]];
            const Vec3& b2 = b.vertices[tB[1]];
            const Vec3& c2 = b.vertices[tB[2]];

            if (auto seg = intersectTriangles(a1, b1, c1, a2, b2, c2)) {
                result.push_back(*seg);
            }
        }
    }
    return result;
}

}  // namespace qi::intersection
