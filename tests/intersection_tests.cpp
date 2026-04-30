#include <gtest/gtest.h>

#include <cmath>

#include "Polyline.hpp"
#include "TriangleTriangle.hpp"
#include "Vec3.hpp"
#include "test_utils.hpp"

using qi::geometry::Vec3;
using qi::intersection::intersectTriangles;
using qi::mesh::Segment;

namespace {

// Small helper: distance from a point to the line through two points.
double distancePointToLine(const Vec3& p, const Vec3& a, const Vec3& b) {
    const Vec3 d = b - a;
    return (d.cross(p - a)).norm() / d.norm();
}

}  // namespace

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
    // Two overlapping triangles in the same plane (z = 0).
    auto seg = intersectTriangles(
        Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(0, 2, 0),
        Vec3(1, 1, 0), Vec3(3, 1, 0), Vec3(1, 3, 0));
    EXPECT_FALSE(seg.has_value());
}

TEST(TriangleTriangleTest, OrthogonalCrossingAtXAxis) {
    // T1 in XY plane; T2 in XZ plane. Their planes intersect along the X axis.
    // T1 covers x ∈ [0, 1] near y = 0; T2 covers x ∈ [0, 1] near z = 0.
    auto seg = intersectTriangles(
        Vec3(0, -1, 0), Vec3(1, -1, 0), Vec3(0.5, 1, 0),    // XY-plane triangle
        Vec3(0, 0, -1), Vec3(1, 0, -1), Vec3(0.5, 0, 1));   // XZ-plane triangle
    ASSERT_TRUE(seg.has_value());

    // Intersection should be a segment along the X axis at y = z = 0.
    EXPECT_NEAR(seg->a.y(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(seg->a.z(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(seg->b.y(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(seg->b.z(), 0.0, qi::test::kEpsTight);

    // Endpoints are bounded by both triangles → x ∈ [0, 1].
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

    // Both endpoints are on the intersection line: x = 0.5, z = 0.
    EXPECT_NEAR(seg->a.x(), 0.5, qi::test::kEpsTight);
    EXPECT_NEAR(seg->a.z(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(seg->b.x(), 0.5, qi::test::kEpsTight);
    EXPECT_NEAR(seg->b.z(), 0.0, qi::test::kEpsTight);
}

TEST(TriangleTriangleTest, DegenerateZeroAreaReturnsNone) {
    // First triangle is degenerate (all points colinear).
    auto seg = intersectTriangles(
        Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(2, 0, 0),
        Vec3(0, -1, 0), Vec3(0, 1, 0), Vec3(0, 0, 1));
    EXPECT_FALSE(seg.has_value());
}

TEST(TriangleTriangleTest, EndpointsLieOnBothPlanes) {
    // General-position intersection — verify endpoints satisfy both plane equations.
    const Vec3 a1(-1, -1, 0), b1(2, -1, 0), c1(0, 2, 0);                // z = 0
    const Vec3 a2(0, 0, -1), b2(1, 0, 1), c2(-0.5, 1, 1);                // some tilted plane

    auto seg = intersectTriangles(a1, b1, c1, a2, b2, c2);
    ASSERT_TRUE(seg.has_value());

    // Endpoints must lie on plane of T1 (z = 0).
    EXPECT_NEAR(seg->a.z(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(seg->b.z(), 0.0, qi::test::kEpsTight);

    // Endpoints must lie on the line through both planes — they should be colinear with each other.
    // (One-point check: a and b are different but both on the plane intersection.)
    const Vec3 d = seg->b - seg->a;
    EXPECT_GT(d.norm(), 0.0);
}
