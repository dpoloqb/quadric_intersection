#include "OrbitalCamera.hpp"

#include <QtMath>
#include <algorithm>
#include <cmath>

namespace qi::ui {

namespace {
constexpr float kPitchLimit = 1.5f;  // ~85.9°
constexpr float kMinDistance = 1e-3f;
constexpr float kMaxDistance = 1e6f;
}  // namespace

void OrbitalCamera::setTarget(const QVector3D& t) { target_ = t; }

void OrbitalCamera::setYawPitch(float yaw, float pitch) {
    yaw_ = yaw;
    pitch_ = std::clamp(pitch, -kPitchLimit, kPitchLimit);
}

void OrbitalCamera::setDistance(float d) {
    distance_ = std::clamp(d, kMinDistance, kMaxDistance);
}

QVector3D OrbitalCamera::eye() const {
    const float cp = std::cos(pitch_);
    const float sp = std::sin(pitch_);
    const float cy = std::cos(yaw_);
    const float sy = std::sin(yaw_);
    const QVector3D dir(cp * sy, sp, cp * cy);
    return target_ + dir * distance_;
}

QMatrix4x4 OrbitalCamera::viewMatrix() const {
    QMatrix4x4 m;
    m.lookAt(eye(), target_, QVector3D(0.0f, 1.0f, 0.0f));
    return m;
}

void OrbitalCamera::orbit(float deltaYawRad, float deltaPitchRad) {
    yaw_ += deltaYawRad;
    pitch_ = std::clamp(pitch_ + deltaPitchRad, -kPitchLimit, kPitchLimit);
}

void OrbitalCamera::zoom(float factor) {
    distance_ = std::clamp(distance_ * factor, kMinDistance, kMaxDistance);
}

void OrbitalCamera::panScreen(float dxScreen, float dyScreen, int viewportHeightPx) {
    if (viewportHeightPx <= 0) return;
    // Camera-local right and up axes.
    const QVector3D forward = (target_ - eye()).normalized();
    const QVector3D worldUp(0.0f, 1.0f, 0.0f);
    QVector3D right = QVector3D::crossProduct(forward, worldUp).normalized();
    QVector3D up = QVector3D::crossProduct(right, forward).normalized();
    // World-space delta scaled so that one screen height ≈ visible vertical
    // distance at the target depth (≈ 2 · distance · tan(fov/2)).
    constexpr float kFovYHalfTan = 0.5f;  // matches default 60° FOV in Viewport3D
    const float worldPerPixel = (2.0f * distance_ * kFovYHalfTan) / viewportHeightPx;
    target_ += -right * (dxScreen * worldPerPixel) + up * (dyScreen * worldPerPixel);
}

void OrbitalCamera::frame(const QVector3D& centre, float extent, float fovYRad) {
    target_ = centre;
    if (extent <= 0.0f) {
        distance_ = 10.0f;
        return;
    }
    // distance = (extent/2) / tan(fov/2), with a 1.5× margin for breathing room.
    const float halfTan = std::tan(fovYRad * 0.5f);
    distance_ = std::max((extent * 0.5f) / std::max(halfTan, 1e-6f) * 1.5f,
                         kMinDistance);
}

}  // namespace qi::ui
