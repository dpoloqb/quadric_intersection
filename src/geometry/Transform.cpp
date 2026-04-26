#include "Transform.hpp"

namespace qi::geometry {

Transform::Transform() : translation_(Vec3::Zero()), rotation_(Quat::Identity()) {}

Transform::Transform(const Vec3& translation, const Quat& rotation)
    : translation_(translation), rotation_(rotation.normalized()) {}

Transform Transform::identity() { return Transform(); }

Vec3 Transform::apply(const Vec3& p) const { return rotation_ * p + translation_; }

Vec3 Transform::applyInverse(const Vec3& p) const {
    return rotation_.conjugate() * (p - translation_);
}

Eigen::Matrix4d Transform::toMatrix() const {
    Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
    m.block<3, 3>(0, 0) = rotation_.toRotationMatrix();
    m.block<3, 1>(0, 3) = translation_;
    return m;
}

}  // namespace qi::geometry
