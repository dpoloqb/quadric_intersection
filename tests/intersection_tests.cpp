#include <gtest/gtest.h>

#include <cmath>

#include <Eigen/Geometry>
#include <chrono>
#include <iostream>
#include <memory>

#include "BoundingBox.hpp"
#include "BvhIntersector.hpp"
#include "Ellipsoid.hpp"
#include "MarchingCubes.hpp"
#include "MarchingCubesParams.hpp"
#include "Mesh.hpp"
#include "NaiveIntersector.hpp"
#include "Orient3d.hpp"
#include "ParametricParams.hpp"
#include "ParametricTriangulator.hpp"
#include "Polyline.hpp"
#include "PolylineBuilder.hpp"
#include "Quadric.hpp"
#include "QuadricFactory.hpp"
#include "Transform.hpp"
#include "TriangleTriangle.hpp"
#include "Vec3.hpp"
#include "test_utils.hpp"

using qi::geometry::BoundingBox;
using qi::geometry::Ellipsoid;
using qi::geometry::Quat;
using qi::geometry::Transform;
using qi::geometry::Vec3;
using qi::intersection::buildPolylines;
using qi::intersection::BvhIntersector;
using qi::intersection::intersectTriangles;
using qi::intersection::NaiveIntersector;
using qi::intersection::orient3d;
using qi::mesh::Mesh;
using qi::mesh::Polyline;
using qi::mesh::Segment;
using qi::triangulation::MarchingCubes;
using qi::triangulation::MarchingCubesParams;
using qi::triangulation::ParametricParams;
using qi::triangulation::ParametricTriangulator;

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

// ---- PolylineBuilder ----

TEST(PolylineBuilderTest, EmptyInputProducesNoPolylines) {
    auto poly = buildPolylines({});
    EXPECT_TRUE(poly.empty());
}

TEST(PolylineBuilderTest, SingleSegmentProducesOpenPolyline) {
    Segment s{Vec3(0, 0, 0), Vec3(1, 0, 0)};
    auto poly = buildPolylines({s});
    ASSERT_EQ(poly.size(), 1u);
    EXPECT_FALSE(poly[0].closed);
    EXPECT_EQ(poly[0].points.size(), 2u);
}

TEST(PolylineBuilderTest, SimpleClosedLoop) {
    // Square: A=(0,0,0), B=(1,0,0), C=(1,1,0), D=(0,1,0).
    const Vec3 A(0, 0, 0), B(1, 0, 0), C(1, 1, 0), D(0, 1, 0);
    std::vector<Segment> segs{
        {A, B}, {B, C}, {C, D}, {D, A},
    };
    auto poly = buildPolylines(segs);
    ASSERT_EQ(poly.size(), 1u);
    EXPECT_TRUE(poly[0].closed);
    // Closed polyline ends with the start point repeated → 5 points.
    EXPECT_EQ(poly[0].points.size(), 5u);
    EXPECT_NEAR((poly[0].points.front() - poly[0].points.back()).norm(), 0.0,
                qi::test::kEpsTight);
}

TEST(PolylineBuilderTest, OpenChain) {
    const Vec3 A(0, 0, 0), B(1, 0, 0), C(2, 0, 0), D(3, 0, 0);
    std::vector<Segment> segs{{A, B}, {B, C}, {C, D}};
    auto poly = buildPolylines(segs);
    ASSERT_EQ(poly.size(), 1u);
    EXPECT_FALSE(poly[0].closed);
    EXPECT_EQ(poly[0].points.size(), 4u);
}

TEST(PolylineBuilderTest, OpenChainHandlesArbitrarySegmentOrder) {
    // Same chain as above but segments listed in reversed order — the builder
    // must walk forward AND backward from the seed segment to recover the full
    // chain.
    const Vec3 A(0, 0, 0), B(1, 0, 0), C(2, 0, 0), D(3, 0, 0);
    std::vector<Segment> segs{{B, C}, {A, B}, {C, D}};
    auto poly = buildPolylines(segs);
    ASSERT_EQ(poly.size(), 1u);
    EXPECT_FALSE(poly[0].closed);
    EXPECT_EQ(poly[0].points.size(), 4u);
}

TEST(PolylineBuilderTest, TwoSeparateLoops) {
    // Two disjoint triangles.
    const Vec3 A(0, 0, 0), B(1, 0, 0), C(0, 1, 0);
    const Vec3 D(10, 0, 0), E(11, 0, 0), F(10, 1, 0);
    std::vector<Segment> segs{
        {A, B}, {B, C}, {C, A},
        {D, E}, {E, F}, {F, D},
    };
    auto poly = buildPolylines(segs);
    ASSERT_EQ(poly.size(), 2u);
    EXPECT_TRUE(poly[0].closed);
    EXPECT_TRUE(poly[1].closed);
}

TEST(PolylineBuilderTest, ToleranceMergesNearbyPoints) {
    // Square, but each segment's endpoints are perturbed by epsilon/10.
    const double e = 1e-6;
    const double d = e / 10.0;  // smaller than epsilon → must be merged
    std::vector<Segment> segs{
        {Vec3(0, 0, 0),       Vec3(1, 0, 0)},
        {Vec3(1 + d, 0, 0),   Vec3(1, 1, 0)},
        {Vec3(1, 1 + d, 0),   Vec3(0, 1, 0)},
        {Vec3(0 + d, 1, 0),   Vec3(0, 0 + d, 0)},
    };
    auto poly = buildPolylines(segs, e);
    ASSERT_EQ(poly.size(), 1u);
    EXPECT_TRUE(poly[0].closed);
}

TEST(PolylineBuilderTest, DegenerateZeroLengthSegmentsDropped) {
    // A real triangle plus two zero-length segments.
    const Vec3 A(0, 0, 0), B(1, 0, 0), C(0, 1, 0);
    std::vector<Segment> segs{
        {A, A},
        {A, B}, {B, C}, {C, A},
        {B, B},
    };
    auto poly = buildPolylines(segs);
    ASSERT_EQ(poly.size(), 1u);
    EXPECT_TRUE(poly[0].closed);
    EXPECT_EQ(poly[0].points.size(), 4u);
}

// ---- NaiveIntersector ----

namespace {

// Helper: triangulate a quadric with parametric method into a Mesh.
Mesh triangulateParametric(const qi::geometry::Quadric& q,
                           const BoundingBox& bbox,
                           int uSteps = 80, int vSteps = 80) {
    ParametricTriangulator pt(ParametricParams{uSteps, vSteps});
    return pt.triangulate(q, bbox);
}

}  // namespace

TEST(NaiveIntersectorTest, MethodNameIsStable) {
    NaiveIntersector ni;
    EXPECT_EQ(ni.methodName(), "naive");
}

TEST(NaiveIntersectorTest, TwoDisjointSpheres) {
    // Sphere R=1 at origin, sphere R=1 at (10,0,0). No overlap.
    auto qa = std::make_unique<Ellipsoid>(1.0, 1.0, 1.0);
    auto qb = std::make_unique<Ellipsoid>(1.0, 1.0, 1.0);
    qb->setTransform(Transform(Vec3(10, 0, 0), Quat::Identity()));

    BoundingBox bboxA(Vec3(-2, -2, -2), Vec3(2, 2, 2));
    BoundingBox bboxB(Vec3(8, -2, -2), Vec3(12, 2, 2));
    Mesh ma = triangulateParametric(*qa, bboxA, 24, 24);
    Mesh mb = triangulateParametric(*qb, bboxB, 24, 24);

    NaiveIntersector ni;
    auto segs = ni.findSegments(ma, mb);
    auto polys = buildPolylines(segs);
    EXPECT_EQ(segs.size(), 0u);
    EXPECT_EQ(polys.size(), 0u);
}

// Two R=2 spheres centered at (0,0,0) and (2,0,0) intersect in a circle in
// plane x=1, radius √3. Combine all invariants into one test so we only run
// the O(n·m) pipeline once. With 32 uSteps×32 vSteps ≈ 2k triangles per mesh,
// pair test count ≈ 4M and Naive takes ~20 s in Debug.
TEST(NaiveIntersectorTest, TwoIntersectingSpheresFullInvariants) {
    Ellipsoid qa(2.0, 2.0, 2.0);
    Ellipsoid qb(2.0, 2.0, 2.0);
    qb.setTransform(Transform(Vec3(2, 0, 0), Quat::Identity()));
    BoundingBox bbox(Vec3(-3, -3, -3), Vec3(5, 3, 3));
    Mesh ma = triangulateParametric(qa, bbox, 32, 32);
    Mesh mb = triangulateParametric(qb, bbox, 32, 32);

    NaiveIntersector ni;
    auto segs = ni.findSegments(ma, mb);
    ASSERT_GT(segs.size(), 0u);

    // Tolerances. Triangulation edge length L ≈ 4πR/N ≈ 0.78 for N=32, R=2;
    // tangent-plane error ≈ L²/(8R) ≈ 0.04.
    const double tolGeom = 0.08;
    const double tolImplicit = 0.06;

    // Invariant 1: every segment endpoint lies on both implicit surfaces.
    for (const auto& s : segs) {
        EXPECT_NEAR(qa.implicit(s.a), 0.0, tolImplicit);
        EXPECT_NEAR(qb.implicit(s.a), 0.0, tolImplicit);
        EXPECT_NEAR(qa.implicit(s.b), 0.0, tolImplicit);
        EXPECT_NEAR(qb.implicit(s.b), 0.0, tolImplicit);
    }

    // Invariant 2: symmetry about plane x=1 is preserved — every endpoint
    // has x ≈ 1.
    for (const auto& s : segs) {
        EXPECT_NEAR(s.a.x(), 1.0, tolGeom);
        EXPECT_NEAR(s.b.x(), 1.0, tolGeom);
    }

    // Invariant 3: stitched polylines form a closed circle of radius √3.
    auto polys = buildPolylines(segs, 1e-3);
    ASSERT_GE(polys.size(), 1u);
    std::size_t biggest = 0;
    for (std::size_t i = 1; i < polys.size(); ++i) {
        if (polys[i].points.size() > polys[biggest].points.size()) biggest = i;
    }
    const Polyline& circle = polys[biggest];
    EXPECT_TRUE(circle.closed);
    for (const auto& p : circle.points) {
        EXPECT_NEAR(p.x(), 1.0, tolGeom);
        const double r = std::sqrt(p.y() * p.y() + p.z() * p.z());
        EXPECT_NEAR(r, std::sqrt(3.0), tolGeom);
    }
}

// ---- BvhIntersector ----

TEST(BvhIntersectorTest, MethodNameIsStable) {
    BvhIntersector bi;
    EXPECT_EQ(bi.methodName(), "bvh");
}

TEST(BvhIntersectorTest, EmptyMeshDoesNotCrash) {
    Mesh empty;
    Mesh nonEmpty;
    nonEmpty.vertices = {Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0)};
    nonEmpty.triangles = {{0, 1, 2}};
    BvhIntersector bi;
    EXPECT_TRUE(bi.findSegments(empty, nonEmpty).empty());
    EXPECT_TRUE(bi.findSegments(nonEmpty, empty).empty());
    EXPECT_TRUE(bi.findSegments(empty, empty).empty());
}

TEST(BvhIntersectorTest, TwoDisjointSpheresReturnsEmpty) {
    auto qa = std::make_unique<Ellipsoid>(1.0, 1.0, 1.0);
    auto qb = std::make_unique<Ellipsoid>(1.0, 1.0, 1.0);
    qb->setTransform(Transform(Vec3(10, 0, 0), Quat::Identity()));

    BoundingBox bboxA(Vec3(-2, -2, -2), Vec3(2, 2, 2));
    BoundingBox bboxB(Vec3(8, -2, -2), Vec3(12, 2, 2));
    Mesh ma = triangulateParametric(*qa, bboxA, 24, 24);
    Mesh mb = triangulateParametric(*qb, bboxB, 24, 24);

    BvhIntersector bi;
    auto segs = bi.findSegments(ma, mb);
    EXPECT_EQ(segs.size(), 0u);
}

TEST(BvhIntersectorTest, ResultMatchesNaiveOnIntersectingSpheres) {
    // Identical setup to NaiveIntersectorTest.TwoIntersectingSpheresFullInvariants;
    // BVH must produce a result equivalent to Naive (modulo ordering).
    Ellipsoid qa(2.0, 2.0, 2.0);
    Ellipsoid qb(2.0, 2.0, 2.0);
    qb.setTransform(Transform(Vec3(2, 0, 0), Quat::Identity()));
    BoundingBox bbox(Vec3(-3, -3, -3), Vec3(5, 3, 3));
    Mesh ma = triangulateParametric(qa, bbox, 32, 32);
    Mesh mb = triangulateParametric(qb, bbox, 32, 32);

    NaiveIntersector ni;
    BvhIntersector bi;

    auto naiveSegs = ni.findSegments(ma, mb);
    auto bvhSegs = bi.findSegments(ma, mb);

    // Same number of segments and the stitched polylines have the same shape.
    EXPECT_EQ(naiveSegs.size(), bvhSegs.size());

    auto polysNaive = buildPolylines(naiveSegs, 1e-3);
    auto polysBvh = buildPolylines(bvhSegs, 1e-3);
    EXPECT_EQ(polysNaive.size(), polysBvh.size());

    // Both should give a single closed circle of radius √3 in plane x=1.
    ASSERT_GE(polysBvh.size(), 1u);
    std::size_t biggest = 0;
    for (std::size_t i = 1; i < polysBvh.size(); ++i) {
        if (polysBvh[i].points.size() > polysBvh[biggest].points.size()) biggest = i;
    }
    const auto& circle = polysBvh[biggest];
    EXPECT_TRUE(circle.closed);
    const double tol = 0.08;
    for (const auto& p : circle.points) {
        EXPECT_NEAR(p.x(), 1.0, tol);
        EXPECT_NEAR(std::sqrt(p.y() * p.y() + p.z() * p.z()), std::sqrt(3.0), tol);
    }
}

TEST(BvhIntersectorTest, FasterThanNaiveOnIntersectingSpheres) {
    Ellipsoid qa(2.0, 2.0, 2.0);
    Ellipsoid qb(2.0, 2.0, 2.0);
    qb.setTransform(Transform(Vec3(2, 0, 0), Quat::Identity()));
    BoundingBox bbox(Vec3(-3, -3, -3), Vec3(5, 3, 3));
    Mesh ma = triangulateParametric(qa, bbox, 40, 40);
    Mesh mb = triangulateParametric(qb, bbox, 40, 40);

    NaiveIntersector ni;
    BvhIntersector bi;

    using clock = std::chrono::steady_clock;

    const auto t0 = clock::now();
    auto naiveSegs = ni.findSegments(ma, mb);
    const auto t1 = clock::now();
    auto bvhSegs = bi.findSegments(ma, mb);
    const auto t2 = clock::now();

    const auto naiveMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    const auto bvhMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "  naive: " << naiveMs << " ms;  bvh: " << bvhMs << " ms\n";

    EXPECT_EQ(naiveSegs.size(), bvhSegs.size());
    EXPECT_LT(bvhMs, naiveMs);
}

// ---- Triangle-triangle: integrative test (kept last) ----

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
