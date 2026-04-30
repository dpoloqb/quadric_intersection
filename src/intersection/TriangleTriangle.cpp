#include "TriangleTriangle.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

#include "Orient3d.hpp"

namespace qi::intersection {

using qi::geometry::Vec3;
using qi::mesh::Segment;

namespace {

constexpr double kInterpEps = 1e-15;

// Result of finding the canonical permutation for one triangle: a cyclic
// rotation of its own vertices, plus a flag saying whether the OTHER triangle
// must have q ↔ r swapped (which flips the other plane's normal direction
// and thereby flips the signs of these vertices).
struct Canonical {
    int rotation;     // 0, 1 or 2: cyclic shift of (a, b, c)
    bool flipOther;
};

// Devillers-Guigue 2002, §3.1 canonical form: given the three sign tests of a
// triangle's vertices against the OTHER triangle's plane, find a cyclic
// rotation such that the rotated vertex 0 (= p1) ends up on the positive
// halfspace and vertices 1, 2 (= q1, r1) on the negative halfspace.
// flipOther=true means the caller must additionally swap the other triangle's
// q ↔ r (which flips that plane's normal and thereby negates these signs).
//
// Two patterns from the paper:
//   - General case (no vertex coplanar): p1 is the lone strict vertex.
//     Pattern (sa>0, sb≤0, sc≤0) without flip, or (sa<0, sb≥0, sc≥0) with flip.
//   - Special case (one vertex on plane, two on the same strict side):
//     "p1 is the only vertex that does not lie on the negative open halfspace".
//     Pattern (sa=0, sb<0, sc<0) without flip, or (sa=0, sb>0, sc>0) with flip.
std::optional<Canonical> findCanonical(int s0, int s1, int s2) {
    const std::array<int, 3> signs = {s0, s1, s2};
    for (int r = 0; r < 3; ++r) {
        const int sa = signs[r];
        const int sb = signs[(r + 1) % 3];
        const int sc = signs[(r + 2) % 3];
        if (sa > 0 && sb <= 0 && sc <= 0) return Canonical{r, false};
        if (sa < 0 && sb >= 0 && sc >= 0) return Canonical{r, true};
    }
    for (int r = 0; r < 3; ++r) {
        const int sa = signs[r];
        const int sb = signs[(r + 1) % 3];
        const int sc = signs[(r + 2) % 3];
        if (sa == 0 && sb < 0 && sc < 0) return Canonical{r, false};
        if (sa == 0 && sb > 0 && sc > 0) return Canonical{r, true};
    }
    return std::nullopt;
}

// Cyclic shift: rotation=0 keeps (p, q, r); 1 → (q, r, p); 2 → (r, p, q).
void applyRotation(int rotation, Vec3& p, Vec3& q, Vec3& r) {
    if (rotation == 1) {
        const Vec3 tmp = p;
        p = q;
        q = r;
        r = tmp;
    } else if (rotation == 2) {
        const Vec3 tmp = p;
        p = r;
        r = q;
        q = tmp;
    }
}

// Linear interpolation along edge (a → b) to the point where the signed
// distance reaches zero. da, db are the signed distances of a, b to the
// reference plane.
Vec3 interpToZero(const Vec3& a, double da, const Vec3& b, double db) {
    const double denom = da - db;
    if (std::abs(denom) < kInterpEps) return a;
    const double t = da / denom;
    return a + t * (b - a);
}

// Construct the 3D segment of intersection given canonical-form triangles:
// p1 is on +side of plane(T2), q1, r1 on -side (or on plane);
// p2 is on +side of plane(T1), q2, r2 on -side (or on plane).
// Therefore edges p1q1, p1r1 cross plane(T2) and edges p2q2, p2r2 cross
// plane(T1). The four crossing points all lie on the line π1 ∩ π2; the
// intersection segment is the overlap of intervals (i, j) and (k, l) along
// that line.
Segment constructSegment(const Vec3& p1, const Vec3& q1, const Vec3& r1,
                         const Vec3& p2, const Vec3& q2, const Vec3& r2) {
    Vec3 n2 = (q2 - p2).cross(r2 - p2);
    n2.normalize();
    const double D2 = -n2.dot(p2);

    Vec3 n1 = (q1 - p1).cross(r1 - p1);
    n1.normalize();
    const double D1 = -n1.dot(p1);

    auto sd1 = [&](const Vec3& v) { return n1.dot(v) + D1; };
    auto sd2 = [&](const Vec3& v) { return n2.dot(v) + D2; };

    const double dp1 = sd2(p1), dq1 = sd2(q1), dr1 = sd2(r1);
    const double dp2 = sd1(p2), dq2 = sd1(q2), dr2 = sd1(r2);

    const Vec3 i = interpToZero(p1, dp1, q1, dq1);
    const Vec3 j = interpToZero(p1, dp1, r1, dr1);
    const Vec3 k = interpToZero(p2, dp2, q2, dq2);
    const Vec3 l = interpToZero(p2, dp2, r2, dr2);

    const Vec3 dir = n1.cross(n2);
    int axis = 0;
    if (std::abs(dir.y()) > std::abs(dir[axis])) axis = 1;
    if (std::abs(dir.z()) > std::abs(dir[axis])) axis = 2;

    Vec3 i1 = i, i2 = j;
    if (i1[axis] > i2[axis]) std::swap(i1, i2);
    Vec3 k1 = k, k2 = l;
    if (k1[axis] > k2[axis]) std::swap(k1, k2);

    const Vec3 pStart = (i1[axis] >= k1[axis]) ? i1 : k1;
    const Vec3 pEnd   = (i2[axis] <= k2[axis]) ? i2 : k2;

    return {pStart, pEnd};
}

}  // namespace

// Devillers & Guigue 2002, "Faster Triangle-Triangle Intersection Tests"
// (INRIA RR-4488). All branching decisions are signs of orient3d, which is a
// degree-3 polynomial — 5–8× lower polynomial degree than Möller 1997's
// interval test, giving better numerical stability and ~20–30% fewer
// arithmetic operations.
//
// The coplanar case is left as nullopt (TODO per PLAN.md), matching prior
// behaviour and the Möller-based implementation that lived here before.
std::optional<Segment> intersectTriangles(const Vec3& a1, const Vec3& b1, const Vec3& c1,
                                          const Vec3& a2, const Vec3& b2, const Vec3& c2) {
    // Step 1: signs of T1's vertices w.r.t. plane(T2).
    const int s0 = orient3d(a2, b2, c2, a1);
    const int s1 = orient3d(a2, b2, c2, b1);
    const int s2 = orient3d(a2, b2, c2, c1);
    if (s0 > 0 && s1 > 0 && s2 > 0) return std::nullopt;
    if (s0 < 0 && s1 < 0 && s2 < 0) return std::nullopt;

    // Step 2: signs of T2's vertices w.r.t. plane(T1).
    const int t0 = orient3d(a1, b1, c1, a2);
    const int t1 = orient3d(a1, b1, c1, b2);
    const int t2 = orient3d(a1, b1, c1, c2);
    if (t0 > 0 && t1 > 0 && t2 > 0) return std::nullopt;
    if (t0 < 0 && t1 < 0 && t2 < 0) return std::nullopt;

    // Coplanar (TODO per PLAN.md): both triangles share a plane.
    if (s0 == 0 && s1 == 0 && s2 == 0) return std::nullopt;
    if (t0 == 0 && t1 == 0 && t2 == 0) return std::nullopt;

    // Step 3: find canonical permutations.
    const auto canon1 = findCanonical(s0, s1, s2);
    const auto canon2 = findCanonical(t0, t1, t2);
    if (!canon1.has_value() || !canon2.has_value()) {
        // Edge cases like (+, +, 0) where the triangle merely touches the
        // other plane at a vertex without crossing it. Treated as
        // no-intersection (degenerate point contact, no segment).
        return std::nullopt;
    }

    Vec3 p1 = a1, q1 = b1, r1 = c1;
    applyRotation(canon1->rotation, p1, q1, r1);

    Vec3 p2 = a2, q2 = b2, r2 = c2;
    applyRotation(canon2->rotation, p2, q2, r2);

    // canon1 says: swap q2 ↔ r2 to flip plane(T2)'s normal (which makes p1
    // land on the positive side). Symmetrically, canon2 may direct a swap
    // of q1 ↔ r1.
    if (canon1->flipOther) std::swap(q2, r2);
    if (canon2->flipOther) std::swap(q1, r1);

    // Step 4: final two orient3d sign tests (Devillers-Guigue eq. 1).
    if (orient3d(p1, q1, p2, q2) > 0) return std::nullopt;
    if (orient3d(p1, r1, r2, p2) > 0) return std::nullopt;

    return constructSegment(p1, q1, r1, p2, q2, r2);
}

}  // namespace qi::intersection
