#pragma once

#include <string>

#include "BoundingBox.hpp"
#include "Mesh.hpp"
#include "Quadric.hpp"

namespace qi::triangulation {

class ITriangulator {
public:
    virtual ~ITriangulator() = default;

    virtual qi::mesh::Mesh triangulate(const qi::geometry::Quadric& quadric,
                                       const qi::geometry::BoundingBox& bbox) = 0;

    virtual std::string methodName() const = 0;
};

}  // namespace qi::triangulation
