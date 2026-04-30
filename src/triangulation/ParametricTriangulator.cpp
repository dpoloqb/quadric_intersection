#include "ParametricTriangulator.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace qi::triangulation {

using qi::geometry::BoundingBox;
using qi::geometry::Quadric;
using qi::geometry::Vec3;
using qi::mesh::Mesh;
using qi::mesh::Triangle;

namespace {

// Sutherland–Hodgman clip of a polygon against one bbox plane.
// axis: 0=x, 1=y, 2=z; sign: +1 keeps coord >= bound, -1 keeps coord <= bound.
std::vector<Vec3> clipAgainstPlane(const std::vector<Vec3>& poly,
                                   int axis, double bound, int sign) {
    std::vector<Vec3> out;
    if (poly.empty()) return out;

    auto signedDist = [&](const Vec3& p) -> double {
        return sign * (p[axis] - bound);
    };

    Vec3 prev = poly.back();
    double prevD = signedDist(prev);

    for (const Vec3& curr : poly) {
        const double currD = signedDist(curr);
        if (currD >= 0.0) {
            if (prevD < 0.0) {
                const double t = prevD / (prevD - currD);
                out.push_back(prev + t * (curr - prev));
            }
            out.push_back(curr);
        } else if (prevD >= 0.0) {
            const double t = prevD / (prevD - currD);
            out.push_back(prev + t * (curr - prev));
        }
        prev = curr;
        prevD = currD;
    }
    return out;
}

std::vector<Vec3> clipTriangleAgainstBbox(const Vec3& a, const Vec3& b, const Vec3& c,
                                          const BoundingBox& bbox) {
    std::vector<Vec3> poly = {a, b, c};
    poly = clipAgainstPlane(poly, 0, bbox.min().x(), +1);
    if (poly.size() < 3) return {};
    poly = clipAgainstPlane(poly, 0, bbox.max().x(), -1);
    if (poly.size() < 3) return {};
    poly = clipAgainstPlane(poly, 1, bbox.min().y(), +1);
    if (poly.size() < 3) return {};
    poly = clipAgainstPlane(poly, 1, bbox.max().y(), -1);
    if (poly.size() < 3) return {};
    poly = clipAgainstPlane(poly, 2, bbox.min().z(), +1);
    if (poly.size() < 3) return {};
    poly = clipAgainstPlane(poly, 2, bbox.max().z(), -1);
    if (poly.size() < 3) return {};
    return poly;
}

// Triangulate a convex polygon by fanning from poly[0]; append into mesh.
void emitFan(const std::vector<Vec3>& poly, Mesh& out) {
    if (poly.size() < 3) return;
    for (std::size_t i = 1; i + 1 < poly.size(); ++i) {
        const auto base = static_cast<std::uint32_t>(out.vertices.size());
        out.vertices.push_back(poly[0]);
        out.vertices.push_back(poly[i]);
        out.vertices.push_back(poly[i + 1]);
        out.triangles.push_back(Triangle{base, base + 1, base + 2});
    }
}

}  // namespace

ParametricTriangulator::ParametricTriangulator(const ParametricParams& params)
    : params_(params) {}

Mesh ParametricTriangulator::triangulate(const Quadric& quadric, const BoundingBox& bbox) {
    Mesh out;

    const int us = std::max(1, params_.uSteps);
    const int vs = std::max(1, params_.vSteps);

    const auto [uMin, uMax] = quadric.uRange();
    const auto [vMin, vMax] = quadric.vRange();

    // Pre-compute (us+1) × (vs+1) grid of parametric points.
    std::vector<Vec3> grid(static_cast<std::size_t>(us + 1) * (vs + 1));
    auto at = [&](int i, int j) -> Vec3& {
        return grid[static_cast<std::size_t>(j) * (us + 1) + i];
    };
    for (int j = 0; j <= vs; ++j) {
        const double v = vMin + (vMax - vMin) * static_cast<double>(j) / vs;
        for (int i = 0; i <= us; ++i) {
            const double u = uMin + (uMax - uMin) * static_cast<double>(i) / us;
            at(i, j) = quadric.parametric(u, v);
        }
    }

    // Each quad → 2 triangles → clip against bbox → fan-triangulate the result.
    for (int j = 0; j < vs; ++j) {
        for (int i = 0; i < us; ++i) {
            const Vec3 p00 = at(i,     j    );
            const Vec3 p10 = at(i + 1, j    );
            const Vec3 p01 = at(i,     j + 1);
            const Vec3 p11 = at(i + 1, j + 1);

            emitFan(clipTriangleAgainstBbox(p00, p10, p11, bbox), out);
            emitFan(clipTriangleAgainstBbox(p00, p11, p01, bbox), out);
        }
    }

    return out;
}

}  // namespace qi::triangulation
