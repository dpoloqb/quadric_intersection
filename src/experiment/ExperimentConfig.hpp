#pragma once

#include <QString>
#include <vector>

#include "BoundingBox.hpp"
#include "QuadricFactory.hpp"
#include "Transform.hpp"

namespace qi::experiment {

struct SurfaceConfig {
    QString type;  // matches Quadric::typeName(), e.g. "ellipsoid"
    qi::geometry::QuadricParams params{};
    qi::geometry::Transform transform{};
    QString triangulationMethod = "parametric";  // "marching_cubes" | "parametric"
    int mcResolution = 32;
    int uSteps = 40;
    int vSteps = 40;
};

struct ExperimentConfig {
    qi::geometry::BoundingBox bbox{};
    std::vector<SurfaceConfig> surfaces;
    QString intersectionMethod = "bvh";  // "naive" | "bvh"
    QString notes;
};

}  // namespace qi::experiment
