#pragma once

#include <array>

#include "Vec3.hpp"

namespace qi::geometry {

class BoundingBox {
public:
    BoundingBox();
    BoundingBox(const Vec3& min, const Vec3& max);

    const Vec3& min() const { return min_; }
    const Vec3& max() const { return max_; }

    bool isValid() const;
    Vec3 size() const { return max_ - min_; }
    Vec3 center() const;
    double diagonal() const;

    bool contains(const Vec3& p) const;
    Vec3 clampPoint(const Vec3& p) const;
    bool intersectsSegment(const Vec3& a, const Vec3& b) const;
    std::array<Vec3, 8> corners() const;

private:
    Vec3 min_;
    Vec3 max_;
};

}  // namespace qi::geometry
