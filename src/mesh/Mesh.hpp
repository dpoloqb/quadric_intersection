#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "Vec3.hpp"

namespace qi::mesh {

using Triangle = std::array<std::uint32_t, 3>;

struct Mesh {
    std::vector<qi::geometry::Vec3> vertices;
    std::vector<Triangle> triangles;
    std::vector<qi::geometry::Vec3> normals;  // optional, may be empty

    std::size_t triangleCount() const { return triangles.size(); }
    std::size_t vertexCount() const { return vertices.size(); }

    void clear();
};

bool isValidMesh(const Mesh& mesh);

}  // namespace qi::mesh
