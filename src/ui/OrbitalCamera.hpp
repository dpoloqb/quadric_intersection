#pragma once

#include <QMatrix4x4>
#include <QVector3D>

namespace qi::ui {

// Yaw/pitch/distance camera orbiting around a target point. Yaw is rotation
// about the world-up axis (Y), pitch is rotation up/down clamped to avoid
// gimbal flip near the poles. View matrix is plain GL right-handed.
class OrbitalCamera {
public:
    OrbitalCamera() = default;

    void setTarget(const QVector3D& t);
    void setYawPitch(float yaw, float pitch);
    void setDistance(float d);

    QVector3D target() const { return target_; }
    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }
    float distance() const { return distance_; }

    QVector3D eye() const;
    QMatrix4x4 viewMatrix() const;

    // Interactions (called from mouse event handlers).
    void orbit(float deltaYawRad, float deltaPitchRad);
    void zoom(float factor);          // factor < 1 zoom in, > 1 zoom out
    void panScreen(float dxScreen, float dyScreen, int viewportHeightPx);

    // Frame the camera so that the bounding box of size `extent` (largest
    // axis) centered at `centre` fits the view at vertical FOV `fovYRad`.
    void frame(const QVector3D& centre, float extent, float fovYRad);

private:
    QVector3D target_{0.0f, 0.0f, 0.0f};
    float yaw_ = 0.6f;
    float pitch_ = 0.4f;
    float distance_ = 10.0f;
};

}  // namespace qi::ui
