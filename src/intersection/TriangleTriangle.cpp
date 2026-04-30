#include "TriangleTriangle.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace qi::intersection {

using qi::geometry::Vec3;
using qi::mesh::Segment;

namespace {

constexpr double kPlaneEps = 1e-10;
constexpr double kAreaEps = 1e-20;

// Collects up to 2 points where the edges of triangle (p0, p1, p2) cross a
// plane, given signed distances (d0, d1, d2) of the vertices to that plane.
// Returns the number of unique crossings written into `out`.
int collectEdgeCrossings(const Vec3& p0, double d0,
                         const Vec3& p1, double d1,
                         const Vec3& p2, double d2,
                         std::array<Vec3, 2>& out) {
    int n = 0;
    auto edgeCross = [&](const Vec3& pa, double da, const Vec3& pb, double db) {
        // Closed-half convention: ">= 0 vs < 0" registers each crossing exactly
        // once, even when an endpoint is exactly on the plane.
        const bool aPos = da >= 0.0;
        const bool bPos = db >= 0.0;
        if (aPos == bPos) return;
        if (n >= 2) return;  // safety; should not happen for non-coplanar triangles
        const double t = da / (da - db);
        out[n++] = pa + t * (pb - pa);
    };
    edgeCross(p0, d0, p1, d1);
    edgeCross(p1, d1, p2, d2);
    edgeCross(p2, d2, p0, d0);
    return n;
}

}  // namespace

std::optional<Segment> intersectTriangles(const Vec3& a1, const Vec3& b1, const Vec3& c1,
                                          const Vec3& a2, const Vec3& b2, const Vec3& c2) {
    // Plane of T2.
    Vec3 n2 = (b2 - a2).cross(c2 - a2);
    const double n2sq = n2.squaredNorm();
    if (n2sq < kAreaEps) return std::nullopt;
    n2 /= std::sqrt(n2sq);
    const double D2 = -n2.dot(a2);

    const double da1 = n2.dot(a1) + D2;
    const double db1 = n2.dot(b1) + D2;
    const double dc1 = n2.dot(c1) + D2;

    // Reject if all three vertices of T1 are strictly on one side of plane(T2).
    if ((da1 > kPlaneEps && db1 > kPlaneEps && dc1 > kPlaneEps) ||
        (da1 < -kPlaneEps && db1 < -kPlaneEps && dc1 < -kPlaneEps)) {
        return std::nullopt;
    }

    // Plane of T1.
    Vec3 n1 = (b1 - a1).cross(c1 - a1);
    const double n1sq = n1.squaredNorm();
    if (n1sq < kAreaEps) return std::nullopt;
    n1 /= std::sqrt(n1sq);
    const double D1 = -n1.dot(a1);

    const double da2 = n1.dot(a2) + D1;
    const double db2 = n1.dot(b2) + D1;
    const double dc2 = n1.dot(c2) + D1;

    if ((da2 > kPlaneEps && db2 > kPlaneEps && dc2 > kPlaneEps) ||
        (da2 < -kPlaneEps && db2 < -kPlaneEps && dc2 < -kPlaneEps)) {
        return std::nullopt;
    }

    // Coplanar case (TODO per PLAN.md): all signed distances ≈ 0.
    const bool t1InT2plane = std::abs(da1) < kPlaneEps && std::abs(db1) < kPlaneEps &&
                             std::abs(dc1) < kPlaneEps;
    const bool t2InT1plane = std::abs(da2) < kPlaneEps && std::abs(db2) < kPlaneEps &&
                             std::abs(dc2) < kPlaneEps;
    if (t1InT2plane || t2InT1plane) {
        return std::nullopt;
    }

    // Collect each triangle's edge-crossings against the other plane.
    std::array<Vec3, 2> seg1Pts;
    std::array<Vec3, 2> seg2Pts;
    const int n1Cross = collectEdgeCrossings(a1, da1, b1, db1, c1, dc1, seg1Pts);
    const int n2Cross = collectEdgeCrossings(a2, da2, b2, db2, c2, dc2, seg2Pts);
    if (n1Cross != 2 || n2Cross != 2) {
        return std::nullopt;
    }

    // Both little segments lie on the line of intersection of the two planes.
    // Project onto the axis where dir = n1 × n2 has the largest component.
    const Vec3 dir = n1.cross(n2);
    int axis = 0;
    if (std::abs(dir.y()) > std::abs(dir[axis])) axis = 1;
    if (std::abs(dir.z()) > std::abs(dir[axis])) axis = 2;

    auto sortByAxis = [&](std::array<Vec3, 2>& seg) {
        if (seg[0][axis] > seg[1][axis]) std::swap(seg[0], seg[1]);
    };
    sortByAxis(seg1Pts);
    sortByAxis(seg2Pts);

    const double t1Lo = seg1Pts[0][axis], t1Hi = seg1Pts[1][axis];
    const double t2Lo = seg2Pts[0][axis], t2Hi = seg2Pts[1][axis];

    if (t1Hi < t2Lo || t2Hi < t1Lo) {
        return std::nullopt;
    }

    const Vec3 pStart = (t1Lo >= t2Lo) ? seg1Pts[0] : seg2Pts[0];
    const Vec3 pEnd   = (t1Hi <= t2Hi) ? seg1Pts[1] : seg2Pts[1];

    return Segment{pStart, pEnd};
}

}  // namespace qi::intersection
