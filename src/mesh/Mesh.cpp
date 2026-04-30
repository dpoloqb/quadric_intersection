#include "Mesh.hpp"

namespace qi::mesh {

void Mesh::clear() {
    vertices.clear();
    triangles.clear();
    normals.clear();
}

bool isValidMesh(const Mesh& mesh) {
    const auto vertexCount = mesh.vertices.size();
    for (const auto& tri : mesh.triangles) {
        for (auto idx : tri) {
            if (idx >= vertexCount) {
                return false;
            }
        }
    }
    if (!mesh.normals.empty() && mesh.normals.size() != vertexCount) {
        return false;
    }
    return true;
}

}  // namespace qi::mesh
