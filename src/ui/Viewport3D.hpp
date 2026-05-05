#pragma once

#include <QColor>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPoint>
#include <array>
#include <memory>
#include <vector>

#include "BoundingBox.hpp"
#include "Mesh.hpp"
#include "Polyline.hpp"

namespace qi::ui {

class OrbitalCamera;

// Thin OpenGL 3.3-core renderer for triangulated quadric meshes plus their
// pairwise intersection polylines. Single ambient-plus-directional light;
// alpha-blended meshes; thick lines for intersections (via glLineWidth);
// bounding-box wireframe and world axes for orientation.
class Viewport3D : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    explicit Viewport3D(QWidget* parent = nullptr);
    ~Viewport3D() override;

    void clearScene();
    void addMesh(const qi::mesh::Mesh& mesh, const QColor& colorWithAlpha);
    void addPolyline(const qi::mesh::Polyline& polyline, const QColor& colorOpaque);
    void setSceneBoundingBox(const qi::geometry::BoundingBox& bbox);
    void resetCamera();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

private:
    struct GpuMesh {
        std::vector<float> vertexAttribs;  // [pos.xyz, normal.xyz] × N
        QOpenGLVertexArrayObject vao;
        QOpenGLBuffer vbo{QOpenGLBuffer::VertexBuffer};
        GLsizei vertexCount = 0;
        QColor color{255, 200, 100, 96};
        bool uploaded = false;
    };
    struct GpuLines {
        std::vector<float> positions;  // pos.xyz × N
        QOpenGLVertexArrayObject vao;
        QOpenGLBuffer vbo{QOpenGLBuffer::VertexBuffer};
        GLsizei vertexCount = 0;
        QColor color{255, 255, 255, 255};
        float lineWidth = 1.0f;
        bool uploaded = false;
    };

    void uploadAll();
    void rebuildBboxAndAxes();
    void uploadOneMesh(GpuMesh& m);
    void uploadOneLines(GpuLines& l);
    void drawMeshes();
    void drawLines();

    static std::vector<float> meshToFlatAttribs(const qi::mesh::Mesh& mesh);
    static std::vector<float> polylineToPositions(const qi::mesh::Polyline& poly);
    static std::vector<float> bboxWireframe(const qi::geometry::BoundingBox& b);
    // Single XYZ axis as a line segment through the bbox centre. `axis`: 0=X, 1=Y, 2=Z.
    static std::vector<float> axisLine(int axis, const qi::geometry::BoundingBox& b);
    void drawAxisLabels();

    std::unique_ptr<QOpenGLShaderProgram> meshProgram_;
    std::unique_ptr<QOpenGLShaderProgram> lineProgram_;

    std::vector<std::unique_ptr<GpuMesh>> meshes_;
    std::vector<std::unique_ptr<GpuLines>> polylines_;
    std::unique_ptr<GpuLines> bboxLines_;
    std::array<std::unique_ptr<GpuLines>, 3> axes_;  // X, Y, Z (red, green, blue)

    qi::geometry::BoundingBox bbox_;
    bool bboxValid_ = false;

    std::unique_ptr<OrbitalCamera> camera_;
    QPoint lastMouse_;
    Qt::MouseButton activeButton_ = Qt::NoButton;
    QMatrix4x4 projection_;
    bool initialized_ = false;
};

}  // namespace qi::ui
