#pragma once

#include <functional>

#include <QString>

#include "ExperimentConfig.hpp"
#include "ExperimentResult.hpp"

namespace qi::experiment {

// Progress callback signature.
//   stageDescription — human-readable label like
//     "triangulating ellipsoid (1/3)" or "intersecting (0,1)";
//   currentStep      — number of completed stages so far (0..totalSteps);
//   totalSteps       — N triangulations + N·(N-1)/2 intersections.
using ProgressCallback =
    std::function<void(const QString& stageDescription, int currentStep, int totalSteps)>;

// Orchestrates a full experiment without any UI dependency:
//   1. Build each surface via QuadricFactory.
//   2. Triangulate each (Marching Cubes or Parametric per SurfaceConfig).
//   3. Pairwise intersect (Naive or BVH per ExperimentConfig::intersectionMethod).
//   4. Stitch each pair's segments into polylines.
// Times are measured with std::chrono::steady_clock and stored as
// double-precision milliseconds.
class ExperimentRunner {
public:
    ExperimentRunner() = default;

    // Returns a fully populated ExperimentResult. `id` and `dbId` fields are
    // 0 — persistence is the caller's responsibility.
    ExperimentResult run(const ExperimentConfig& config,
                         ProgressCallback onProgress = {});
};

}  // namespace qi::experiment
