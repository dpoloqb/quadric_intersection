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

    const auto uDiscs = quadric.uDiscontinuities();
    const auto vDiscs = quadric.vDiscontinuities();

    // True if some discontinuity falls inside the half-open interval
    // `(lo, hi]`. The half-open shape matches the right-continuous
    // convention used by `HyperboloidTwoSheet` / `HyperbolicCylinder`:
    // `parametric(0)` returns the upper sheet / right branch, while
    // `parametric(0-)` returns the lower / left side. So a quad spans the
    // discontinuity iff the discontinuity is strictly above `lo` and at
    // most equal to `hi`.
    auto crossesDisc = [](double a, double b, const std::vector<double>& discs) {
        const double lo = std::min(a, b);
        const double hi = std::max(a, b);
        for (double d : discs) {
            if (d > lo && d <= hi) return true;
        }
        return false;
    };

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
    // Quads that span a discontinuity in either parameter are skipped to
    // avoid bridging two disconnected sheets / branches.
    for (int j = 0; j < vs; ++j) {
        const double vLo = vMin + (vMax - vMin) * static_cast<double>(j) / vs;
        const double vHi = vMin + (vMax - vMin) * static_cast<double>(j + 1) / vs;
        if (crossesDisc(vLo, vHi, vDiscs)) continue;
        for (int i = 0; i < us; ++i) {
            const double uLo = uMin + (uMax - uMin) * static_cast<double>(i) / us;
            const double uHi = uMin + (uMax - uMin) * static_cast<double>(i + 1) / us;
            if (crossesDisc(uLo, uHi, uDiscs)) continue;

            const Vec3 p00 = at(i,     j    );
            const Vec3 p10 = at(i + 1, j    );
            const Vec3 p01 = at(i,     j + 1);
            const Vec3 p11 = at(i + 1, j + 1);

            emitFan(clipTriangleAgainstBbox(p00, p10, p11, bbox), out);
            emitFan(clipTriangleAgainstBbox(p00, p11, p01, bbox), out);
        }
    }

    // Close any sheet that ends adjacent to a discontinuity but lacks an
    // apex point in the grid (e.g. the lower sheet of HyperboloidTwoSheet,
    // since `parametric(u, 0)` is right-continuous and returns the *upper*
    // apex). The quadric supplies a synthetic apex via `closingApex*`;
    // we fan it to the nearest-on-that-side ring.
    auto vAt = [&](int j) { return vMin + (vMax - vMin) * j / vs; };
    auto uAt = [&](int i) { return uMin + (uMax - uMin) * i / us; };

    for (double d : vDiscs) {
        int jBelow = -1;
        for (int j = 0; j <= vs; ++j) {
            if (vAt(j) < d) jBelow = j;
        }
        if (jBelow >= 0) {
            if (auto apex = quadric.closingApexBelowV(d)) {
                for (int i = 0; i < us; ++i) {
                    if (crossesDisc(uAt(i), uAt(i + 1), uDiscs)) continue;
                    emitFan(clipTriangleAgainstBbox(at(i, jBelow), at(i + 1, jBelow),
                                                   *apex, bbox),
                            out);
                }
            }
        }

        int jAbove = vs + 1;
        for (int j = vs; j >= 0; --j) {
            if (vAt(j) > d) jAbove = j;
        }
        if (jAbove <= vs) {
            if (auto apex = quadric.closingApexAboveV(d)) {
                for (int i = 0; i < us; ++i) {
                    if (crossesDisc(uAt(i), uAt(i + 1), uDiscs)) continue;
                    emitFan(clipTriangleAgainstBbox(*apex, at(i + 1, jAbove),
                                                   at(i, jAbove), bbox),
                            out);
                }
            }
        }
    }

    for (double d : uDiscs) {
        int iBelow = -1;
        for (int i = 0; i <= us; ++i) {
            if (uAt(i) < d) iBelow = i;
        }
        if (iBelow >= 0) {
            if (auto apex = quadric.closingApexBelowU(d)) {
                for (int j = 0; j < vs; ++j) {
                    if (crossesDisc(vAt(j), vAt(j + 1), vDiscs)) continue;
                    emitFan(clipTriangleAgainstBbox(at(iBelow, j), at(iBelow, j + 1),
                                                   *apex, bbox),
                            out);
                }
            }
        }

        int iAbove = us + 1;
        for (int i = us; i >= 0; --i) {
            if (uAt(i) > d) iAbove = i;
        }
        if (iAbove <= us) {
            if (auto apex = quadric.closingApexAboveU(d)) {
                for (int j = 0; j < vs; ++j) {
                    if (crossesDisc(vAt(j), vAt(j + 1), vDiscs)) continue;
                    emitFan(clipTriangleAgainstBbox(*apex, at(iAbove, j + 1),
                                                   at(iAbove, j), bbox),
                            out);
                }
            }
        }
    }

    return out;
}

}  // namespace qi::triangulation
