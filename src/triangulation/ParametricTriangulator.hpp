#pragma once

#include "ITriangulator.hpp"
#include "ParametricParams.hpp"

namespace qi::triangulation {

class ParametricTriangulator final : public ITriangulator {
public:
    ParametricTriangulator() = default;
    explicit ParametricTriangulator(const ParametricParams& params);

    void setParams(const ParametricParams& params) { params_ = params; }
    const ParametricParams& params() const { return params_; }

    qi::mesh::Mesh triangulate(const qi::geometry::Quadric& quadric,
                               const qi::geometry::BoundingBox& bbox) override;

    std::string methodName() const override { return "parametric"; }

private:
    ParametricParams params_;
};

}  // namespace qi::triangulation
