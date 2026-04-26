#include <gtest/gtest.h>

#include <Eigen/Geometry>
#include <cmath>

#include "BoundingBox.hpp"
#include "Ellipsoid.hpp"
#include "Quadric.hpp"
#include "Transform.hpp"
#include "Vec3.hpp"
#include "test_utils.hpp"

using qi::geometry::BoundingBox;
using qi::geometry::Ellipsoid;
using qi::geometry::Quat;
using qi::geometry::Transform;
using qi::geometry::Vec3;

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

// --- Ellipsoid ---

TEST(EllipsoidTest, ImplicitZeroOnParametricSurface) {
    Ellipsoid e(2.0, 3.0, 4.0);
    qi::test::expectParametricLiesOnImplicit(e, qi::test::kEpsTight);
}

TEST(EllipsoidTest, ImplicitSignsInsideOutside) {
    Ellipsoid e(2.0, 3.0, 4.0);
    EXPECT_LT(e.implicit(Vec3(0, 0, 0)), 0.0);     // inside
    EXPECT_GT(e.implicit(Vec3(10, 0, 0)), 0.0);    // outside
    EXPECT_NEAR(e.implicit(Vec3(2, 0, 0)), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR(e.implicit(Vec3(0, 0, 4)), 0.0, qi::test::kEpsTight);
}

TEST(EllipsoidTest, TransformDoesNotBreakConsistency) {
    Ellipsoid e(2.0, 3.0, 4.0);
    Quat rot(Eigen::AngleAxisd(0.7, Vec3(1, 1, 0).normalized()));
    e.setTransform(Transform(Vec3(5, -2, 1), rot));
    qi::test::expectParametricLiesOnImplicit(e, qi::test::kEpsTight);
}

TEST(EllipsoidTest, CloneCopiesParametersAndTransform) {
    Ellipsoid e(2.0, 3.0, 4.0);
    Quat rot(Eigen::AngleAxisd(0.3, Vec3::UnitX()));
    e.setTransform(Transform(Vec3(1, 1, 1), rot));

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
