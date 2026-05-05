#include "Viewport3D.hpp"

#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QVector4D>
#include <QWheelEvent>
#include <QtMath>
#include <algorithm>
#include <array>

#include "OrbitalCamera.hpp"

namespace qi::ui {

namespace {

constexpr float kFovYRad = 1.04719755f;  // 60°
constexpr float kNearClip = 0.05f;
constexpr float kFarClip = 5000.0f;
constexpr float kAmbient = 0.35f;
constexpr float kPolylineWidth = 3.0f;

const char* const kMeshVS = R"(
#version 330 core
layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNormal;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uModel;
out vec3 vNormal;
void main() {
    gl_Position = uProj * uView * uModel * vec4(inPos, 1.0);
    vNormal = mat3(uModel) * inNormal;
}
)";

const char* const kMeshFS = R"(
#version 330 core
in vec3 vNormal;
uniform vec4 uColor;
uniform vec3 uLightDir;
uniform float uAmbient;
out vec4 outColor;
void main() {
    vec3 n = length(vNormal) > 1e-6 ? normalize(vNormal) : vec3(0,0,1);
    float diff = max(dot(n, -normalize(uLightDir)), 0.0);
    float intensity = uAmbient + (1.0 - uAmbient) * diff;
    outColor = vec4(uColor.rgb * intensity, uColor.a);
}
)";

const char* const kLineVS = R"(
#version 330 core
layout(location=0) in vec3 inPos;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uModel;
void main() {
    gl_Position = uProj * uView * uModel * vec4(inPos, 1.0);
}
)";

const char* const kLineFS = R"(
#version 330 core
uniform vec4 uColor;
out vec4 outColor;
void main() {
    outColor = uColor;
}
)";

// X red, Y green, Z blue — standard 3D-tooling convention.
const std::array<QColor, 3> kAxisColors = {
    QColor(230, 90, 90, 230),
    QColor(100, 210, 100, 230),
    QColor(120, 160, 255, 230),
};
const std::array<const char*, 3> kAxisLabels = {"X", "Y", "Z"};

QColor defaultMeshColor(int index) {
    static const std::array<QColor, 6> palette = {
        QColor(220, 90, 90, 96),
        QColor(90, 180, 90, 96),
        QColor(90, 130, 220, 96),
        QColor(220, 180, 80, 96),
        QColor(180, 90, 200, 96),
        QColor(80, 200, 200, 96),
    };
    return palette[static_cast<std::size_t>(index) % palette.size()];
}

}  // namespace

Viewport3D::Viewport3D(QWidget* parent) : QOpenGLWidget(parent) {
    camera_ = std::make_unique<OrbitalCamera>();
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);
}

Viewport3D::~Viewport3D() {
    makeCurrent();
    meshes_.clear();
    polylines_.clear();
    bboxLines_.reset();
    for (auto& a : axes_) a.reset();
    meshProgram_.reset();
    lineProgram_.reset();
    doneCurrent();
}

void Viewport3D::clearScene() {
    if (initialized_) {
        makeCurrent();
        meshes_.clear();
        polylines_.clear();
        doneCurrent();
    } else {
        meshes_.clear();
        polylines_.clear();
    }
    update();
}

void Viewport3D::addMesh(const qi::mesh::Mesh& mesh, const QColor& color) {
    auto m = std::make_unique<GpuMesh>();
    m->vertexAttribs = meshToFlatAttribs(mesh);
    m->vertexCount = static_cast<GLsizei>(m->vertexAttribs.size() / 6);
    m->color = color.isValid() ? color : defaultMeshColor(static_cast<int>(meshes_.size()));
    meshes_.push_back(std::move(m));
    if (initialized_) {
        makeCurrent();
        uploadOneMesh(*meshes_.back());
        doneCurrent();
    }
    update();
}

void Viewport3D::addPolyline(const qi::mesh::Polyline& polyline, const QColor& color) {
    auto p = std::make_unique<GpuLines>();
    p->positions = polylineToPositions(polyline);
    p->vertexCount = static_cast<GLsizei>(p->positions.size() / 3);
    p->color = color;
    p->lineWidth = kPolylineWidth;
    polylines_.push_back(std::move(p));
    if (initialized_) {
        makeCurrent();
        uploadOneLines(*polylines_.back());
        doneCurrent();
    }
    update();
}

void Viewport3D::setSceneBoundingBox(const qi::geometry::BoundingBox& bbox) {
    bbox_ = bbox;
    bboxValid_ = bbox.isValid();
    if (initialized_) {
        makeCurrent();
        rebuildBboxAndAxes();
        doneCurrent();
    }
    resetCamera();
}

void Viewport3D::resetCamera() {
    if (!bboxValid_) {
        camera_->setTarget({0, 0, 0});
        camera_->setDistance(10.0f);
    } else {
        const auto c = bbox_.center();
        const QVector3D centre(static_cast<float>(c.x()), static_cast<float>(c.y()),
                               static_cast<float>(c.z()));
        const float extent = static_cast<float>(bbox_.diagonal());
        camera_->frame(centre, extent, kFovYRad);
    }
    camera_->setYawPitch(0.6f, 0.4f);
    update();
}

void Viewport3D::initializeGL() {
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.07f, 0.07f, 0.10f, 1.0f);

    meshProgram_ = std::make_unique<QOpenGLShaderProgram>();
    meshProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, kMeshVS);
    meshProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, kMeshFS);
    meshProgram_->link();

    lineProgram_ = std::make_unique<QOpenGLShaderProgram>();
    lineProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, kLineVS);
    lineProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, kLineFS);
    lineProgram_->link();

    initialized_ = true;
    uploadAll();
    rebuildBboxAndAxes();
}

void Viewport3D::resizeGL(int w, int h) {
    projection_.setToIdentity();
    const float aspect = h > 0 ? float(w) / float(h) : 1.0f;
    projection_.perspective(qRadiansToDegrees(kFovYRad), aspect, kNearClip, kFarClip);
}

void Viewport3D::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawMeshes();
    drawLines();
    drawAxisLabels();  // QPainter overlay; must come after all GL draws.
}

void Viewport3D::drawMeshes() {
    if (!meshProgram_ || meshes_.empty()) return;
    meshProgram_->bind();
    QMatrix4x4 model;
    meshProgram_->setUniformValue("uView", camera_->viewMatrix());
    meshProgram_->setUniformValue("uProj", projection_);
    meshProgram_->setUniformValue("uModel", model);
    meshProgram_->setUniformValue("uLightDir", QVector3D(-0.5f, -1.0f, -0.4f));
    meshProgram_->setUniformValue("uAmbient", kAmbient);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    for (const auto& m : meshes_) {
        if (!m->uploaded || m->vertexCount == 0) continue;
        meshProgram_->setUniformValue(
            "uColor",
            QVector4D(m->color.redF(), m->color.greenF(), m->color.blueF(), m->color.alphaF()));
        QOpenGLVertexArrayObject::Binder bind(&m->vao);
        glDrawArrays(GL_TRIANGLES, 0, m->vertexCount);
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
    meshProgram_->release();
}

void Viewport3D::drawLines() {
    if (!lineProgram_) return;
    lineProgram_->bind();
    QMatrix4x4 model;
    lineProgram_->setUniformValue("uView", camera_->viewMatrix());
    lineProgram_->setUniformValue("uProj", projection_);
    lineProgram_->setUniformValue("uModel", model);

    auto drawOne = [&](GpuLines& l, GLenum mode) {
        if (!l.uploaded || l.vertexCount == 0) return;
        glLineWidth(l.lineWidth);
        lineProgram_->setUniformValue(
            "uColor",
            QVector4D(l.color.redF(), l.color.greenF(), l.color.blueF(), l.color.alphaF()));
        QOpenGLVertexArrayObject::Binder bind(&l.vao);
        glDrawArrays(mode, 0, l.vertexCount);
    };

    // Bbox + axes first so polylines render on top.
    if (bboxLines_) drawOne(*bboxLines_, GL_LINES);
    for (auto& a : axes_) {
        if (a) drawOne(*a, GL_LINES);
    }
    for (auto& p : polylines_) drawOne(*p, GL_LINE_STRIP);

    lineProgram_->release();
}

void Viewport3D::uploadAll() {
    for (auto& m : meshes_) uploadOneMesh(*m);
    for (auto& p : polylines_) uploadOneLines(*p);
    if (bboxLines_) uploadOneLines(*bboxLines_);
    for (auto& a : axes_) {
        if (a) uploadOneLines(*a);
    }
}

void Viewport3D::uploadOneMesh(GpuMesh& m) {
    if (!m.vao.isCreated()) m.vao.create();
    if (!m.vbo.isCreated()) m.vbo.create();
    QOpenGLVertexArrayObject::Binder bind(&m.vao);
    m.vbo.bind();
    m.vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m.vbo.allocate(m.vertexAttribs.data(),
                   static_cast<int>(m.vertexAttribs.size() * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    m.vbo.release();
    m.uploaded = true;
}

void Viewport3D::uploadOneLines(GpuLines& l) {
    if (!l.vao.isCreated()) l.vao.create();
    if (!l.vbo.isCreated()) l.vbo.create();
    QOpenGLVertexArrayObject::Binder bind(&l.vao);
    l.vbo.bind();
    l.vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    l.vbo.allocate(l.positions.data(),
                   static_cast<int>(l.positions.size() * sizeof(float)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    l.vbo.release();
    l.uploaded = true;
}

void Viewport3D::rebuildBboxAndAxes() {
    if (!bboxValid_) {
        bboxLines_.reset();
        for (auto& a : axes_) a.reset();
        return;
    }
    if (!bboxLines_) bboxLines_ = std::make_unique<GpuLines>();
    bboxLines_->positions = bboxWireframe(bbox_);
    bboxLines_->vertexCount = static_cast<GLsizei>(bboxLines_->positions.size() / 3);
    bboxLines_->color = QColor(150, 150, 150, 200);
    bboxLines_->lineWidth = 1.0f;
    if (initialized_) uploadOneLines(*bboxLines_);

    for (int axis = 0; axis < 3; ++axis) {
        if (!axes_[axis]) axes_[axis] = std::make_unique<GpuLines>();
        axes_[axis]->positions = axisLine(axis, bbox_);
        axes_[axis]->vertexCount = static_cast<GLsizei>(axes_[axis]->positions.size() / 3);
        axes_[axis]->color = kAxisColors[axis];
        axes_[axis]->lineWidth = 2.0f;
        if (initialized_) uploadOneLines(*axes_[axis]);
    }
}

void Viewport3D::mousePressEvent(QMouseEvent* e) {
    activeButton_ = e->button();
    lastMouse_ = e->pos();
}

void Viewport3D::mouseMoveEvent(QMouseEvent* e) {
    if (activeButton_ == Qt::NoButton) return;
    const QPoint d = e->pos() - lastMouse_;
    lastMouse_ = e->pos();
    if (activeButton_ == Qt::LeftButton) {
        camera_->orbit(-d.x() * 0.01f, -d.y() * 0.01f);
    } else if (activeButton_ == Qt::RightButton) {
        camera_->panScreen(static_cast<float>(d.x()), static_cast<float>(d.y()),
                           height());
    }
    update();
}

void Viewport3D::mouseReleaseEvent(QMouseEvent* /*e*/) {
    activeButton_ = Qt::NoButton;
}

void Viewport3D::wheelEvent(QWheelEvent* e) {
    const float steps = e->angleDelta().y() / 120.0f;
    camera_->zoom(std::pow(0.9f, steps));
    update();
}

std::vector<float> Viewport3D::meshToFlatAttribs(const qi::mesh::Mesh& mesh) {
    std::vector<float> out;
    out.reserve(mesh.triangles.size() * 18);
    for (const auto& tri : mesh.triangles) {
        if (tri[0] >= mesh.vertices.size() ||
            tri[1] >= mesh.vertices.size() ||
            tri[2] >= mesh.vertices.size()) {
            continue;
        }
        const auto& a = mesh.vertices[tri[0]];
        const auto& b = mesh.vertices[tri[1]];
        const auto& c = mesh.vertices[tri[2]];
        const auto e1 = b - a;
        const auto e2 = c - a;
        auto n = e1.cross(e2);
        const double len = n.norm();
        if (len > 1e-12) n /= len;
        else n = {0.0, 0.0, 1.0};
        for (const auto* v : {&a, &b, &c}) {
            out.push_back(static_cast<float>(v->x()));
            out.push_back(static_cast<float>(v->y()));
            out.push_back(static_cast<float>(v->z()));
            out.push_back(static_cast<float>(n.x()));
            out.push_back(static_cast<float>(n.y()));
            out.push_back(static_cast<float>(n.z()));
        }
    }
    return out;
}

std::vector<float> Viewport3D::polylineToPositions(const qi::mesh::Polyline& poly) {
    std::vector<float> out;
    out.reserve(poly.points.size() * 3);
    for (const auto& p : poly.points) {
        out.push_back(static_cast<float>(p.x()));
        out.push_back(static_cast<float>(p.y()));
        out.push_back(static_cast<float>(p.z()));
    }
    return out;
}

std::vector<float> Viewport3D::bboxWireframe(const qi::geometry::BoundingBox& b) {
    const auto corners = b.corners();
    // Edge index pairs of a unit cube in the corner ordering used by
    // BoundingBox::corners() — see BoundingBox.cpp for the canonical order.
    static constexpr std::array<std::pair<int, int>, 12> kEdges = {{
        {0, 1}, {1, 3}, {3, 2}, {2, 0},
        {4, 5}, {5, 7}, {7, 6}, {6, 4},
        {0, 4}, {1, 5}, {3, 7}, {2, 6},
    }};
    std::vector<float> out;
    out.reserve(kEdges.size() * 6);
    for (const auto& [a, c] : kEdges) {
        for (int idx : {a, c}) {
            const auto& p = corners[idx];
            out.push_back(static_cast<float>(p.x()));
            out.push_back(static_cast<float>(p.y()));
            out.push_back(static_cast<float>(p.z()));
        }
    }
    return out;
}

namespace {

// Axes anchor at the world origin (0,0,0) and extend both ±axis. Reach per
// direction is half the bbox diagonal so axes always poke out of the box on
// the far side regardless of where the origin sits relative to the box.
double axisReach(const qi::geometry::BoundingBox& b) { return b.diagonal() * 0.55; }

// Endpoints (negTip, posTip) of axis `i` (0=X, 1=Y, 2=Z) in world space.
// The arrow / label is drawn at posTip; the line itself spans both ends.
std::pair<std::array<double, 3>, std::array<double, 3>> axisEndpoints(
    int axis, const qi::geometry::BoundingBox& b) {
    const double reach = axisReach(b);
    std::array<double, 3> negTip = {0.0, 0.0, 0.0};
    std::array<double, 3> posTip = {0.0, 0.0, 0.0};
    negTip[axis] = -reach;
    posTip[axis] = +reach;
    return {negTip, posTip};
}

}  // namespace

std::vector<float> Viewport3D::axisLine(int axis, const qi::geometry::BoundingBox& b) {
    const auto [from, to] = axisEndpoints(axis, b);
    return {static_cast<float>(from[0]), static_cast<float>(from[1]), static_cast<float>(from[2]),
            static_cast<float>(to[0]),   static_cast<float>(to[1]),   static_cast<float>(to[2])};
}

void Viewport3D::drawAxisLabels() {
    if (!bboxValid_) return;

    const QMatrix4x4 viewProj = projection_ * camera_->viewMatrix();
    const float W = static_cast<float>(width());
    const float H = static_cast<float>(height());

    auto project = [&](const std::array<double, 3>& p, QPointF& out) -> bool {
        const QVector4D clip =
            viewProj * QVector4D(static_cast<float>(p[0]), static_cast<float>(p[1]),
                                 static_cast<float>(p[2]), 1.0f);
        if (clip.w() <= 0.0f) return false;  // behind camera
        out = QPointF((clip.x() / clip.w() * 0.5f + 0.5f) * W,
                      (1.0f - (clip.y() / clip.w() * 0.5f + 0.5f)) * H);
        return true;
    };

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(11);
    painter.setFont(font);

    constexpr double kArrowLen = 12.0;       // pixels
    constexpr double kArrowHalfWidth = 4.5;  // pixels

    for (int axis = 0; axis < 3; ++axis) {
        const auto [negTipW, posTipW] = axisEndpoints(axis, bbox_);
        QPointF tipS, negS;
        if (!project(posTipW, tipS) || !project(negTipW, negS)) continue;

        QPointF dir = tipS - negS;
        const double dlen = std::hypot(dir.x(), dir.y());
        if (dlen < 1e-3) continue;
        dir /= dlen;
        const QPointF perp(-dir.y(), dir.x());

        // Arrowhead: filled triangle, tip at tipS, base perpendicular to dir.
        const QPointF base = tipS - dir * kArrowLen;
        QPolygonF tri;
        tri << tipS << (base + perp * kArrowHalfWidth) << (base - perp * kArrowHalfWidth);
        painter.setPen(Qt::NoPen);
        painter.setBrush(kAxisColors[axis]);
        painter.drawPolygon(tri);

        // Label past the arrow tip in the same direction.
        painter.setPen(kAxisColors[axis]);
        const QPointF labelPos = tipS + dir * 6.0 + perp * (-2.0);
        painter.drawText(labelPos, kAxisLabels[axis]);
    }
}

}  // namespace qi::ui
