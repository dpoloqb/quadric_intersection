#pragma once

#include <QString>
#include <vector>

#include "BoundingBox.hpp"
#include "QuadricFactory.hpp"
#include "Transform.hpp"

namespace qi::experiment {

// All quadric/triangulation parameters in a single row, regardless of which
// method is actually used. The triangulation params helper serializes only
// the subset relevant to `triangulationMethod`.
struct SurfaceRecord {
    int indexInExperiment = 0;
    QString type;  // matches Quadric::typeName(), e.g. "ellipsoid"
    qi::geometry::QuadricParams params{};
    qi::geometry::Transform transform{};
    QString triangulationMethod;  // "marching_cubes" | "parametric"
    int mcResolution = 32;
    int uSteps = 40;
    int vSteps = 40;
    int trianglesCount = 0;
    double timeTriangulationMs = 0.0;
    int dbId = 0;  // populated after save
};

struct IntersectionRecord {
    int surface1Index = 0;  // index into ExperimentResult::surfaces
    int surface2Index = 0;
    QString intersectionMethod;  // "naive" | "bvh"
    double timeIntersectionMs = 0.0;
    int segmentsCount = 0;
    int polylinesCount = 0;
    int dbId = 0;
};

struct ExperimentResult {
    int id = 0;            // populated after save
    QString createdAt;     // ISO 8601 timestamp
    qi::geometry::BoundingBox bbox{};
    QString notes;
    std::vector<SurfaceRecord> surfaces;
    std::vector<IntersectionRecord> intersections;
    bool cancelled = false;  // set by runner when stopped via cancelToken

    int surfacesCount() const { return static_cast<int>(surfaces.size()); }
};

struct ExperimentSummary {
    int id = 0;
    QString createdAt;
    int surfacesCount = 0;
    QString notes;
};

// JSON helpers — used by ExperimentRepository to populate the *_json columns.
QString quadricParamsToJson(const qi::geometry::QuadricParams& params);
qi::geometry::QuadricParams quadricParamsFromJson(const QString& json);

QString transformToJson(const qi::geometry::Transform& t);
qi::geometry::Transform transformFromJson(const QString& json);

// Serializes uSteps/vSteps for "parametric" or resolution for "marching_cubes".
QString triangulationParamsToJson(const SurfaceRecord& s);
void triangulationParamsFromJson(const QString& json, SurfaceRecord& s);

}  // namespace qi::experiment
