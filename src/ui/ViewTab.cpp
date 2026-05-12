#include "ViewTab.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

#include "BoundingBox.hpp"
#include "BvhIntersector.hpp"
#include "ExperimentRepository.hpp"
#include "ExperimentResult.hpp"
#include "MarchingCubes.hpp"
#include "MarchingCubesParams.hpp"
#include "Mesh.hpp"
#include "NaiveIntersector.hpp"
#include "ParametricParams.hpp"
#include "ParametricTriangulator.hpp"
#include "Polyline.hpp"
#include "PolylineBuilder.hpp"
#include "Quadric.hpp"
#include "QuadricFactory.hpp"
#include "Viewport3D.hpp"

namespace qi::ui {

namespace {

QColor paletteColor(int idx) {
    static const std::array<QColor, 6> palette = {
        QColor(220, 90, 90, 110),
        QColor(90, 180, 90, 110),
        QColor(90, 130, 220, 110),
        QColor(220, 180, 80, 110),
        QColor(180, 90, 200, 110),
        QColor(80, 200, 200, 110),
    };
    return palette[static_cast<std::size_t>(idx) % palette.size()];
}

qi::mesh::Mesh triangulateRecord(const qi::experiment::SurfaceRecord& s,
                                 const qi::geometry::BoundingBox& bbox) {
    using qi::geometry::createQuadric;
    using qi::triangulation::MarchingCubes;
    using qi::triangulation::MarchingCubesParams;
    using qi::triangulation::ParametricParams;
    using qi::triangulation::ParametricTriangulator;

    auto q = createQuadric(s.type.toStdString(), s.params);
    q->setTransform(s.transform);
    if (s.triangulationMethod == "marching_cubes") {
        MarchingCubes mc(MarchingCubesParams{s.mcResolution});
        return mc.triangulate(*q, bbox);
    }
    ParametricTriangulator pt(ParametricParams{s.uSteps, s.vSteps});
    return pt.triangulate(*q, bbox);
}

std::vector<qi::mesh::Polyline> intersectMeshes(const qi::mesh::Mesh& a,
                                                const qi::mesh::Mesh& b,
                                                const QString& method) {
    using qi::intersection::buildPolylines;
    using qi::intersection::BvhIntersector;
    using qi::intersection::NaiveIntersector;
    std::vector<qi::mesh::Segment> segments;
    if (method == "naive") {
        NaiveIntersector ni;
        segments = ni.findSegments(a, b);
    } else {
        BvhIntersector bi;
        segments = bi.findSegments(a, b);
    }
    return buildPolylines(segments);
}

}  // namespace

ViewTab::ViewTab(QWidget* parent) : QWidget(parent) {
    experimentList_ = new QListWidget;
    experimentList_->setSelectionMode(QAbstractItemView::SingleSelection);

    viewport_ = new Viewport3D;

    bvhSurfaceCombo_ = new QComboBox;
    bvhSurfaceCombo_->addItem(tr("(off)"), -1);
    bvhSurfaceCombo_->setMinimumContentsLength(20);

    bvhDepthSpin_ = new QSpinBox;
    bvhDepthSpin_->setRange(-1, 0);
    bvhDepthSpin_->setValue(-1);
    bvhDepthSpin_->setSpecialValueText(tr("all"));
    bvhDepthSpin_->setToolTip(tr("BVH depth filter: -1 = show all levels, 0 = root, ..."));
    bvhDepthSpin_->setEnabled(false);

    auto* bvhBar = new QWidget;
    auto* bvhBarLayout = new QHBoxLayout(bvhBar);
    bvhBarLayout->setContentsMargins(4, 2, 4, 2);
    bvhBarLayout->addWidget(new QLabel(tr("BVH:")));
    bvhBarLayout->addWidget(bvhSurfaceCombo_);
    bvhBarLayout->addSpacing(12);
    bvhBarLayout->addWidget(new QLabel(tr("depth:")));
    bvhBarLayout->addWidget(bvhDepthSpin_);
    bvhBarLayout->addStretch(1);

    auto* rightPane = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    rightLayout->addWidget(bvhBar);
    rightLayout->addWidget(viewport_, 1);

    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(experimentList_);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    auto* main = new QVBoxLayout(this);
    main->addWidget(splitter);
    main->setContentsMargins(0, 0, 0, 0);

    connect(experimentList_, &QListWidget::currentRowChanged, this,
            [this](int) { onExperimentSelected(); });
    connect(bvhSurfaceCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ViewTab::onBvhSurfaceChanged);
    connect(bvhDepthSpin_, qOverload<int>(&QSpinBox::valueChanged), this,
            &ViewTab::onBvhDepthChanged);
}

void ViewTab::setRepository(qi::storage::ExperimentRepository* repo) {
    repo_ = repo;
    refreshExperimentList();
}

int ViewTab::experimentListSize() const { return experimentList_->count(); }
int ViewTab::currentMeshCount() const { return loadedMeshCount_; }
int ViewTab::currentPolylineCount() const { return loadedPolylineCount_; }

void ViewTab::refreshExperimentList() {
    experimentList_->clear();
    loadedMeshCount_ = 0;
    loadedPolylineCount_ = 0;
    viewport_->clearScene();
    if (repo_ == nullptr) return;
    const auto summaries = repo_->listExperiments();
    for (const auto& s : summaries) {
        auto* item = new QListWidgetItem(QString("#%1 — %2").arg(s.id).arg(s.createdAt));
        item->setData(Qt::UserRole, s.id);
        item->setToolTip(s.notes);
        experimentList_->addItem(item);
    }
}

void ViewTab::onExperimentSelected() {
    auto* item = experimentList_->currentItem();
    if (!item) return;
    bool ok = false;
    const int id = item->data(Qt::UserRole).toInt(&ok);
    if (!ok || id <= 0) return;
    loadExperiment(id);
}

void ViewTab::loadExperiment(int id) {
    if (repo_ == nullptr) return;
    auto opt = repo_->loadExperiment(id);
    if (!opt.has_value()) return;
    const auto& exp = *opt;

    viewport_->clearScene();
    viewport_->setSceneBoundingBox(exp.bbox);

    // Re-triangulate each surface (meshes are not stored in the DB).
    currentMeshes_.clear();
    currentMeshes_.reserve(exp.surfaces.size());
    for (std::size_t i = 0; i < exp.surfaces.size(); ++i) {
        currentMeshes_.push_back(triangulateRecord(exp.surfaces[i], exp.bbox));
        viewport_->addMesh(currentMeshes_.back(), paletteColor(static_cast<int>(i)));
    }

    // Re-intersect every pair (i<j) per stored method.
    loadedPolylineCount_ = 0;
    for (const auto& isec : exp.intersections) {
        if (isec.surface1Index < 0 ||
            isec.surface1Index >= static_cast<int>(currentMeshes_.size()) ||
            isec.surface2Index < 0 ||
            isec.surface2Index >= static_cast<int>(currentMeshes_.size())) {
            continue;
        }
        const auto polys = intersectMeshes(
            currentMeshes_[isec.surface1Index], currentMeshes_[isec.surface2Index],
            isec.intersectionMethod);
        for (const auto& p : polys) {
            viewport_->addPolyline(p, QColor(255, 255, 255, 255));
        }
        loadedPolylineCount_ += static_cast<int>(polys.size());
    }
    loadedMeshCount_ = static_cast<int>(currentMeshes_.size());

    // Repopulate the BVH surface combo. Block its signal so the combo reset
    // doesn't trigger a stale onBvhSurfaceChanged before we pick a default.
    {
        QSignalBlocker blocker(bvhSurfaceCombo_);
        bvhSurfaceCombo_->clear();
        bvhSurfaceCombo_->addItem(tr("(off)"), -1);
        for (std::size_t i = 0; i < exp.surfaces.size(); ++i) {
            bvhSurfaceCombo_->addItem(
                QString("#%1 %2").arg(i).arg(exp.surfaces[i].type),
                static_cast<int>(i));
        }
        bvhSurfaceCombo_->setCurrentIndex(0);  // (off)
    }
    rebuildBvhForSelectedSurface();
}

void ViewTab::onBvhSurfaceChanged(int /*index*/) {
    rebuildBvhForSelectedSurface();
}

void ViewTab::onBvhDepthChanged(int value) {
    viewport_->setBvhDepthFilter(value);
}

void ViewTab::rebuildBvhForSelectedSurface() {
    const int surfaceIdx = bvhSurfaceCombo_->currentData().toInt();
    if (surfaceIdx < 0 || surfaceIdx >= static_cast<int>(currentMeshes_.size())) {
        viewport_->setBvhNodes({});
        bvhDepthSpin_->setEnabled(false);
        return;
    }
    auto nodes = qi::intersection::buildBvhForVisualization(currentMeshes_[surfaceIdx]);
    viewport_->setBvhNodes(nodes);
    const int maxDepth = viewport_->bvhMaxDepth();
    {
        QSignalBlocker blocker(bvhDepthSpin_);
        bvhDepthSpin_->setRange(-1, maxDepth);
        bvhDepthSpin_->setValue(-1);  // show all levels by default
    }
    bvhDepthSpin_->setEnabled(true);
    viewport_->setBvhDepthFilter(-1);
}

}  // namespace qi::ui
