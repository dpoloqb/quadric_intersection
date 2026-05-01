#include <gtest/gtest.h>

#include <Eigen/Geometry>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

#include <QDir>
#include <QFile>
#include <QUuid>

#include "BoundingBox.hpp"
#include "ConfigJson.hpp"
#include "ExperimentConfig.hpp"
#include "ExperimentResult.hpp"
#include "ExperimentRunner.hpp"
#include "QuadricFactory.hpp"
#include "Transform.hpp"
#include "Vec3.hpp"
#include "test_utils.hpp"

using qi::experiment::ExperimentConfig;
using qi::experiment::ExperimentResult;
using qi::experiment::ExperimentRunner;
using qi::experiment::quadricParamsFromJson;
using qi::experiment::quadricParamsToJson;
using qi::experiment::SurfaceConfig;
using qi::experiment::SurfaceRecord;
using qi::experiment::transformFromJson;
using qi::experiment::transformToJson;
using qi::experiment::triangulationParamsFromJson;
using qi::experiment::triangulationParamsToJson;
using qi::geometry::BoundingBox;
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

// ---- ExperimentRunner ----

namespace {

SurfaceConfig sphereCfg(const Vec3& centre, double radius, int steps = 24) {
    SurfaceConfig sc;
    sc.type = "ellipsoid";
    sc.params = {radius, radius, radius, 1.0};
    sc.transform = Transform(centre, Quat::Identity());
    sc.triangulationMethod = "parametric";
    sc.uSteps = steps;
    sc.vSteps = steps;
    return sc;
}

}  // namespace

TEST(ExperimentRunnerTest, TwoIntersectingSpheresProduceOnePairResult) {
    ExperimentConfig cfg;
    cfg.bbox = BoundingBox(Vec3(-3, -3, -3), Vec3(5, 3, 3));
    cfg.intersectionMethod = "bvh";
    cfg.notes = "test";
    cfg.surfaces = {
        sphereCfg(Vec3(0, 0, 0), 2.0),
        sphereCfg(Vec3(2, 0, 0), 2.0),
    };

    ExperimentRunner runner;
    auto result = runner.run(cfg);

    ASSERT_EQ(result.surfaces.size(), 2u);
    ASSERT_EQ(result.intersections.size(), 1u);
    EXPECT_EQ(result.intersections[0].surface1Index, 0);
    EXPECT_EQ(result.intersections[0].surface2Index, 1);
    EXPECT_GT(result.intersections[0].timeIntersectionMs, 0.0);
    EXPECT_GT(result.intersections[0].segmentsCount, 0);
    EXPECT_EQ(result.intersections[0].polylinesCount, 1);
    for (const auto& s : result.surfaces) {
        EXPECT_GT(s.trianglesCount, 0);
        EXPECT_GT(s.timeTriangulationMs, 0.0);
    }
}

TEST(ExperimentRunnerTest, ThreeSurfacesProduceThreePairs) {
    ExperimentConfig cfg;
    cfg.bbox = BoundingBox(Vec3(-5, -5, -5), Vec3(7, 5, 5));
    cfg.intersectionMethod = "bvh";
    cfg.surfaces = {
        sphereCfg(Vec3(0, 0, 0), 2.0),
        sphereCfg(Vec3(2, 0, 0), 2.0),
        sphereCfg(Vec3(1, 1.5, 0), 1.5),
    };

    ExperimentRunner runner;
    auto result = runner.run(cfg);

    ASSERT_EQ(result.surfaces.size(), 3u);
    ASSERT_EQ(result.intersections.size(), 3u);  // C(3,2) = 3 pairs

    // Each pair (i<j) appears exactly once.
    std::set<std::pair<int, int>> seen;
    for (const auto& isec : result.intersections) {
        seen.emplace(isec.surface1Index, isec.surface2Index);
    }
    EXPECT_EQ(seen.size(), 3u);
    EXPECT_TRUE(seen.count({0, 1}) > 0);
    EXPECT_TRUE(seen.count({0, 2}) > 0);
    EXPECT_TRUE(seen.count({1, 2}) > 0);
}

TEST(ExperimentRunnerTest, DisjointSurfacesProduceZeroPolylines) {
    ExperimentConfig cfg;
    cfg.bbox = BoundingBox(Vec3(-2, -2, -2), Vec3(12, 2, 2));
    cfg.intersectionMethod = "bvh";
    cfg.surfaces = {
        sphereCfg(Vec3(0, 0, 0), 1.0),
        sphereCfg(Vec3(10, 0, 0), 1.0),
    };

    ExperimentRunner runner;
    auto result = runner.run(cfg);

    ASSERT_EQ(result.intersections.size(), 1u);
    EXPECT_EQ(result.intersections[0].segmentsCount, 0);
    EXPECT_EQ(result.intersections[0].polylinesCount, 0);
    // Even with no intersection, the record still exists with non-negative time.
    EXPECT_GE(result.intersections[0].timeIntersectionMs, 0.0);
    EXPECT_TRUE(std::isfinite(result.intersections[0].timeIntersectionMs));
}

TEST(ExperimentRunnerTest, ProgressCallbackFiresAtLeastOncePerStage) {
    ExperimentConfig cfg;
    cfg.bbox = BoundingBox(Vec3(-3, -3, -3), Vec3(5, 3, 3));
    cfg.intersectionMethod = "bvh";
    cfg.surfaces = {
        sphereCfg(Vec3(0, 0, 0), 2.0),
        sphereCfg(Vec3(2, 0, 0), 2.0),
    };

    int callCount = 0;
    int lastTotal = -1;
    int lastCurrent = -1;
    ExperimentRunner runner;
    runner.run(cfg, [&](const QString&, int current, int total) {
        ++callCount;
        lastCurrent = current;
        lastTotal = total;
    });

    // 2 triangulations + C(2,2)=1 intersection = 3 stages, plus the final
    // "done" tick → callCount >= 4.
    EXPECT_GE(callCount, 3);
    EXPECT_EQ(lastTotal, 3);
    EXPECT_EQ(lastCurrent, lastTotal);  // final tick = current==total
}

TEST(ExperimentRunnerTest, TimingIsPositiveAndFinite) {
    ExperimentConfig cfg;
    cfg.bbox = BoundingBox(Vec3(-3, -3, -3), Vec3(5, 3, 3));
    cfg.intersectionMethod = "bvh";
    cfg.surfaces = {
        sphereCfg(Vec3(0, 0, 0), 2.0),
        sphereCfg(Vec3(2, 0, 0), 2.0),
    };

    ExperimentRunner runner;
    auto result = runner.run(cfg);

    for (const auto& s : result.surfaces) {
        EXPECT_GT(s.timeTriangulationMs, 0.0);
        EXPECT_TRUE(std::isfinite(s.timeTriangulationMs));
    }
    for (const auto& isec : result.intersections) {
        EXPECT_GE(isec.timeIntersectionMs, 0.0);
        EXPECT_TRUE(std::isfinite(isec.timeIntersectionMs));
    }
}

TEST(ExperimentRunnerTest, MarchingCubesAndParametricMixed) {
    ExperimentConfig cfg;
    cfg.bbox = BoundingBox(Vec3(-3, -3, -3), Vec3(5, 3, 3));
    cfg.intersectionMethod = "bvh";
    SurfaceConfig s1 = sphereCfg(Vec3(0, 0, 0), 2.0);
    SurfaceConfig s2;
    s2.type = "ellipsoid";
    s2.params = {2.0, 2.0, 2.0, 1.0};
    s2.transform = Transform(Vec3(2, 0, 0), Quat::Identity());
    s2.triangulationMethod = "marching_cubes";
    s2.mcResolution = 24;
    cfg.surfaces = {s1, s2};

    ExperimentRunner runner;
    auto result = runner.run(cfg);
    ASSERT_EQ(result.surfaces.size(), 2u);
    EXPECT_EQ(result.surfaces[0].triangulationMethod, "parametric");
    EXPECT_EQ(result.surfaces[1].triangulationMethod, "marching_cubes");
    EXPECT_GT(result.surfaces[0].trianglesCount, 0);
    EXPECT_GT(result.surfaces[1].trianglesCount, 0);
    ASSERT_EQ(result.intersections.size(), 1u);
    EXPECT_GT(result.intersections[0].segmentsCount, 0);
}

TEST(ExperimentRunnerTest, ResultMetadataIsPopulated) {
    ExperimentConfig cfg;
    cfg.bbox = BoundingBox(Vec3(-3, -3, -3), Vec3(5, 3, 3));
    cfg.intersectionMethod = "bvh";
    cfg.notes = "metadata check";
    cfg.surfaces = {sphereCfg(Vec3(0, 0, 0), 2.0)};

    ExperimentRunner runner;
    auto result = runner.run(cfg);
    EXPECT_FALSE(result.createdAt.isEmpty());
    EXPECT_EQ(result.notes, "metadata check");
    EXPECT_EQ(result.id, 0);  // not yet saved
    EXPECT_NEAR((result.bbox.min() - cfg.bbox.min()).norm(), 0.0,
                qi::test::kEpsTight);
    EXPECT_NEAR((result.bbox.max() - cfg.bbox.max()).norm(), 0.0,
                qi::test::kEpsTight);
}

// ---- ConfigJson ----

namespace {

ExperimentConfig makeSampleConfig() {
    ExperimentConfig cfg;
    cfg.bbox = BoundingBox(Vec3(-3, -2, -1), Vec3(5, 4, 3));
    cfg.intersectionMethod = "naive";
    cfg.notes = "json test";

    SurfaceConfig s1;
    s1.type = "ellipsoid";
    s1.params = {2.5, 3.5, 4.5, 1.0};
    s1.transform = Transform(Vec3(1, 2, 3), Quat::Identity());
    s1.triangulationMethod = "parametric";
    s1.uSteps = 20;
    s1.vSteps = 30;

    SurfaceConfig s2;
    s2.type = "cone";
    s2.params = {1.0, 1.0, 2.0, 1.0};
    s2.transform = Transform::identity();
    s2.triangulationMethod = "marching_cubes";
    s2.mcResolution = 48;

    cfg.surfaces = {s1, s2};
    return cfg;
}

}  // namespace

TEST(ConfigJsonTest, RoundtripPreservesAllFields) {
    const auto in = makeSampleConfig();
    const QString json = qi::experiment::configToJson(in);
    const auto out = qi::experiment::configFromJson(json);

    EXPECT_NEAR((out.bbox.min() - in.bbox.min()).norm(), 0.0, qi::test::kEpsTight);
    EXPECT_NEAR((out.bbox.max() - in.bbox.max()).norm(), 0.0, qi::test::kEpsTight);
    EXPECT_EQ(out.intersectionMethod, in.intersectionMethod);
    EXPECT_EQ(out.notes, in.notes);
    ASSERT_EQ(out.surfaces.size(), in.surfaces.size());
    for (std::size_t i = 0; i < in.surfaces.size(); ++i) {
        EXPECT_EQ(out.surfaces[i].type, in.surfaces[i].type);
        EXPECT_DOUBLE_EQ(out.surfaces[i].params.a, in.surfaces[i].params.a);
        EXPECT_DOUBLE_EQ(out.surfaces[i].params.b, in.surfaces[i].params.b);
        EXPECT_DOUBLE_EQ(out.surfaces[i].params.c, in.surfaces[i].params.c);
        EXPECT_EQ(out.surfaces[i].triangulationMethod, in.surfaces[i].triangulationMethod);
        EXPECT_EQ(out.surfaces[i].uSteps, in.surfaces[i].uSteps);
        EXPECT_EQ(out.surfaces[i].vSteps, in.surfaces[i].vSteps);
        EXPECT_EQ(out.surfaces[i].mcResolution, in.surfaces[i].mcResolution);
    }
}

TEST(ConfigJsonTest, FileRoundtrip) {
    const auto in = makeSampleConfig();
    const QString path = QDir::temp().filePath(
        QString("qi_cfg_%1.json").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));

    ASSERT_TRUE(qi::experiment::saveConfigToFile(in, path));
    auto opt = qi::experiment::loadConfigFromFile(path);
    QFile::remove(path);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->surfaces.size(), in.surfaces.size());
    EXPECT_EQ(opt->intersectionMethod, in.intersectionMethod);
}

TEST(ConfigJsonTest, LoadFromMissingPathReturnsNullopt) {
    EXPECT_FALSE(qi::experiment::loadConfigFromFile("/nonexistent/path.json").has_value());
}

TEST(ConfigJsonTest, EmptySurfacesListSerializes) {
    ExperimentConfig cfg;
    cfg.bbox = BoundingBox(Vec3(0, 0, 0), Vec3(1, 1, 1));
    cfg.intersectionMethod = "bvh";
    const QString json = qi::experiment::configToJson(cfg);
    const auto out = qi::experiment::configFromJson(json);
    EXPECT_TRUE(out.surfaces.empty());
}
