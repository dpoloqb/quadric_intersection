#pragma once

#include <functional>

#include <QString>

#include "ExperimentConfig.hpp"
#include "ExperimentResult.hpp"

namespace qi::experiment {

/// Progress reporter signature.
///
/// @param stageDescription human-readable label, e.g.
///   `"triangulating ellipsoid (1/3)"` or `"intersecting (0,1)"`.
/// @param currentStep number of completed stages so far (0..totalSteps).
/// @param totalSteps `N` triangulations + `N·(N−1)/2` intersections.
using ProgressCallback =
    std::function<void(const QString& stageDescription, int currentStep, int totalSteps)>;

/// Orchestrates a full experiment without any UI dependency:
///   1. Build each surface via `qi::geometry::createQuadric`.
///   2. Triangulate each surface (Marching Cubes or Parametric per
///      `SurfaceConfig::triangulationMethod`).
///   3. Pairwise intersect every `(i < j)` pair using the chosen
///      `MeshIntersector` (Naive or BVH per `ExperimentConfig::intersectionMethod`).
///   4. Stitch each pair's segments into polylines (`buildPolylines`).
/// Times are measured with `std::chrono::steady_clock` and stored as
/// double-precision milliseconds.
///
/// Persistence is the caller's responsibility — `run()` returns a fully
/// populated `ExperimentResult` with `id == 0` and all `dbId == 0`. Pass it
/// to `qi::storage::ExperimentRepository::saveExperiment` to persist.
class ExperimentRunner {
public:
    ExperimentRunner() = default;

    /// Run a single experiment. May be invoked from any thread.
    ExperimentResult run(const ExperimentConfig& config,
                         ProgressCallback onProgress = {});
};

}  // namespace qi::experiment
