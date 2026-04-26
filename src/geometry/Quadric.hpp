#pragma once

#include <memory>
#include <string>
#include <utility>

#include "Transform.hpp"
#include "Vec3.hpp"

namespace qi::geometry {

class Quadric {
public:
    virtual ~Quadric() = default;

    virtual double implicit(const Vec3& p) const = 0;
    virtual Vec3 parametric(double u, double v) const = 0;
    virtual std::pair<double, double> uRange() const = 0;
    virtual std::pair<double, double> vRange() const = 0;
    virtual std::string typeName() const = 0;
    virtual std::unique_ptr<Quadric> clone() const = 0;

    const Transform& transform() const { return transform_; }
    void setTransform(const Transform& transform) { transform_ = transform; }

protected:
    Transform transform_;
};

}  // namespace qi::geometry
