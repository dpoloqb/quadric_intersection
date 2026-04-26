#pragma once

#include <Eigen/Geometry>

#include "Vec3.hpp"

namespace qi::geometry {

class Transform {
public:
    Transform();
    Transform(const Vec3& translation, const Quat& rotation);

    static Transform identity();

    const Vec3& translation() const { return translation_; }
    const Quat& rotation() const { return rotation_; }

    void setTranslation(const Vec3& translation) { translation_ = translation; }
    void setRotation(const Quat& rotation) { rotation_ = rotation.normalized(); }

    Vec3 apply(const Vec3& p) const;
    Vec3 applyInverse(const Vec3& p) const;
    Eigen::Matrix4d toMatrix() const;

private:
    Vec3 translation_;
    Quat rotation_;
};

}  // namespace qi::geometry
