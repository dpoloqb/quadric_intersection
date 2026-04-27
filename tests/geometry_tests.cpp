#include <gtest/gtest.h>

#include <Eigen/Geometry>
#include <cmath>

#include "BoundingBox.hpp"
#include "Cone.hpp"
#include "Ellipsoid.hpp"
#include "EllipticCylinder.hpp"
#include "EllipticParaboloid.hpp"
#include "HyperbolicCylinder.hpp"
#include "HyperbolicParaboloid.hpp"
#include "HyperboloidOneSheet.hpp"
#include "HyperboloidTwoSheet.hpp"
#include "ParabolicCylinder.hpp"
#include "Quadric.hpp"
#include "Transform.hpp"
#include "Vec3.hpp"
#include "test_utils.hpp"

using qi::geometry::BoundingBox;
using qi::geometry::Cone;
using qi::geometry::Ellipsoid;
using qi::geometry::EllipticCylinder;
using qi::geometry::EllipticParaboloid;
using qi::geometry::HyperbolicCylinder;
using qi::geometry::HyperbolicParaboloid;
using qi::geometry::HyperboloidOneSheet;
using qi::geometry::HyperboloidTwoSheet;
using qi::geometry::ParabolicCylinder;
using qi::geometry::Quat;
using qi::geometry::Transform;
using qi::geometry::Vec3;

namespace {
Transform makeTestTransform() {
    Quat rot(Eigen::AngleAxisd(0.7, Vec3(1, 1, 0).normalized()));
    return Transform(Vec3(5, -2, 1), rot);
}
}  // namespace

// ---- BoundingBox + Transform ----

TEST(GeometryBasics, BoundingBoxContainsAndClamp) {
    BoundingBox bbox(Vec3(-1, -1, -1), Vec3(1, 1, 1));

    EXPECT_TRUE(bbox.contains(Vec3(0, 0, 0)));
    EXPECT_TRUE(bbox.contains(Vec3(1, 1, 1)));
    EXPECT_TRUE(bbox.contains(Vec3(-1, -1, -1)));
    EXPECT_FALSE(bbox.contains(Vec3(2, 0, 0)));
    EXPECT_FALSE(bbox.contains(Vec3(0, 0, -2)));

    Vec3 clamped = bbox.clampPoint(Vec3(2, 0.5, -3));
    EXPECT_DOUBLE_EQ(clamped.x(), 1.0);
    EXPECT_DOUBLE_EQ(clamped.y(), 0.5);
    EXPECT_DOUBLE_EQ(clamped.z(), -1.0);
}

TEST(GeometryBasics, BoundingBoxSegmentIntersection) {
    BoundingBox bbox(Vec3(0, 0, 0), Vec3(1, 1, 1));

    EXPECT_TRUE(bbox.intersectsSegment(Vec3(-1, 0.5, 0.5), Vec3(2, 0.5, 0.5)));
    EXPECT_FALSE(bbox.intersectsSegment(Vec3(2, 2, 2), Vec3(3, 3, 3)));
    EXPECT_TRUE(bbox.intersectsSegment(Vec3(0.3, 0.3, 0.3), Vec3(0.7, 0.7, 0.7)));
    EXPECT_FALSE(bbox.intersectsSegment(Vec3(-1, 2, 0.5), Vec3(2, 2, 0.5)));
    EXPECT_TRUE(bbox.intersectsSegment(Vec3(-1, 0.5, 0.5), Vec3(0.0, 0.5, 0.5)));
}

TEST(GeometryBasics, BoundingBoxCornersCenterDiagonal) {
    BoundingBox bbox(Vec3(0, 0, 0), Vec3(2, 4, 6));

    auto corners = bbox.corners();
    EXPECT_EQ(corners.size(), 8u);
    for (const auto& c : corners) {
        EXPECT_TRUE(bbox.contains(c));
    }

    EXPECT_DOUBLE_EQ(bbox.center().x(), 1.0);
    EXPECT_DOUBLE_EQ(bbox.center().y(), 2.0);
    EXPECT_DOUBLE_EQ(bbox.center().z(), 3.0);

    EXPECT_NEAR(bbox.diagonal(), std::sqrt(4.0 + 16.0 + 36.0), qi::test::kEpsTight);
    EXPECT_TRUE(bbox.isValid());
    EXPECT_FALSE(BoundingBox(Vec3(1, 0, 0), Vec3(0, 1, 1)).isValid());
}

TEST(GeometryBasics, TransformApplyAndInverse) {
    Quat rot(Eigen::AngleAxisd(qi::test::kPi / 2.0, Vec3::UnitZ()));
    Transform t(Vec3(1, 2, 3), rot);

    Vec3 p(1, 0, 0);
    Vec3 q = t.apply(p);
    Vec3 back = t.applyInverse(q);

    EXPECT_NEAR(back.x(), p.x(), qi::test::kEpsTight);
    EXPECT_NEAR(back.y(), p.y(), qi::test::kEpsTight);
    EXPECT_NEAR(back.z(), p.z(), qi::test::kEpsTight);
}

TEST(GeometryBasics, TransformIdentityIsNoOp) {
    Transform t = Transform::identity();
    Vec3 p(7, -3, 11);
    EXPECT_NEAR((t.apply(p) - p).norm(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR((t.applyInverse(p) - p).norm(), 0.0, qi::test::kEpsTight);
}

TEST(GeometryBasics, TransformToMatrixMatchesApply) {
    Quat rot(Eigen::AngleAxisd(0.7, Vec3(1, 1, 0).normalized()));
    Transform t(Vec3(2, -1, 5), rot);
    Eigen::Matrix4d m = t.toMatrix();

    Vec3 p(0.3, -0.4, 0.2);
    Eigen::Vector4d ph(p.x(), p.y(), p.z(), 1.0);
    Eigen::Vector4d qh = m * ph;
    Vec3 q = t.apply(p);

    EXPECT_NEAR(q.x(), qh.x(), qi::test::kEpsTight);
    EXPECT_NEAR(q.y(), qh.y(), qi::test::kEpsTight);
    EXPECT_NEAR(q.z(), qh.z(), qi::test::kEpsTight);
}

// ---- Ellipsoid ----

TEST(EllipsoidTest, ImplicitZeroOnParametricSurface) {
    Ellipsoid e(2.0, 3.0, 4.0);
    qi::test::expectParametricLiesOnImplicit(e, qi::test::kEpsTight);
}

TEST(EllipsoidTest, ImplicitSignsInsideOutside) {
    Ellipsoid e(2.0, 3.0, 4.0);
    EXPECT_LT(e.implicit(Vec3(0, 0, 0)), 0.0);
    EXPECT_GT(e.implicit(Vec3(10, 0, 0)), 0.0);
    EXPECT_NEAR(e.implicit(Vec3(2, 0, 0)), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(e.implicit(Vec3(0, 0, 4)), 0.0, qi::test::kEpsTight);
}

TEST(EllipsoidTest, TransformDoesNotBreakConsistency) {
    Ellipsoid e(2.0, 3.0, 4.0);
    e.setTransform(makeTestTransform());
    qi::test::expectParametricLiesOnImplicit(e, qi::test::kEpsTight);
}

TEST(EllipsoidTest, CloneCopiesParametersAndTransform) {
    Ellipsoid e(2.0, 3.0, 4.0);
    e.setTransform(makeTestTransform());

    auto cp = e.clone();
    auto* ep = dynamic_cast<Ellipsoid*>(cp.get());
    ASSERT_NE(ep, nullptr);
    EXPECT_DOUBLE_EQ(ep->a(), 2.0);
    EXPECT_DOUBLE_EQ(ep->b(), 3.0);
    EXPECT_DOUBLE_EQ(ep->c(), 4.0);
    EXPECT_NEAR((ep->transform().translation() - e.transform().translation()).norm(), 0.0,
                qi::test::kEpsTight);
}

TEST(EllipsoidTest, TypeName) {
    Ellipsoid e(1, 1, 1);
    EXPECT_EQ(e.typeName(), "ellipsoid");
}

// ---- HyperboloidOneSheet ----

TEST(HyperboloidOneSheetTest, ImplicitZeroOnParametricSurface) {
    HyperboloidOneSheet h(2.0, 3.0, 4.0);
    qi::test::expectParametricLiesOnImplicit(h, qi::test::kEpsTight);
}

TEST(HyperboloidOneSheetTest, TransformDoesNotBreakConsistency) {
    HyperboloidOneSheet h(2.0, 3.0, 4.0);
    h.setTransform(makeTestTransform());
    qi::test::expectParametricLiesOnImplicit(h, qi::test::kEpsTight);
}

TEST(HyperboloidOneSheetTest, CloneAndTypeName) {
    HyperboloidOneSheet h(1.5, 2.5, 3.5);
    auto cp = h.clone();
    auto* hp = dynamic_cast<HyperboloidOneSheet*>(cp.get());
    ASSERT_NE(hp, nullptr);
    EXPECT_DOUBLE_EQ(hp->a(), 1.5);
    EXPECT_DOUBLE_EQ(hp->c(), 3.5);
    EXPECT_EQ(h.typeName(), "hyperboloid_one_sheet");
}

// ---- HyperboloidTwoSheet ----

TEST(HyperboloidTwoSheetTest, ImplicitZeroOnParametricSurface) {
    HyperboloidTwoSheet h(2.0, 3.0, 4.0);
    qi::test::expectParametricLiesOnImplicit(h, qi::test::kEpsTight);
}

TEST(HyperboloidTwoSheetTest, BothSheetsAreReached) {
    HyperboloidTwoSheet h(1.0, 1.0, 1.0);
    Vec3 upper = h.parametric(0.0, 1.5);
    Vec3 lower = h.parametric(0.0, -1.5);
    EXPECT_GT(upper.z(), 0.0);
    EXPECT_LT(lower.z(), 0.0);
    EXPECT_NEAR(h.implicit(upper), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(h.implicit(lower), 0.0, qi::test::kEpsTight);
}

TEST(HyperboloidTwoSheetTest, TransformDoesNotBreakConsistency) {
    HyperboloidTwoSheet h(2.0, 3.0, 4.0);
    h.setTransform(makeTestTransform());
    qi::test::expectParametricLiesOnImplicit(h, qi::test::kEpsTight);
}

TEST(HyperboloidTwoSheetTest, TypeName) {
    HyperboloidTwoSheet h(1, 1, 1);
    EXPECT_EQ(h.typeName(), "hyperboloid_two_sheet");
}

// ---- EllipticParaboloid ----

TEST(EllipticParaboloidTest, ImplicitZeroOnParametricSurface) {
    EllipticParaboloid e(2.0, 3.0);
    qi::test::expectParametricLiesOnImplicit(e, qi::test::kEpsTight);
}

TEST(EllipticParaboloidTest, TransformDoesNotBreakConsistency) {
    EllipticParaboloid e(2.0, 3.0);
    e.setTransform(makeTestTransform());
    qi::test::expectParametricLiesOnImplicit(e, qi::test::kEpsTight);
}

TEST(EllipticParaboloidTest, TypeName) {
    EllipticParaboloid e(1, 1);
    EXPECT_EQ(e.typeName(), "elliptic_paraboloid");
}

// ---- HyperbolicParaboloid ----

TEST(HyperbolicParaboloidTest, ImplicitZeroOnParametricSurface) {
    HyperbolicParaboloid h(2.0, 3.0);
    qi::test::expectParametricLiesOnImplicit(h, qi::test::kEpsTight);
}

TEST(HyperbolicParaboloidTest, TransformDoesNotBreakConsistency) {
    HyperbolicParaboloid h(2.0, 3.0);
    h.setTransform(makeTestTransform());
    qi::test::expectParametricLiesOnImplicit(h, qi::test::kEpsTight);
}

TEST(HyperbolicParaboloidTest, TypeName) {
    HyperbolicParaboloid h(1, 1);
    EXPECT_EQ(h.typeName(), "hyperbolic_paraboloid");
}

// ---- Cone ----

TEST(ConeTest, ImplicitZeroOnParametricSurface) {
    Cone c(2.0, 3.0, 4.0);
    qi::test::expectParametricLiesOnImplicit(c, qi::test::kEpsTight);
}

TEST(ConeTest, VertexIsOnSurface) {
    Cone c(2.0, 3.0, 4.0);
    Vec3 vertex = c.parametric(0.5, 0.0);
    EXPECT_NEAR(vertex.norm(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(c.implicit(vertex), 0.0, qi::test::kEpsTight);
}

TEST(ConeTest, BothNappesReached) {
    Cone c(1.0, 1.0, 1.0);
    Vec3 upper = c.parametric(0.0, 2.0);
    Vec3 lower = c.parametric(0.0, -2.0);
    EXPECT_GT(upper.z(), 0.0);
    EXPECT_LT(lower.z(), 0.0);
    EXPECT_NEAR(c.implicit(upper), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(c.implicit(lower), 0.0, qi::test::kEpsTight);
}

TEST(ConeTest, TransformDoesNotBreakConsistency) {
    Cone c(2.0, 3.0, 4.0);
    c.setTransform(makeTestTransform());
    qi::test::expectParametricLiesOnImplicit(c, qi::test::kEpsTight);
}

TEST(ConeTest, TypeName) {
    Cone c(1, 1, 1);
    EXPECT_EQ(c.typeName(), "cone");
}

// ---- EllipticCylinder ----

TEST(EllipticCylinderTest, ImplicitZeroOnParametricSurface) {
    EllipticCylinder cyl(2.0, 3.0);
    qi::test::expectParametricLiesOnImplicit(cyl, qi::test::kEpsTight);
}

TEST(EllipticCylinderTest, TransformDoesNotBreakConsistency) {
    EllipticCylinder cyl(2.0, 3.0);
    cyl.setTransform(makeTestTransform());
    qi::test::expectParametricLiesOnImplicit(cyl, qi::test::kEpsTight);
}

TEST(EllipticCylinderTest, TypeName) {
    EllipticCylinder cyl(1, 1);
    EXPECT_EQ(cyl.typeName(), "elliptic_cylinder");
}

// ---- HyperbolicCylinder ----

TEST(HyperbolicCylinderTest, ImplicitZeroOnParametricSurface) {
    HyperbolicCylinder cyl(2.0, 3.0);
    qi::test::expectParametricLiesOnImplicit(cyl, qi::test::kEpsTight);
}

TEST(HyperbolicCylinderTest, BothBranchesReached) {
    HyperbolicCylinder cyl(1.0, 1.0);
    Vec3 right = cyl.parametric(1.0, 0.0);
    Vec3 left = cyl.parametric(-1.0, 0.0);
    EXPECT_GT(right.x(), 0.0);
    EXPECT_LT(left.x(), 0.0);
    EXPECT_NEAR(cyl.implicit(right), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(cyl.implicit(left), 0.0, qi::test::kEpsTight);
}

TEST(HyperbolicCylinderTest, TransformDoesNotBreakConsistency) {
    HyperbolicCylinder cyl(2.0, 3.0);
    cyl.setTransform(makeTestTransform());
    qi::test::expectParametricLiesOnImplicit(cyl, qi::test::kEpsTight);
}

TEST(HyperbolicCylinderTest, TypeName) {
    HyperbolicCylinder cyl(1, 1);
    EXPECT_EQ(cyl.typeName(), "hyperbolic_cylinder");
}

// ---- ParabolicCylinder ----

TEST(ParabolicCylinderTest, ImplicitZeroOnParametricSurface) {
    ParabolicCylinder cyl(2.0);
    qi::test::expectParametricLiesOnImplicit(cyl, qi::test::kEpsTight);
}

TEST(ParabolicCylinderTest, TransformDoesNotBreakConsistency) {
    ParabolicCylinder cyl(2.0);
    cyl.setTransform(makeTestTransform());
    qi::test::expectParametricLiesOnImplicit(cyl, qi::test::kEpsTight);
}

TEST(ParabolicCylinderTest, TypeName) {
    ParabolicCylinder cyl(1.0);
    EXPECT_EQ(cyl.typeName(), "parabolic_cylinder");
    EXPECT_DOUBLE_EQ(cyl.p(), 1.0);
}
