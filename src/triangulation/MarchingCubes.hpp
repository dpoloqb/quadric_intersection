#pragma once

#include "ITriangulator.hpp"
#include "MarchingCubesParams.hpp"

namespace qi::triangulation {

class MarchingCubes final : public ITriangulator {
public:
    MarchingCubes() = default;
    explicit MarchingCubes(const MarchingCubesParams& params);

    void setParams(const MarchingCubesParams& params) { params_ = params; }
    const MarchingCubesParams& params() const { return params_; }

    qi::mesh::Mesh triangulate(const qi::geometry::Quadric& quadric,
                               const qi::geometry::BoundingBox& bbox) override;

    std::string methodName() const override { return "marching_cubes"; }

private:
    MarchingCubesParams params_;
};

}  // namespace qi::triangulation
