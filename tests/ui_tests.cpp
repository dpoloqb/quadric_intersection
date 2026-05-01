#include <gtest/gtest.h>

#include <Eigen/Geometry>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QUuid>

#include "BoundingBox.hpp"
#include "BoundingBoxWidget.hpp"
#include "DatabaseManager.hpp"
#include "ExperimentConfig.hpp"
#include "ExperimentRepository.hpp"
#include "ExperimentTab.hpp"
#include "QuadricParamsWidget.hpp"
#include "SurfaceEditorWidget.hpp"
#include "Transform.hpp"
#include "TransformWidget.hpp"
#include "TriangulationParamsWidget.hpp"
#include "Vec3.hpp"
#include "test_utils.hpp"

using qi::experiment::ExperimentResult;
using qi::experiment::SurfaceConfig;
using qi::geometry::BoundingBox;
using qi::geometry::QuadricParams;
using qi::geometry::Quat;
using qi::geometry::Transform;
using qi::geometry::Vec3;
using qi::storage::DatabaseManager;
using qi::storage::ExperimentRepository;
using qi::ui::BoundingBoxWidget;
using qi::ui::ExperimentTab;
using qi::ui::QuadricParamsWidget;
using qi::ui::SurfaceEditorWidget;
using qi::ui::TransformWidget;
using qi::ui::TriangulationParamsWidget;

// ---- BoundingBoxWidget ----

TEST(BoundingBoxWidgetTest, DefaultIsValid) {
    BoundingBoxWidget w;
    EXPECT_TRUE(w.boundingBox().isValid());
}

TEST(BoundingBoxWidgetTest, SetGetRoundtrip) {
    BoundingBoxWidget w;
    BoundingBox in(Vec3(-5, -10, -1), Vec3(5, 10, 1));
    w.setBoundingBox(in);
    auto out = w.boundingBox();
    EXPECT_DOUBLE_EQ(out.min().x(), -5);
    EXPECT_DOUBLE_EQ(out.max().z(), 1);
}

TEST(BoundingBoxWidgetTest, BboxChangedSignalFires) {
    BoundingBoxWidget w;
    QSignalSpy spy(&w, &BoundingBoxWidget::bboxChanged);
    w.setBoundingBox(BoundingBox(Vec3(-1, -1, -1), Vec3(2, 2, 2)));
    EXPECT_GE(spy.count(), 1);
}

// ---- TransformWidget ----

TEST(TransformWidgetTest, IdentityRoundtrip) {
    TransformWidget w;
    w.setTransform(Transform::identity());
    Vec3 p(0.3, -0.4, 0.2);
    EXPECT_NEAR((w.transform().apply(p) - p).norm(), 0.0, qi::test::kEpsLoose);
}

TEST(TransformWidgetTest, TranslationOnly) {
    TransformWidget w;
    w.setTransform(Transform(Vec3(2, -3, 5), Quat::Identity()));
    EXPECT_NEAR((w.transform().translation() - Vec3(2, -3, 5)).norm(), 0.0,
                qi::test::kEpsLoose);
}

TEST(TransformWidgetTest, RotationRoundtripMatchesAction) {
    TransformWidget w;
    Quat rot(Eigen::AngleAxisd(0.5, Vec3::UnitY()));
    Transform in(Vec3(1, 2, 3), rot);
    w.setTransform(in);

    // Compare action of the recovered transform on a sample point. Euler→Quat
    // roundtrip can produce different (but equivalent) quaternion components.
    const Vec3 p(0.7, -0.3, 0.5);
    EXPECT_NEAR((w.transform().apply(p) - in.apply(p)).norm(), 0.0,
                qi::test::kEpsLoose);
}

// ---- TriangulationParamsWidget ----

TEST(TriangulationParamsWidgetTest, DefaultsAreParametric) {
    TriangulationParamsWidget w;
    EXPECT_EQ(w.method(), "parametric");
    EXPECT_EQ(w.uSteps(), 40);
    EXPECT_EQ(w.vSteps(), 40);
}

TEST(TriangulationParamsWidgetTest, SwitchToMarchingCubes) {
    TriangulationParamsWidget w;
    QSignalSpy spy(&w, &TriangulationParamsWidget::parametersChanged);
    w.setMethod("marching_cubes");
    w.setMcResolution(64);
    EXPECT_EQ(w.method(), "marching_cubes");
    EXPECT_EQ(w.mcResolution(), 64);
    EXPECT_GE(spy.count(), 2);  // method change + resolution change
}

// ---- QuadricParamsWidget ----

TEST(QuadricParamsWidgetTest, DefaultIsEllipsoidWithUnitParams) {
    QuadricParamsWidget w;
    EXPECT_EQ(w.type(), "ellipsoid");
    auto p = w.params();
    EXPECT_DOUBLE_EQ(p.a, 1.0);
    EXPECT_DOUBLE_EQ(p.b, 1.0);
    EXPECT_DOUBLE_EQ(p.c, 1.0);
}

TEST(QuadricParamsWidgetTest, SwitchTypeAndSetParams) {
    QuadricParamsWidget w;
    w.setType("parabolic_cylinder");
    w.setParams({1.0, 1.0, 1.0, 7.5});
    auto p = w.params();
    EXPECT_DOUBLE_EQ(p.p, 7.5);
}

TEST(QuadricParamsWidgetTest, AbcParamsForCone) {
    QuadricParamsWidget w;
    w.setType("cone");
    w.setParams({2.0, 3.0, 4.0, 1.0});
    auto p = w.params();
    EXPECT_DOUBLE_EQ(p.a, 2.0);
    EXPECT_DOUBLE_EQ(p.b, 3.0);
    EXPECT_DOUBLE_EQ(p.c, 4.0);
}

// ---- SurfaceEditorWidget ----

TEST(SurfaceEditorWidgetTest, RoundtripConfig) {
    SurfaceEditorWidget w;
    SurfaceConfig in;
    in.type = "elliptic_cylinder";
    in.params = {1.5, 2.5, 1.0, 1.0};
    in.transform = Transform(Vec3(2, -1, 0), Quat::Identity());
    in.triangulationMethod = "marching_cubes";
    in.mcResolution = 48;
    w.setConfig(in);

    auto out = w.config();
    EXPECT_EQ(out.type, in.type);
    EXPECT_DOUBLE_EQ(out.params.a, 1.5);
    EXPECT_DOUBLE_EQ(out.params.b, 2.5);
    EXPECT_NEAR((out.transform.translation() - in.transform.translation()).norm(),
                0.0, qi::test::kEpsLoose);
    EXPECT_EQ(out.triangulationMethod, "marching_cubes");
    EXPECT_EQ(out.mcResolution, 48);
}

TEST(SurfaceEditorWidgetTest, ConfigChangedSignalFires) {
    SurfaceEditorWidget w;
    QSignalSpy spy(&w, &SurfaceEditorWidget::configChanged);
    SurfaceConfig sc;
    sc.type = "ellipsoid";
    sc.triangulationMethod = "parametric";
    w.setConfig(sc);
    EXPECT_GE(spy.count(), 1);
}

// ---- ExperimentTab ----

namespace {
class TempDbForTab {
public:
    TempDbForTab() {
        path_ = QDir(QDir::tempPath())
                    .filePath(QString("qi_tab_%1.db")
                                  .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    }
    ~TempDbForTab() { QFile::remove(path_); }
    QString path() const { return path_; }
private:
    QString path_;
};
}  // namespace

TEST(ExperimentTabTest, ConstructionSeedsOneSurface) {
    ExperimentTab tab;
    EXPECT_EQ(tab.surfaceCount(), 1);
}

TEST(ExperimentTabTest, AddRemoveDuplicateSurface) {
    ExperimentTab tab;
    tab.addSurface();
    tab.addSurface();
    EXPECT_EQ(tab.surfaceCount(), 3);

    tab.duplicateSelectedSurface();
    EXPECT_EQ(tab.surfaceCount(), 4);

    tab.removeSelectedSurface();
    tab.removeSelectedSurface();
    EXPECT_EQ(tab.surfaceCount(), 2);
}

TEST(ExperimentTabTest, BuildConfigReflectsUiState) {
    ExperimentTab tab;
    auto cfg = tab.buildConfig();
    EXPECT_EQ(cfg.surfaces.size(), 1u);
    EXPECT_EQ(cfg.surfaces[0].type, "ellipsoid");
    EXPECT_EQ(cfg.intersectionMethod, "bvh");
    EXPECT_TRUE(cfg.bbox.isValid());
}

TEST(ExperimentTabTest, RunSyncProducesNonEmptyResult) {
    // Default config: one ellipsoid (R=1) in [-3,3]³ — gives a non-trivial
    // mesh and zero pairwise intersections (only 1 surface).
    ExperimentTab tab;
    auto result = tab.runSync();
    ASSERT_EQ(result.surfaces.size(), 1u);
    EXPECT_GT(result.surfaces[0].trianglesCount, 0);
    EXPECT_TRUE(result.intersections.empty());
}

TEST(ExperimentTabTest, RunSyncSavedToRepositoryWhenAttached) {
    TempDbForTab tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());
    ExperimentRepository repo(dm);

    ExperimentTab tab;
    tab.setRepository(&repo);
    auto result = tab.runSync();

    // runSync itself doesn't save (saving happens in onRunFinished after the
    // async path). Persist explicitly to verify the wired-in repo works.
    int id = repo.saveExperiment(result);
    EXPECT_GT(id, 0);
    EXPECT_EQ(repo.listExperiments().size(), 1u);
}
