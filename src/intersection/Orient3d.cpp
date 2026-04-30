#include "Orient3d.hpp"

namespace qi::intersection {

namespace {
// Tolerance is intentionally generous: orient3d is degree 3 in the inputs, so
// for triangles with coordinates of magnitude ~30 (typical of our quadric
// bboxes), the determinant scales by ~27000. 1e-10 then corresponds to a
// signed distance of roughly 1e-14 in normalized space — comfortably below
// any geometric tolerance used elsewhere (kEpsTight = 1e-9).
constexpr double kOrientEps = 1e-10;
}  // namespace

int orient3d(const qi::geometry::Vec3& a,
             const qi::geometry::Vec3& b,
             const qi::geometry::Vec3& c,
             const qi::geometry::Vec3& d) {
    // [a,b,c,d] = (a-d) · ((b-d) × (c-d)) — scalar triple product of the
    // edge vectors of the (a,b,c,d) tetrahedron with apex d.
    const double det = (a - d).dot((b - d).cross(c - d));
    if (det > kOrientEps) return +1;
    if (det < -kOrientEps) return -1;
    return 0;
}

}  // namespace qi::intersection
