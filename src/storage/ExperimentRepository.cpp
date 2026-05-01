#include "ExperimentRepository.hpp"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <unordered_map>

#include "DatabaseManager.hpp"

namespace qi::storage {

using qi::experiment::ExperimentResult;
using qi::experiment::ExperimentSummary;
using qi::experiment::IntersectionRecord;
using qi::experiment::quadricParamsFromJson;
using qi::experiment::quadricParamsToJson;
using qi::experiment::SurfaceRecord;
using qi::experiment::transformFromJson;
using qi::experiment::transformToJson;
using qi::experiment::triangulationParamsFromJson;
using qi::experiment::triangulationParamsToJson;
using qi::geometry::BoundingBox;
using qi::geometry::Vec3;

int ExperimentRepository::saveExperiment(ExperimentResult& experiment) {
    QSqlDatabase db = db_.database();
    if (!db.isOpen()) return -1;

    if (!db.transaction()) return -1;

    // 1. INSERT experiments
    QSqlQuery insertExp(db);
    insertExp.prepare(R"sql(
        INSERT INTO experiments
            (created_at, bbox_min_x, bbox_min_y, bbox_min_z,
                         bbox_max_x, bbox_max_y, bbox_max_z,
                         surfaces_count, notes)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )sql");
    insertExp.addBindValue(experiment.createdAt);
    insertExp.addBindValue(experiment.bbox.min().x());
    insertExp.addBindValue(experiment.bbox.min().y());
    insertExp.addBindValue(experiment.bbox.min().z());
    insertExp.addBindValue(experiment.bbox.max().x());
    insertExp.addBindValue(experiment.bbox.max().y());
    insertExp.addBindValue(experiment.bbox.max().z());
    insertExp.addBindValue(experiment.surfacesCount());
    insertExp.addBindValue(experiment.notes);
    if (!insertExp.exec()) {
        db.rollback();
        return -1;
    }
    const int experimentId = insertExp.lastInsertId().toInt();
    experiment.id = experimentId;

    // 2. INSERT surfaces; record dbId per index for the next pass.
    QSqlQuery insertSurface(db);
    insertSurface.prepare(R"sql(
        INSERT INTO surfaces
            (experiment_id, index_in_experiment, type, params_json,
             transform_json, triangulation_method, triangulation_params_json,
             triangles_count, time_triangulation_ms)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )sql");
    for (auto& s : experiment.surfaces) {
        insertSurface.bindValue(0, experimentId);
        insertSurface.bindValue(1, s.indexInExperiment);
        insertSurface.bindValue(2, s.type);
        insertSurface.bindValue(3, quadricParamsToJson(s.params));
        insertSurface.bindValue(4, transformToJson(s.transform));
        insertSurface.bindValue(5, s.triangulationMethod);
        insertSurface.bindValue(6, triangulationParamsToJson(s));
        insertSurface.bindValue(7, s.trianglesCount);
        insertSurface.bindValue(8, s.timeTriangulationMs);
        if (!insertSurface.exec()) {
            db.rollback();
            return -1;
        }
        s.dbId = insertSurface.lastInsertId().toInt();
    }

    // 3. INSERT intersections, looking up surface dbIds via the index.
    QSqlQuery insertIntersection(db);
    insertIntersection.prepare(R"sql(
        INSERT INTO intersections
            (experiment_id, surface1_id, surface2_id, intersection_method,
             time_intersection_ms, segments_count, polylines_count)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )sql");
    for (auto& isec : experiment.intersections) {
        if (isec.surface1Index < 0 ||
            isec.surface1Index >= static_cast<int>(experiment.surfaces.size()) ||
            isec.surface2Index < 0 ||
            isec.surface2Index >= static_cast<int>(experiment.surfaces.size())) {
            db.rollback();
            return -1;
        }
        const int s1Id = experiment.surfaces[isec.surface1Index].dbId;
        const int s2Id = experiment.surfaces[isec.surface2Index].dbId;
        insertIntersection.bindValue(0, experimentId);
        insertIntersection.bindValue(1, s1Id);
        insertIntersection.bindValue(2, s2Id);
        insertIntersection.bindValue(3, isec.intersectionMethod);
        insertIntersection.bindValue(4, isec.timeIntersectionMs);
        insertIntersection.bindValue(5, isec.segmentsCount);
        insertIntersection.bindValue(6, isec.polylinesCount);
        if (!insertIntersection.exec()) {
            db.rollback();
            return -1;
        }
        isec.dbId = insertIntersection.lastInsertId().toInt();
    }

    if (!db.commit()) {
        db.rollback();
        return -1;
    }
    return experimentId;
}

std::optional<ExperimentResult> ExperimentRepository::loadExperiment(int id) {
    QSqlDatabase db = db_.database();
    if (!db.isOpen()) return std::nullopt;

    ExperimentResult exp;

    {
        QSqlQuery q(db);
        q.prepare(R"sql(
            SELECT created_at, bbox_min_x, bbox_min_y, bbox_min_z,
                               bbox_max_x, bbox_max_y, bbox_max_z, notes
            FROM experiments WHERE id = ?
        )sql");
        q.addBindValue(id);
        if (!q.exec() || !q.next()) return std::nullopt;
        exp.id = id;
        exp.createdAt = q.value(0).toString();
        const Vec3 mn(q.value(1).toDouble(), q.value(2).toDouble(),
                      q.value(3).toDouble());
        const Vec3 mx(q.value(4).toDouble(), q.value(5).toDouble(),
                      q.value(6).toDouble());
        exp.bbox = BoundingBox(mn, mx);
        exp.notes = q.value(7).toString();
    }

    // Map surface dbId → vector index for the intersections pass.
    std::unordered_map<int, int> surfaceIdToIndex;

    {
        QSqlQuery q(db);
        q.prepare(R"sql(
            SELECT id, index_in_experiment, type, params_json, transform_json,
                   triangulation_method, triangulation_params_json,
                   triangles_count, time_triangulation_ms
            FROM surfaces
            WHERE experiment_id = ?
            ORDER BY index_in_experiment ASC
        )sql");
        q.addBindValue(id);
        if (!q.exec()) return std::nullopt;
        while (q.next()) {
            SurfaceRecord s;
            s.dbId = q.value(0).toInt();
            s.indexInExperiment = q.value(1).toInt();
            s.type = q.value(2).toString();
            s.params = quadricParamsFromJson(q.value(3).toString());
            s.transform = transformFromJson(q.value(4).toString());
            s.triangulationMethod = q.value(5).toString();
            triangulationParamsFromJson(q.value(6).toString(), s);
            s.trianglesCount = q.value(7).toInt();
            s.timeTriangulationMs = q.value(8).toDouble();
            surfaceIdToIndex[s.dbId] = static_cast<int>(exp.surfaces.size());
            exp.surfaces.push_back(std::move(s));
        }
    }

    {
        QSqlQuery q(db);
        q.prepare(R"sql(
            SELECT id, surface1_id, surface2_id, intersection_method,
                   time_intersection_ms, segments_count, polylines_count
            FROM intersections
            WHERE experiment_id = ?
            ORDER BY id ASC
        )sql");
        q.addBindValue(id);
        if (!q.exec()) return std::nullopt;
        while (q.next()) {
            IntersectionRecord r;
            r.dbId = q.value(0).toInt();
            const int s1Id = q.value(1).toInt();
            const int s2Id = q.value(2).toInt();
            const auto it1 = surfaceIdToIndex.find(s1Id);
            const auto it2 = surfaceIdToIndex.find(s2Id);
            if (it1 == surfaceIdToIndex.end() || it2 == surfaceIdToIndex.end()) {
                // Inconsistent DB; surface row missing for this intersection.
                return std::nullopt;
            }
            r.surface1Index = it1->second;
            r.surface2Index = it2->second;
            r.intersectionMethod = q.value(3).toString();
            r.timeIntersectionMs = q.value(4).toDouble();
            r.segmentsCount = q.value(5).toInt();
            r.polylinesCount = q.value(6).toInt();
            exp.intersections.push_back(std::move(r));
        }
    }

    return exp;
}

std::vector<ExperimentSummary> ExperimentRepository::listExperiments() {
    std::vector<ExperimentSummary> out;
    QSqlDatabase db = db_.database();
    if (!db.isOpen()) return out;

    QSqlQuery q(db);
    if (!q.exec("SELECT id, created_at, surfaces_count, notes "
                "FROM experiments ORDER BY id ASC")) {
        return out;
    }
    while (q.next()) {
        ExperimentSummary s;
        s.id = q.value(0).toInt();
        s.createdAt = q.value(1).toString();
        s.surfacesCount = q.value(2).toInt();
        s.notes = q.value(3).toString();
        out.push_back(std::move(s));
    }
    return out;
}

bool ExperimentRepository::deleteExperiment(int id) {
    QSqlDatabase db = db_.database();
    if (!db.isOpen()) return false;

    QSqlQuery q(db);
    q.prepare("DELETE FROM experiments WHERE id = ?");
    q.addBindValue(id);
    if (!q.exec()) return false;
    return q.numRowsAffected() > 0;
}

}  // namespace qi::storage
