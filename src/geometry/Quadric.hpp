#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Transform.hpp"
#include "Vec3.hpp"

namespace qi::geometry {

/// Abstract base class for the nine canonical quadric surfaces.
///
/// Each subclass exposes both an implicit form (sign tells inside/outside)
/// and an explicit parametric form `f(u, v) -> R^3`. The two are kept
/// consistent: every parametric point satisfies `implicit(parametric(u, v))
/// ≈ 0` within numerical tolerance — this is the property tested in
/// `geometry_tests.cpp`.
///
/// A rigid `Transform` is applied on top: derived classes invert it inside
/// `implicit()` (world → local) and apply it inside `parametric()`
/// (local → world). External code interacts only with world-space points.
class Quadric {
public:
    virtual ~Quadric() = default;

    /// Signed implicit value at a world-space point.
    /// Sign convention: < 0 inside the surface, > 0 outside, ≈ 0 on it.
    /// Magnitude is unit-less and depends on parametrization.
    virtual double implicit(const Vec3& p) const = 0;

    /// Point on the surface for the given parameters (world space).
    virtual Vec3 parametric(double u, double v) const = 0;

    /// Recommended parameter ranges. For unbounded surfaces (cylinders,
    /// paraboloids) returns a finite interval suitable for typical bbox sizes;
    /// the actual visible region is clipped externally.
    virtual std::pair<double, double> uRange() const = 0;
    virtual std::pair<double, double> vRange() const = 0;

    /// Stable string identifier matching `QuadricFactory::createQuadric` and
    /// the `surfaces.type` column in the SQLite schema.
    virtual std::string typeName() const = 0;

    /// Polymorphic deep copy, including the current transform.
    virtual std::unique_ptr<Quadric> clone() const = 0;

    /// Parameter values where `parametric()` is discontinuous along the
    /// respective axis. The parametric triangulator uses these to skip
    /// quads that would bridge two sheets / branches across a jump.
    /// Default: empty (continuous parametrization).
    /// Examples:
    ///   - `HyperboloidTwoSheet::vDiscontinuities()` → `{0.0}` (upper vs lower sheet)
    ///   - `HyperbolicCylinder::uDiscontinuities()` → `{0.0}` (right vs left branch)
    virtual std::vector<double> uDiscontinuities() const { return {}; }
    virtual std::vector<double> vDiscontinuities() const { return {}; }

    /// Optional "closing apex" point for the side of a discontinuity that
    /// is missing a degenerate ring in the parametric grid. Used by the
    /// triangulator to seal an otherwise open cone-like sheet.
    ///
    /// `closingApexBelowV(d)` is the limit `parametric(u, d-)` *iff* it is
    /// independent of `u` (a true apex). `closingApexAboveV(d)` is the
    /// `parametric(u, d+)` limit. Same for U. When the limit is not a
    /// single point (e.g. the two branches of `HyperbolicCylinder`), return
    /// `std::nullopt` and the triangulator leaves the sheet open.
    ///
    /// Concrete case: `HyperboloidTwoSheet::parametric(u, 0)` is right-
    /// continuous and returns the upper apex `(0, 0, +c)`, so the lower
    /// sheet has no apex in the grid. `closingApexBelowV(0)` returns
    /// `(0, 0, -c)` and the triangulator fans from there to the ring at
    /// the largest `v < 0`.
    virtual std::optional<Vec3> closingApexBelowU(double /*d*/) const { return std::nullopt; }
    virtual std::optional<Vec3> closingApexAboveU(double /*d*/) const { return std::nullopt; }
    virtual std::optional<Vec3> closingApexBelowV(double /*d*/) const { return std::nullopt; }
    virtual std::optional<Vec3> closingApexAboveV(double /*d*/) const { return std::nullopt; }

    const Transform& transform() const { return transform_; }
    void setTransform(const Transform& transform) { transform_ = transform; }

protected:
    Transform transform_;
};

}  // namespace qi::geometry
