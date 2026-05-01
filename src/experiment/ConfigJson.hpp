#pragma once

#include <QString>
#include <optional>

#include "ExperimentConfig.hpp"

namespace qi::experiment {

// Serialize / deserialize an `ExperimentConfig` as a single self-contained
// JSON document. Reuses the same primitives as `ExperimentResult` JSON
// helpers (params, transform, triangulation params).
//
// Schema:
// {
//   "bbox": {"min": [x,y,z], "max": [x,y,z]},
//   "intersectionMethod": "bvh"|"naive",
//   "notes": "…",
//   "surfaces": [
//     {
//       "type": "ellipsoid"|…,
//       "params": {"a":…,"b":…,"c":…,"p":…},
//       "transform": {"translation":[x,y,z],"rotation":[x,y,z,w]},
//       "triangulationMethod": "parametric"|"marching_cubes",
//       "uSteps": …, "vSteps": …,
//       "mcResolution": …
//     }, …
//   ]
// }

QString configToJson(const ExperimentConfig& cfg);
ExperimentConfig configFromJson(const QString& json);

bool saveConfigToFile(const ExperimentConfig& cfg, const QString& path);
std::optional<ExperimentConfig> loadConfigFromFile(const QString& path);

}  // namespace qi::experiment
