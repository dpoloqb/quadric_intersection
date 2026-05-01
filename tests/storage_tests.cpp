#include <gtest/gtest.h>

#include <Eigen/Geometry>
#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include "BoundingBox.hpp"
#include "DatabaseManager.hpp"
#include "ExperimentRepository.hpp"
#include "ExperimentResult.hpp"
#include "Transform.hpp"
#include "Vec3.hpp"
#include "test_utils.hpp"

using qi::experiment::ExperimentResult;
using qi::experiment::IntersectionRecord;
using qi::experiment::SurfaceRecord;
using qi::geometry::BoundingBox;
using qi::geometry::Quat;
using qi::geometry::Transform;
using qi::geometry::Vec3;
using qi::storage::DatabaseManager;
using qi::storage::ExperimentRepository;
using qi::storage::kCurrentSchemaVersion;

namespace {

// Creates a fresh temporary SQLite file path; deletes it on destruction.
class TempDatabase {
public:
    TempDatabase() {
        const QString name =
            QString("qi_test_%1.db").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        path_ = QDir(QDir::tempPath()).filePath(name);
    }
    ~TempDatabase() { QFile::remove(path_); }

    TempDatabase(const TempDatabase&) = delete;
    TempDatabase& operator=(const TempDatabase&) = delete;

    QString path() const { return path_; }

private:
    QString path_;
};

ExperimentResult makeSampleExperiment() {
    ExperimentResult exp;
    exp.createdAt = "2026-05-01T12:00:00";
    exp.bbox = BoundingBox(Vec3(-3, -3, -3), Vec3(5, 3, 3));
    exp.notes = "two intersecting spheres";

    SurfaceRecord s0;
    s0.indexInExperiment = 0;
    s0.type = "ellipsoid";
    s0.params = {2.0, 2.0, 2.0, 1.0};
    s0.transform = Transform::identity();
    s0.triangulationMethod = "parametric";
    s0.uSteps = 32;
    s0.vSteps = 32;
    s0.trianglesCount = 2048;
    s0.timeTriangulationMs = 12.5;
    exp.surfaces.push_back(s0);

    SurfaceRecord s1;
    s1.indexInExperiment = 1;
    s1.type = "ellipsoid";
    s1.params = {2.0, 2.0, 2.0, 1.0};
    Quat rot(Eigen::AngleAxisd(0.3, Vec3::UnitX()));
    s1.transform = Transform(Vec3(2, 0, 0), rot);
    s1.triangulationMethod = "marching_cubes";
    s1.mcResolution = 64;
    s1.trianglesCount = 4096;
    s1.timeTriangulationMs = 87.0;
    exp.surfaces.push_back(s1);

    SurfaceRecord s2;
    s2.indexInExperiment = 2;
    s2.type = "cone";
    s2.params = {1.0, 1.0, 1.0, 1.0};
    s2.transform = Transform::identity();
    s2.triangulationMethod = "parametric";
    s2.uSteps = 24;
    s2.vSteps = 24;
    s2.trianglesCount = 1024;
    s2.timeTriangulationMs = 5.5;
    exp.surfaces.push_back(s2);

    IntersectionRecord i01;
    i01.surface1Index = 0;
    i01.surface2Index = 1;
    i01.intersectionMethod = "bvh";
    i01.timeIntersectionMs = 0.097;
    i01.segmentsCount = 400;
    i01.polylinesCount = 1;
    exp.intersections.push_back(i01);

    IntersectionRecord i02;
    i02.surface1Index = 0;
    i02.surface2Index = 2;
    i02.intersectionMethod = "naive";
    i02.timeIntersectionMs = 320.0;
    i02.segmentsCount = 50;
    i02.polylinesCount = 2;
    exp.intersections.push_back(i02);

    IntersectionRecord i12;
    i12.surface1Index = 1;
    i12.surface2Index = 2;
    i12.intersectionMethod = "bvh";
    i12.timeIntersectionMs = 0.05;
    i12.segmentsCount = 30;
    i12.polylinesCount = 1;
    exp.intersections.push_back(i12);

    return exp;
}

bool sameTransform(const Transform& a, const Transform& b, double tol) {
    if ((a.translation() - b.translation()).norm() > tol) return false;
    const Vec3 sample(0.3, -0.4, 0.2);
    return (a.apply(sample) - b.apply(sample)).norm() <= tol;
}

}  // namespace

// ---- DatabaseManager ----

TEST(DatabaseManagerTest, OpensFile) {
    TempDatabase tdb;
    DatabaseManager dm(tdb.path());
    EXPECT_TRUE(dm.isOpen());
}

TEST(DatabaseManagerTest, InitializeCreatesTables) {
    TempDatabase tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());

    QSqlQuery q(dm.database());
    ASSERT_TRUE(q.exec("SELECT name FROM sqlite_master WHERE type='table'"));
    QSet<QString> names;
    while (q.next()) names.insert(q.value(0).toString());
    EXPECT_TRUE(names.contains("experiments"));
    EXPECT_TRUE(names.contains("surfaces"));
    EXPECT_TRUE(names.contains("intersections"));
    EXPECT_TRUE(names.contains("schema_version"));
}

TEST(DatabaseManagerTest, InitializeIsIdempotent) {
    TempDatabase tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());
    EXPECT_TRUE(dm.initialize());  // running again must not fail
    EXPECT_EQ(dm.schemaVersion(), kCurrentSchemaVersion);
}

TEST(DatabaseManagerTest, SchemaVersionBeforeInitIsZero) {
    TempDatabase tdb;
    DatabaseManager dm(tdb.path());
    EXPECT_EQ(dm.schemaVersion(), 0);
}

TEST(DatabaseManagerTest, ForeignKeysEnabled) {
    TempDatabase tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());

    QSqlQuery q(dm.database());
    ASSERT_TRUE(q.exec("PRAGMA foreign_keys"));
    ASSERT_TRUE(q.next());
    EXPECT_EQ(q.value(0).toInt(), 1);
}

// ---- ExperimentRepository ----

TEST(ExperimentRepositoryTest, SaveAndLoadRoundtrip) {
    TempDatabase tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());
    ExperimentRepository repo(dm);

    ExperimentResult original = makeSampleExperiment();
    const int id = repo.saveExperiment(original);
    ASSERT_GT(id, 0);
    EXPECT_EQ(original.id, id);
    for (const auto& s : original.surfaces) EXPECT_GT(s.dbId, 0);
    for (const auto& isec : original.intersections) EXPECT_GT(isec.dbId, 0);

    auto loadedOpt = repo.loadExperiment(id);
    ASSERT_TRUE(loadedOpt.has_value());
    const ExperimentResult& loaded = *loadedOpt;

    EXPECT_EQ(loaded.id, id);
    EXPECT_EQ(loaded.createdAt, original.createdAt);
    EXPECT_EQ(loaded.notes, original.notes);
    EXPECT_NEAR((loaded.bbox.min() - original.bbox.min()).norm(), 0.0,
                qi::test::kEpsTight);
    EXPECT_NEAR((loaded.bbox.max() - original.bbox.max()).norm(), 0.0,
                qi::test::kEpsTight);

    ASSERT_EQ(loaded.surfaces.size(), original.surfaces.size());
    for (std::size_t i = 0; i < loaded.surfaces.size(); ++i) {
        const auto& a = loaded.surfaces[i];
        const auto& b = original.surfaces[i];
        EXPECT_EQ(a.indexInExperiment, b.indexInExperiment);
        EXPECT_EQ(a.type, b.type);
        EXPECT_DOUBLE_EQ(a.params.a, b.params.a);
        EXPECT_DOUBLE_EQ(a.params.b, b.params.b);
        EXPECT_DOUBLE_EQ(a.params.c, b.params.c);
        EXPECT_DOUBLE_EQ(a.params.p, b.params.p);
        EXPECT_TRUE(sameTransform(a.transform, b.transform, qi::test::kEpsTight));
        EXPECT_EQ(a.triangulationMethod, b.triangulationMethod);
        if (a.triangulationMethod == "marching_cubes") {
            EXPECT_EQ(a.mcResolution, b.mcResolution);
        } else {
            EXPECT_EQ(a.uSteps, b.uSteps);
            EXPECT_EQ(a.vSteps, b.vSteps);
        }
        EXPECT_EQ(a.trianglesCount, b.trianglesCount);
        EXPECT_DOUBLE_EQ(a.timeTriangulationMs, b.timeTriangulationMs);
    }

    ASSERT_EQ(loaded.intersections.size(), original.intersections.size());
    for (std::size_t i = 0; i < loaded.intersections.size(); ++i) {
        const auto& a = loaded.intersections[i];
        const auto& b = original.intersections[i];
        EXPECT_EQ(a.surface1Index, b.surface1Index);
        EXPECT_EQ(a.surface2Index, b.surface2Index);
        EXPECT_EQ(a.intersectionMethod, b.intersectionMethod);
        EXPECT_DOUBLE_EQ(a.timeIntersectionMs, b.timeIntersectionMs);
        EXPECT_EQ(a.segmentsCount, b.segmentsCount);
        EXPECT_EQ(a.polylinesCount, b.polylinesCount);
    }
}

TEST(ExperimentRepositoryTest, LoadMissingIdReturnsNullopt) {
    TempDatabase tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());
    ExperimentRepository repo(dm);
    EXPECT_FALSE(repo.loadExperiment(999).has_value());
}

TEST(ExperimentRepositoryTest, DeleteCascadesToChildren) {
    TempDatabase tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());
    ExperimentRepository repo(dm);

    ExperimentResult exp = makeSampleExperiment();
    const int id = repo.saveExperiment(exp);
    ASSERT_GT(id, 0);

    EXPECT_TRUE(repo.deleteExperiment(id));

    // Verify cascade via direct counts.
    auto countWhere = [&](const QString& sql) {
        QSqlQuery q(dm.database());
        q.prepare(sql);
        q.addBindValue(id);
        if (!q.exec() || !q.next()) return -1;
        return q.value(0).toInt();
    };
    EXPECT_EQ(countWhere("SELECT COUNT(*) FROM surfaces WHERE experiment_id = ?"), 0);
    EXPECT_EQ(countWhere("SELECT COUNT(*) FROM intersections WHERE experiment_id = ?"), 0);
}

TEST(ExperimentRepositoryTest, DeleteNonexistentReturnsFalse) {
    TempDatabase tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());
    ExperimentRepository repo(dm);
    EXPECT_FALSE(repo.deleteExperiment(999));
}

TEST(ExperimentRepositoryTest, ListReturnsAllExperiments) {
    TempDatabase tdb;
    DatabaseManager dm(tdb.path());
    ASSERT_TRUE(dm.initialize());
    ExperimentRepository repo(dm);

    EXPECT_TRUE(repo.listExperiments().empty());

    for (int i = 0; i < 3; ++i) {
        ExperimentResult exp = makeSampleExperiment();
        exp.notes = QString("experiment %1").arg(i);
        const int id = repo.saveExperiment(exp);
        ASSERT_GT(id, 0);
    }

    auto summaries = repo.listExperiments();
    ASSERT_EQ(summaries.size(), 3u);
    // Listed in id-ascending order, and surfaces_count is 3 for each.
    int prevId = 0;
    for (const auto& s : summaries) {
        EXPECT_GT(s.id, prevId);
        prevId = s.id;
        EXPECT_EQ(s.surfacesCount, 3);
        EXPECT_FALSE(s.notes.isEmpty());
    }
}
