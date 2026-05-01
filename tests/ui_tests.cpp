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
#include "ExperimentResult.hpp"
#include "OrbitalCamera.hpp"
#include "QuadricParamsWidget.hpp"
#include "ResultsTab.hpp"
#include "SurfaceEditorWidget.hpp"
#include "Transform.hpp"
#include "TransformWidget.hpp"
#include "TriangulationParamsWidget.hpp"
#include "Vec3.hpp"
#include "ViewTab.hpp"
#include "test_utils.hpp"

using qi::experiment::ExperimentResult;
using qi::experiment::IntersectionRecord;
using qi::experiment::SurfaceConfig;
using qi::experiment::SurfaceRecord;
using qi::geometry::BoundingBox;
using qi::geometry::QuadricParams;
using qi::geometry::Quat;
using qi::geometry::Transform;
using qi::geometry::Vec3;
using qi::storage::DatabaseManager;
using qi::storage::ExperimentRepository;
using qi::ui::BoundingBoxWidget;
using qi::ui::ExperimentTab;
using qi::ui::OrbitalCamera;
using qi::ui::QuadricParamsWidget;
using qi::ui::ResultsTab;
using qi::ui::SurfaceEditorWidget;
using qi::ui::TransformWidget;
using qi::ui::TriangulationParamsWidget;
using qi::ui::ViewTab;

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

TEST(ExperimentTabTest, ApplyConfigReplacesSurfaces) {
    ExperimentTab tab;
    qi::experiment::ExperimentConfig cfg;
    cfg.bbox = BoundingBox(Vec3(-2, -2, -2), Vec3(2, 2, 2));
    cfg.intersectionMethod = "naive";
    cfg.notes = "loaded";
    SurfaceConfig s;
    s.type = "cone";
    s.params = {1.0, 1.0, 2.0, 1.0};
    s.transform = Transform::identity();
    s.triangulationMethod = "marching_cubes";
    s.mcResolution = 48;
    cfg.surfaces = {s, s, s};

    tab.applyConfig(cfg);
    EXPECT_EQ(tab.surfaceCount(), 3);
    auto out = tab.buildConfig();
    EXPECT_EQ(out.intersectionMethod, "naive");
    EXPECT_EQ(out.notes, "loaded");
    EXPECT_EQ(out.surfaces.size(), 3u);
    EXPECT_EQ(out.surfaces[0].type, "cone");
}

TEST(ExperimentTabTest, SaveLoadConfigFileRoundtrip) {
    ExperimentTab tab;
    tab.addSurface();
    tab.addSurface();  // 3 surfaces total (1 default + 2)

    const QString path = QDir::temp().filePath(
        QString("qi_tab_cfg_%1.json").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    EXPECT_TRUE(tab.saveConfigToFile(path));

    ExperimentTab tab2;
    EXPECT_TRUE(tab2.loadConfigFromFile(path));
    QFile::remove(path);

    EXPECT_EQ(tab2.surfaceCount(), 3);
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

// ---- ResultsTab ----

namespace {

// Creates a tiny ExperimentResult with N surfaces and C(N,2) intersections —
// no actual mesh work, just data for the DB.
ExperimentResult makeStubExperiment(int surfaces, const QString& notes) {
    ExperimentResult exp;
    exp.createdAt = "2026-05-01T00:00:00Z";
    exp.bbox = qi::geometry::BoundingBox(Vec3(-1, -1, -1), Vec3(1, 1, 1));
    exp.notes = notes;
    for (int i = 0; i < surfaces; ++i) {
        SurfaceRecord s;
        s.indexInExperiment = i;
        s.type = (i % 2 == 0) ? "ellipsoid" : "cone";
        s.params = {1.0 + i, 1.0, 1.0, 1.0};
        s.transform = Transform::identity();
        s.triangulationMethod = "parametric";
        s.uSteps = 10;
        s.vSteps = 10;
        s.trianglesCount = 200;
        s.timeTriangulationMs = 1.0;
        exp.surfaces.push_back(s);
    }
    for (int i = 0; i < surfaces; ++i) {
        for (int j = i + 1; j < surfaces; ++j) {
            IntersectionRecord r;
            r.surface1Index = i;
            r.surface2Index = j;
            r.intersectionMethod = "bvh";
            r.timeIntersectionMs = 0.5;
            r.segmentsCount = 100;
            r.polylinesCount = 1;
            exp.intersections.push_back(r);
        }
    }
    return exp;
}

}  // namespace

TEST(ResultsTabTest, EmptyDatabaseShowsZeroRows) {
    TempDbForTab tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());
    ExperimentRepository repo(dm);

    ResultsTab tab;
    tab.setRepository(&repo);
    tab.setDatabase(dm.database());
    EXPECT_EQ(tab.pairsRowCount(), 0);
    EXPECT_EQ(tab.experimentsRowCount(), 0);
}

TEST(ResultsTabTest, RefreshShowsSavedRows) {
    TempDbForTab tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());
    ExperimentRepository repo(dm);

    ResultsTab tab;
    tab.setRepository(&repo);
    tab.setDatabase(dm.database());

    auto e1 = makeStubExperiment(3, "first");   // 3 pairs
    auto e2 = makeStubExperiment(2, "second");  // 1 pair
    repo.saveExperiment(e1);
    repo.saveExperiment(e2);
    tab.refresh();

    EXPECT_EQ(tab.experimentsRowCount(), 2);
    EXPECT_EQ(tab.pairsRowCount(), 4);  // 3 + 1
}

// ---- OrbitalCamera ----

TEST(OrbitalCameraTest, EyeMovesAroundTarget) {
    OrbitalCamera c;
    c.setTarget(QVector3D(1.0f, 2.0f, 3.0f));
    c.setDistance(5.0f);

    c.setYawPitch(0.0f, 0.0f);
    auto e0 = c.eye();
    EXPECT_NEAR(e0.x(), 1.0f, qi::test::kEpsLoose);
    EXPECT_NEAR(e0.y(), 2.0f, qi::test::kEpsLoose);
    EXPECT_NEAR(e0.z(), 3.0f + 5.0f, qi::test::kEpsLoose);

    c.setYawPitch(static_cast<float>(qi::test::kPi) / 2.0f, 0.0f);
    auto e1 = c.eye();
    EXPECT_NEAR(e1.x(), 1.0f + 5.0f, 1e-4f);
    EXPECT_NEAR(e1.z(), 3.0f, 1e-4f);
}

TEST(OrbitalCameraTest, PitchClamps) {
    OrbitalCamera c;
    c.setYawPitch(0.0f, 100.0f);  // far past pole
    EXPECT_LT(c.pitch(), 1.6f);   // clamp ≈ 1.5
    c.setYawPitch(0.0f, -100.0f);
    EXPECT_GT(c.pitch(), -1.6f);
}

TEST(OrbitalCameraTest, ZoomMultipliesDistance) {
    OrbitalCamera c;
    c.setDistance(10.0f);
    c.zoom(0.5f);
    EXPECT_NEAR(c.distance(), 5.0f, qi::test::kEpsLoose);
    c.zoom(2.0f);
    EXPECT_NEAR(c.distance(), 10.0f, qi::test::kEpsLoose);
}

TEST(OrbitalCameraTest, FrameSetsTargetAndDistance) {
    OrbitalCamera c;
    c.frame(QVector3D(2, 0, 0), 4.0f, 1.0f);
    EXPECT_NEAR(c.target().x(), 2.0f, qi::test::kEpsLoose);
    EXPECT_GT(c.distance(), 0.0f);
}

TEST(ResultsTabTest, ExportCsvWritesAllRowsWithHeader) {
    TempDbForTab tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());
    ExperimentRepository repo(dm);

    ResultsTab tab;
    tab.setRepository(&repo);
    tab.setDatabase(dm.database());

    auto e1 = makeStubExperiment(2, "csv test");
    repo.saveExperiment(e1);
    tab.refresh();

    const QString csvPath = QDir::temp().filePath(
        QString("qi_csv_%1.csv").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    ASSERT_TRUE(tab.exportPairsToCsv(csvPath));

    QFile f(csvPath);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = QString::fromUtf8(f.readAll());
    f.close();
    QFile::remove(csvPath);

    // Header + 1 data row + trailing newline = 2 non-empty lines.
    const auto lines = content.split('\n', Qt::SkipEmptyParts);
    EXPECT_EQ(lines.size(), 2);
    EXPECT_TRUE(lines[0].contains("id"));
    EXPECT_TRUE(lines[0].contains("segments"));
}

// ---- ViewTab ----

namespace {

ExperimentResult makeRealisticExperiment() {
    // Two intersecting R=2 spheres with low-res parametric grids — small
    // enough to triangulate fast in the test.
    ExperimentResult exp;
    exp.createdAt = "2026-05-01T00:00:00Z";
    exp.bbox = BoundingBox(Vec3(-3, -3, -3), Vec3(5, 3, 3));
    exp.notes = "viewtab roundtrip";

    SurfaceRecord s0;
    s0.indexInExperiment = 0;
    s0.type = "ellipsoid";
    s0.params = {2.0, 2.0, 2.0, 1.0};
    s0.transform = Transform::identity();
    s0.triangulationMethod = "parametric";
    s0.uSteps = 16;
    s0.vSteps = 16;
    exp.surfaces.push_back(s0);

    SurfaceRecord s1 = s0;
    s1.indexInExperiment = 1;
    s1.transform = Transform(Vec3(2, 0, 0), Quat::Identity());
    exp.surfaces.push_back(s1);

    IntersectionRecord ir;
    ir.surface1Index = 0;
    ir.surface2Index = 1;
    ir.intersectionMethod = "bvh";
    exp.intersections.push_back(ir);

    return exp;
}

}  // namespace

TEST(ViewTabTest, EmptyRepositoryShowsNoExperiments) {
    TempDbForTab tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());
    ExperimentRepository repo(dm);

    ViewTab tab;
    tab.setRepository(&repo);
    EXPECT_EQ(tab.experimentListSize(), 0);
}

TEST(ViewTabTest, RefreshPopulatesListFromRepository) {
    TempDbForTab tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());
    ExperimentRepository repo(dm);

    auto e1 = makeRealisticExperiment();
    auto e2 = makeRealisticExperiment();
    repo.saveExperiment(e1);
    repo.saveExperiment(e2);

    ViewTab tab;
    tab.setRepository(&repo);
    EXPECT_EQ(tab.experimentListSize(), 2);
}

TEST(ViewTabTest, LoadExperimentRecomputesMeshesAndPolylines) {
    TempDbForTab tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());
    ExperimentRepository repo(dm);

    auto exp = makeRealisticExperiment();
    int id = repo.saveExperiment(exp);
    ASSERT_GT(id, 0);

    ViewTab tab;
    tab.setRepository(&repo);
    tab.loadExperiment(id);

    // Two surfaces → 2 meshes; two intersecting R=2 spheres → 1 closed polyline.
    EXPECT_EQ(tab.currentMeshCount(), 2);
    EXPECT_GE(tab.currentPolylineCount(), 1);
}
