#include <gtest/gtest.h>

#include <Eigen/Geometry>
#include <QJsonDocument>
#include <QJsonObject>

#include "ExperimentResult.hpp"
#include "QuadricFactory.hpp"
#include "Transform.hpp"
#include "Vec3.hpp"
#include "test_utils.hpp"

using qi::experiment::quadricParamsFromJson;
using qi::experiment::quadricParamsToJson;
using qi::experiment::SurfaceRecord;
using qi::experiment::transformFromJson;
using qi::experiment::transformToJson;
using qi::experiment::triangulationParamsFromJson;
using qi::experiment::triangulationParamsToJson;
using qi::geometry::QuadricParams;
using qi::geometry::Quat;
using qi::geometry::Transform;
using qi::geometry::Vec3;

TEST(ExperimentJson, QuadricParamsRoundtrip) {
    QuadricParams in{2.5, 3.5, 4.5, 7.0};
    const QString json = quadricParamsToJson(in);
    const QuadricParams out = quadricParamsFromJson(json);
    EXPECT_DOUBLE_EQ(out.a, 2.5);
    EXPECT_DOUBLE_EQ(out.b, 3.5);
    EXPECT_DOUBLE_EQ(out.c, 4.5);
    EXPECT_DOUBLE_EQ(out.p, 7.0);
}

TEST(ExperimentJson, QuadricParamsDefaultsOnEmpty) {
    const QuadricParams out = quadricParamsFromJson("{}");
    EXPECT_DOUBLE_EQ(out.a, 1.0);
    EXPECT_DOUBLE_EQ(out.b, 1.0);
    EXPECT_DOUBLE_EQ(out.c, 1.0);
    EXPECT_DOUBLE_EQ(out.p, 1.0);
}

TEST(ExperimentJson, TransformRoundtrip) {
    Quat rot(Eigen::AngleAxisd(0.7, Vec3(1, 1, 0).normalized()));
    Transform in(Vec3(2, -1, 5), rot);

    const QString json = transformToJson(in);
    const Transform out = transformFromJson(json);

    EXPECT_NEAR((out.translation() - in.translation()).norm(), 0.0,
                qi::test::kEpsTight);
    // Quaternions q and -q encode the same rotation; compare the action on a
    // sample point instead of comparing components directly.
    const Vec3 p(0.3, -0.4, 0.2);
    EXPECT_NEAR((out.apply(p) - in.apply(p)).norm(), 0.0, qi::test::kEpsTight);
}

TEST(ExperimentJson, TransformIdentityRoundtrip) {
    Transform in = Transform::identity();
    const QString json = transformToJson(in);
    const Transform out = transformFromJson(json);
    const Vec3 p(7, -3, 11);
    EXPECT_NEAR((out.apply(p) - p).norm(), 0.0, qi::test::kEpsTight);
}

TEST(ExperimentJson, TriangulationParamsMarchingCubes) {
    SurfaceRecord in{};
    in.triangulationMethod = "marching_cubes";
    in.mcResolution = 64;

    const QString json = triangulationParamsToJson(in);
    SurfaceRecord out{};
    out.triangulationMethod = "marching_cubes";
    triangulationParamsFromJson(json, out);
    EXPECT_EQ(out.mcResolution, 64);
}

TEST(ExperimentJson, TriangulationParamsParametric) {
    SurfaceRecord in{};
    in.triangulationMethod = "parametric";
    in.uSteps = 50;
    in.vSteps = 70;

    const QString json = triangulationParamsToJson(in);
    SurfaceRecord out{};
    out.triangulationMethod = "parametric";
    triangulationParamsFromJson(json, out);
    EXPECT_EQ(out.uSteps, 50);
    EXPECT_EQ(out.vSteps, 70);
}

TEST(ExperimentJson, JsonIsValidUtf8AndCompact) {
    QuadricParams params{1.5, 2.5, 3.5, 4.5};
    const QString json = quadricParamsToJson(params);
    // Compact form has no spaces around colons.
    EXPECT_FALSE(json.contains(": "));
    // Round-trips through QJsonDocument cleanly.
    const auto doc = QJsonDocument::fromJson(json.toUtf8());
    EXPECT_FALSE(doc.isNull());
    EXPECT_TRUE(doc.isObject());
}
