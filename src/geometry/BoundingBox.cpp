#include "BoundingBox.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace qi::geometry {

namespace {
constexpr double kSlabEpsilon = 1e-12;
}  // namespace

BoundingBox::BoundingBox()
    : min_(Vec3::Constant(std::numeric_limits<double>::infinity())),
      max_(Vec3::Constant(-std::numeric_limits<double>::infinity())) {}

BoundingBox::BoundingBox(const Vec3& min, const Vec3& max) : min_(min), max_(max) {}

bool BoundingBox::isValid() const { return (min_.array() <= max_.array()).all(); }

Vec3 BoundingBox::center() const { return 0.5 * (min_ + max_); }

double BoundingBox::diagonal() const { return (max_ - min_).norm(); }

bool BoundingBox::contains(const Vec3& p) const {
    return (p.array() >= min_.array()).all() && (p.array() <= max_.array()).all();
}

Vec3 BoundingBox::clampPoint(const Vec3& p) const {
    return Vec3(std::clamp(p.x(), min_.x(), max_.x()),
                std::clamp(p.y(), min_.y(), max_.y()),
                std::clamp(p.z(), min_.z(), max_.z()));
}

bool BoundingBox::intersectsSegment(const Vec3& a, const Vec3& b) const {
    const Vec3 d = b - a;
    double tMin = 0.0;
    double tMax = 1.0;
    for (int i = 0; i < 3; ++i) {
        if (std::abs(d[i]) < kSlabEpsilon) {
            if (a[i] < min_[i] || a[i] > max_[i]) {
                return false;
            }
        } else {
            double t1 = (min_[i] - a[i]) / d[i];
            double t2 = (max_[i] - a[i]) / d[i];
            if (t1 > t2) {
                std::swap(t1, t2);
            }
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            if (tMin > tMax) {
                return false;
            }
        }
    }
    return true;
}

std::array<Vec3, 8> BoundingBox::corners() const {
    return {
        Vec3(min_.x(), min_.y(), min_.z()), Vec3(max_.x(), min_.y(), min_.z()),
        Vec3(min_.x(), max_.y(), min_.z()), Vec3(max_.x(), max_.y(), min_.z()),
        Vec3(min_.x(), min_.y(), max_.z()), Vec3(max_.x(), min_.y(), max_.z()),
        Vec3(min_.x(), max_.y(), max_.z()), Vec3(max_.x(), max_.y(), max_.z()),
    };
}

}  // namespace qi::geometry
