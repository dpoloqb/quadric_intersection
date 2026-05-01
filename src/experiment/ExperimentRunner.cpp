#include "ExperimentRunner.hpp"

#include <QDateTime>
#include <chrono>
#include <memory>
#include <utility>

#include "BvhIntersector.hpp"
#include "MarchingCubes.hpp"
#include "MarchingCubesParams.hpp"
#include "Mesh.hpp"
#include "NaiveIntersector.hpp"
#include "ParametricParams.hpp"
#include "ParametricTriangulator.hpp"
#include "Polyline.hpp"
#include "PolylineBuilder.hpp"
#include "Quadric.hpp"
#include "QuadricFactory.hpp"

namespace qi::experiment {

using qi::geometry::createQuadric;
using qi::intersection::buildPolylines;
using qi::intersection::BvhIntersector;
using qi::intersection::NaiveIntersector;
using qi::mesh::Mesh;
using qi::mesh::Segment;
using qi::triangulation::MarchingCubes;
using qi::triangulation::MarchingCubesParams;
using qi::triangulation::ParametricParams;
using qi::triangulation::ParametricTriangulator;

namespace {

double elapsedMs(std::chrono::steady_clock::time_point t0,
                 std::chrono::steady_clock::time_point t1) {
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

}  // namespace

ExperimentResult ExperimentRunner::run(const ExperimentConfig& config,
                                       ProgressCallback onProgress) {
    ExperimentResult result;
    result.createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    result.bbox = config.bbox;
    result.notes = config.notes;

    const int n = static_cast<int>(config.surfaces.size());
    const int totalIntersections = n * (n - 1) / 2;
    const int totalSteps = n + totalIntersections;
    int currentStep = 0;

    // Step 1: triangulate each surface.
    std::vector<Mesh> meshes;
    meshes.reserve(static_cast<std::size_t>(n));

    for (int i = 0; i < n; ++i) {
        const auto& sc = config.surfaces[i];
        if (onProgress) {
            onProgress(QString("triangulating %1 (%2/%3)")
                           .arg(sc.type)
                           .arg(i + 1)
                           .arg(n),
                       currentStep, totalSteps);
        }

        auto quadric = createQuadric(sc.type.toStdString(), sc.params);
        quadric->setTransform(sc.transform);

        const auto t0 = std::chrono::steady_clock::now();
        Mesh mesh;
        if (sc.triangulationMethod == "marching_cubes") {
            MarchingCubes mc(MarchingCubesParams{sc.mcResolution});
            mesh = mc.triangulate(*quadric, config.bbox);
        } else {
            ParametricTriangulator pt(ParametricParams{sc.uSteps, sc.vSteps});
            mesh = pt.triangulate(*quadric, config.bbox);
        }
        const auto t1 = std::chrono::steady_clock::now();

        SurfaceRecord rec;
        rec.indexInExperiment = i;
        rec.type = sc.type;
        rec.params = sc.params;
        rec.transform = sc.transform;
        rec.triangulationMethod = sc.triangulationMethod;
        rec.mcResolution = sc.mcResolution;
        rec.uSteps = sc.uSteps;
        rec.vSteps = sc.vSteps;
        rec.trianglesCount = static_cast<int>(mesh.triangleCount());
        rec.timeTriangulationMs = elapsedMs(t0, t1);
        result.surfaces.push_back(std::move(rec));
        meshes.push_back(std::move(mesh));

        ++currentStep;
    }

    // Step 2: pairwise intersect.
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (onProgress) {
                onProgress(QString("intersecting (%1, %2)").arg(i).arg(j),
                           currentStep, totalSteps);
            }

            const auto t0 = std::chrono::steady_clock::now();
            std::vector<Segment> segments;
            if (config.intersectionMethod == "naive") {
                NaiveIntersector ni;
                segments = ni.findSegments(meshes[i], meshes[j]);
            } else {
                BvhIntersector bi;
                segments = bi.findSegments(meshes[i], meshes[j]);
            }
            const auto polylines = buildPolylines(segments);
            const auto t1 = std::chrono::steady_clock::now();

            IntersectionRecord rec;
            rec.surface1Index = i;
            rec.surface2Index = j;
            rec.intersectionMethod = config.intersectionMethod;
            rec.timeIntersectionMs = elapsedMs(t0, t1);
            rec.segmentsCount = static_cast<int>(segments.size());
            rec.polylinesCount = static_cast<int>(polylines.size());
            result.intersections.push_back(std::move(rec));

            ++currentStep;
        }
    }

    if (onProgress) {
        onProgress(QStringLiteral("done"), totalSteps, totalSteps);
    }

    return result;
}

}  // namespace qi::experiment
