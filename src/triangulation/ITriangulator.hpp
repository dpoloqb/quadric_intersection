#pragma once

#include <string>

#include "BoundingBox.hpp"
#include "Mesh.hpp"
#include "Quadric.hpp"

namespace qi::triangulation {

/// Strategy interface for "convert a quadric surface inside a bounding box
/// into a triangle mesh". Concrete implementations:
///   - `MarchingCubes` — scalar-field marching cubes on the implicit form.
///     Universal but isotropic; vertex precision ≈ `bbox.diagonal/resolution`.
///   - `ParametricTriangulator` — sweeps `(u, v)` over the parametric form
///     and clips by AABB via Sutherland-Hodgman. Vertex precision is exact
///     (within `kEpsTight`) but only for the part of the surface that fits
///     inside the bbox.
class ITriangulator {
public:
    virtual ~ITriangulator() = default;

    /// Build a mesh approximating `quadric` ∩ `bbox`.
    virtual qi::mesh::Mesh triangulate(const qi::geometry::Quadric& quadric,
                                       const qi::geometry::BoundingBox& bbox) = 0;

    /// Stable identifier (`"marching_cubes"` / `"parametric"`) stored in
    /// `SurfaceRecord` and the SQLite `surfaces.triangulation_method` column.
    virtual std::string methodName() const = 0;
};

}  // namespace qi::triangulation
