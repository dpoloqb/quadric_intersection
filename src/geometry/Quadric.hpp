#pragma once

#include <memory>
#include <string>
#include <utility>

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

    const Transform& transform() const { return transform_; }
    void setTransform(const Transform& transform) { transform_ = transform; }

protected:
    Transform transform_;
};

}  // namespace qi::geometry
