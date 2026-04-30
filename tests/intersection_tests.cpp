#include <gtest/gtest.h>

#include <cmath>

#include "Orient3d.hpp"
#include "Polyline.hpp"
#include "TriangleTriangle.hpp"
#include "Vec3.hpp"
#include "test_utils.hpp"

using qi::geometry::Vec3;
using qi::intersection::intersectTriangles;
using qi::intersection::orient3d;
using qi::mesh::Segment;

// ---- orient3d primitive ----

TEST(Orient3dTest, SignFollowsDevillersGuigueFormula) {
    // [a,b,c,d] is computed as det of rows (a-d, b-d, c-d). For a=O,
    // b=+X, c=+Y (a CCW triangle in XY plane) this evaluates to a NEGATIVE
    // value when d is on the +Z side. This is opposite to CGAL's orient3d
    // (which would return positive); we follow the paper's formula directly
    // because the canonical-form rules in TriangleTriangle.cpp are derived
    // for that convention.
    const Vec3 a(0, 0, 0), b(1, 0, 0), c(0, 1, 0);
    EXPECT_EQ(orient3d(a, b, c, Vec3(0, 0, 1)), -1);   // d on +Z (right-hand normal side)
    EXPECT_EQ(orient3d(a, b, c, Vec3(0, 0, -1)), +1);  // d on -Z
    EXPECT_EQ(orient3d(a, b, c, Vec3(0.3, 0.3, 0)), 0);
}

TEST(Orient3dTest, SwapTwoArgsNegatesSign) {
    const Vec3 a(0, 0, 0), b(1, 0, 0), c(0, 1, 0), d(0, 0, 1);
    const int s = orient3d(a, b, c, d);
    EXPECT_EQ(orient3d(a, c, b, d), -s);  // swap b,c
    EXPECT_EQ(orient3d(b, a, c, d), -s);  // swap a,b
    EXPECT_EQ(orient3d(a, b, d, c), -s);  // swap c,d
}

TEST(Orient3dTest, CyclicShiftPreservesSign) {
    const Vec3 a(0, 0, 0), b(1, 0, 0), c(0, 1, 0), d(0, 0, 1);
    const int s = orient3d(a, b, c, d);
    // Cyclic shift on the triangle (a, b, c) keeps the normal direction.
    EXPECT_EQ(orient3d(b, c, a, d), s);
    EXPECT_EQ(orient3d(c, a, b, d), s);
}

TEST(Orient3dTest, DegenerateTriangleReturnsZero) {
    const Vec3 a(0, 0, 0), b(1, 0, 0), c(2, 0, 0);  // colinear
    EXPECT_EQ(orient3d(a, b, c, Vec3(0, 1, 0)), 0);
}

// ---- Triangle-triangle: trivial rejections ----

TEST(TriangleTriangleTest, NoIntersectionFarApart) {
    auto seg = intersectTriangles(
        Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0),
        Vec3(10, 10, 10), Vec3(11, 10, 10), Vec3(10, 11, 10));
    EXPECT_FALSE(seg.has_value());
}

TEST(TriangleTriangleTest, NoIntersectionParallel) {
    auto seg = intersectTriangles(
        Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0),       // z = 0
        Vec3(0, 0, 1), Vec3(1, 0, 1), Vec3(0, 1, 1));      // z = 1
    EXPECT_FALSE(seg.has_value());
}

TEST(TriangleTriangleTest, CoplanarTrianglesReturnNone) {
    auto seg = intersectTriangles(
        Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(0, 2, 0),
        Vec3(1, 1, 0), Vec3(3, 1, 0), Vec3(1, 3, 0));
    EXPECT_FALSE(seg.has_value());
}

TEST(TriangleTriangleTest, DegenerateZeroAreaReturnsNone) {
    auto seg = intersectTriangles(
        Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(2, 0, 0),  // colinear
        Vec3(0, -1, 0), Vec3(0, 1, 0), Vec3(0, 0, 1));
    EXPECT_FALSE(seg.has_value());
}

// ---- Triangle-triangle: known geometric cases ----

TEST(TriangleTriangleTest, OrthogonalCrossingAtXAxis) {
    auto seg = intersectTriangles(
        Vec3(0, -1, 0), Vec3(1, -1, 0), Vec3(0.5, 1, 0),    // XY-plane triangle
        Vec3(0, 0, -1), Vec3(1, 0, -1), Vec3(0.5, 0, 1));   // XZ-plane triangle
    ASSERT_TRUE(seg.has_value());

    EXPECT_NEAR(seg->a.y(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(seg->a.z(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(seg->b.y(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(seg->b.z(), 0.0, qi::test::kEpsTight);

    const double xLo = std::min(seg->a.x(), seg->b.x());
    const double xHi = std::max(seg->a.x(), seg->b.x());
    EXPECT_GE(xLo, 0.0 - qi::test::kEpsTight);
    EXPECT_LE(xHi, 1.0 + qi::test::kEpsTight);
}

TEST(TriangleTriangleTest, IntersectionLineDirection) {
    // Two planes whose intersection line is along Y at x = 0.5, z = 0.
    auto seg = intersectTriangles(
        Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0.5, 2, 0),         // XY plane, z = 0
        Vec3(0.5, 0, -1), Vec3(0.5, 0, 1), Vec3(0.5, 2, 0));   // x = 0.5 plane
    ASSERT_TRUE(seg.has_value());

    EXPECT_NEAR(seg->a.x(), 0.5, qi::test::kEpsTight);
    EXPECT_NEAR(seg->a.z(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(seg->b.x(), 0.5, qi::test::kEpsTight);
    EXPECT_NEAR(seg->b.z(), 0.0, qi::test::kEpsTight);
}

TEST(TriangleTriangleTest, EndpointsLieOnBothPlanes) {
    const Vec3 a1(-1, -1, 0), b1(2, -1, 0), c1(0, 2, 0);
    const Vec3 a2(0, 0, -1), b2(1, 0, 1), c2(-0.5, 1, 1);

    auto seg = intersectTriangles(a1, b1, c1, a2, b2, c2);
    ASSERT_TRUE(seg.has_value());

    // Endpoints must lie on plane of T1 (z = 0).
    EXPECT_NEAR(seg->a.z(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(seg->b.z(), 0.0, qi::test::kEpsTight);

    EXPECT_GT((seg->b - seg->a).norm(), 0.0);
}

// ---- Tests that exercise Devillers-Guigue canonical-form branches ----

TEST(TriangleTriangleTest, CrossingResultIndependentOfTriangleVertexOrder) {
    // The intersection segment should not depend on cyclic permutations of
    // either triangle's vertices, modulo segment endpoint swap.
    const Vec3 a1(-1, -1, 0), b1(2, -1, 0), c1(0, 2, 0);
    const Vec3 a2(0, 0, -1), b2(1, 0, 1), c2(-0.5, 1, 1);

    auto baseline = intersectTriangles(a1, b1, c1, a2, b2, c2);
    ASSERT_TRUE(baseline.has_value());

    // Cyclic permutations of T1 vertices must produce the same segment.
    auto rot1 = intersectTriangles(b1, c1, a1, a2, b2, c2);
    ASSERT_TRUE(rot1.has_value());
    auto rot2 = intersectTriangles(c1, a1, b1, a2, b2, c2);
    ASSERT_TRUE(rot2.has_value());

    auto sameSegment = [](const Segment& s, const Segment& t) {
        const double sLen = (s.b - s.a).norm();
        const double tLen = (t.b - t.a).norm();
        if (std::abs(sLen - tLen) > qi::test::kEpsLoose) return false;
        const bool sameDir = (s.a - t.a).norm() < qi::test::kEpsLoose &&
                             (s.b - t.b).norm() < qi::test::kEpsLoose;
        const bool flipped = (s.a - t.b).norm() < qi::test::kEpsLoose &&
                             (s.b - t.a).norm() < qi::test::kEpsLoose;
        return sameDir || flipped;
    };

    EXPECT_TRUE(sameSegment(*baseline, *rot1));
    EXPECT_TRUE(sameSegment(*baseline, *rot2));

    // Cyclic permutations of T2 vertices similarly.
    auto rot3 = intersectTriangles(a1, b1, c1, b2, c2, a2);
    auto rot4 = intersectTriangles(a1, b1, c1, c2, a2, b2);
    ASSERT_TRUE(rot3.has_value());
    ASSERT_TRUE(rot4.has_value());
    EXPECT_TRUE(sameSegment(*baseline, *rot3));
    EXPECT_TRUE(sameSegment(*baseline, *rot4));
}

TEST(TriangleTriangleTest, ResultIndependentOfTriangleSwap) {
    // Swapping the order of T1 and T2 should yield the same segment.
    const Vec3 a1(-1, -1, 0), b1(2, -1, 0), c1(0, 2, 0);
    const Vec3 a2(0, 0, -1), b2(1, 0, 1), c2(-0.5, 1, 1);

    auto first = intersectTriangles(a1, b1, c1, a2, b2, c2);
    auto swapped = intersectTriangles(a2, b2, c2, a1, b1, c1);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(swapped.has_value());

    auto sameLine = [](const Segment& s, const Segment& t) {
        const bool sameDir = (s.a - t.a).norm() < qi::test::kEpsLoose &&
                             (s.b - t.b).norm() < qi::test::kEpsLoose;
        const bool flipped = (s.a - t.b).norm() < qi::test::kEpsLoose &&
                             (s.b - t.a).norm() < qi::test::kEpsLoose;
        return sameDir || flipped;
    };
    EXPECT_TRUE(sameLine(*first, *swapped));
}

TEST(TriangleTriangleTest, VertexExactlyOnPlaneStraddleCrossesProduces) {
    // T2 is in plane y = 0. T1 has one vertex exactly on that plane and the
    // other two on opposite sides.
    auto seg = intersectTriangles(
        Vec3(0.5, 0, 0.5),    // exactly on plane y = 0
        Vec3(1, 1, 0),
        Vec3(0, -1, 0),
        Vec3(-1, 0, -1), Vec3(2, 0, -1), Vec3(0.5, 0, 2));  // big triangle in plane y = 0

    ASSERT_TRUE(seg.has_value());
    // Both endpoints must lie in y = 0 (plane of T2).
    EXPECT_NEAR(seg->a.y(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(seg->b.y(), 0.0, qi::test::kEpsTight);
}

TEST(TriangleTriangleTest, VertexTouchesPlaneOutsideOtherTriangle) {
    // T1 has one vertex exactly on plane(T2)=y=0, two on +y side. The on-plane
    // vertex (5, 0, 0) lies OUTSIDE the projection of T2, so no intersection.
    auto seg = intersectTriangles(
        Vec3(5, 0, 0),
        Vec3(5.5, 1, 0),
        Vec3(5, 1, 1),
        Vec3(-1, 0, -1), Vec3(2, 0, -1), Vec3(0.5, 0, 2));
    EXPECT_FALSE(seg.has_value());
}

TEST(TriangleTriangleTest, VertexTouchesPlaneInsideOtherTriangle) {
    // Same setup but the on-plane vertex (0, 0, 0) lies INSIDE the projection
    // of T2 onto y=0. Per Devillers-Guigue's special-case canonical form, the
    // algorithm reports a (degenerate) intersection at the touching vertex.
    auto seg = intersectTriangles(
        Vec3(0, 0, 0),
        Vec3(1, 1, 0),
        Vec3(0, 1, 1),
        Vec3(-1, 0, -1), Vec3(2, 0, -1), Vec3(0.5, 0, 2));
    ASSERT_TRUE(seg.has_value());
    // The segment collapses to the touching point (0, 0, 0).
    EXPECT_NEAR(seg->a.norm(), 0.0, qi::test::kEpsLoose);
    EXPECT_NEAR(seg->b.norm(), 0.0, qi::test::kEpsLoose);
}

TEST(TriangleTriangleTest, ManyPermutationsProduceConsistentBoolean) {
    // Two triangles that intersect, two that don't. For each, all 9 cyclic
    // permutations of vertices (3 of T1 × 3 of T2) must produce a
    // consistent intersect/no-intersect classification.
    auto allRotationsAgree = [](const Vec3& a1, const Vec3& b1, const Vec3& c1,
                                const Vec3& a2, const Vec3& b2, const Vec3& c2,
                                bool expected) {
        const std::array<std::array<Vec3, 3>, 3> r1{{
            {{a1, b1, c1}}, {{b1, c1, a1}}, {{c1, a1, b1}},
        }};
        const std::array<std::array<Vec3, 3>, 3> r2{{
            {{a2, b2, c2}}, {{b2, c2, a2}}, {{c2, a2, b2}},
        }};
        for (const auto& t1 : r1) {
            for (const auto& t2 : r2) {
                const auto s = intersectTriangles(t1[0], t1[1], t1[2],
                                                  t2[0], t2[1], t2[2]);
                EXPECT_EQ(s.has_value(), expected)
                    << "rotation mismatch for triangles "
                    << t1[0].transpose() << " / " << t2[0].transpose();
            }
        }
    };

    // Should intersect.
    allRotationsAgree(Vec3(-1, -1, 0), Vec3(2, -1, 0), Vec3(0, 2, 0),
                      Vec3(0, 0, -1), Vec3(1, 0, 1), Vec3(-0.5, 1, 1),
                      true);
    // Should NOT intersect.
    allRotationsAgree(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0),
                      Vec3(10, 10, 10), Vec3(11, 10, 10), Vec3(10, 11, 10),
                      false);
}
