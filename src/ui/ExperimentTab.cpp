#include "ExperimentTab.hpp"

#include <QComboBox>
#include <QFrame>
#include <QFuture>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent>

#include <QFileDialog>

#include "BoundingBoxWidget.hpp"
#include "ConfigJson.hpp"
#include "ExperimentRepository.hpp"
#include "ExperimentRunner.hpp"
#include "SurfaceEditorWidget.hpp"

namespace qi::ui {

using qi::experiment::ExperimentConfig;
using qi::experiment::ExperimentResult;
using qi::experiment::ExperimentRunner;
using qi::experiment::SurfaceConfig;
using qi::geometry::Quat;
using qi::geometry::Transform;
using qi::geometry::Vec3;
using qi::storage::ExperimentRepository;

ExperimentTab::ExperimentTab(QWidget* parent) : QWidget(parent) {
    bboxWidget_ = new BoundingBoxWidget;
    surfaceList_ = new QListWidget;
    editor_ = new SurfaceEditorWidget;

    auto* addBtn = new QPushButton(tr("+"));
    auto* removeBtn = new QPushButton(tr("−"));
    auto* duplicateBtn = new QPushButton(tr("Duplicate"));

    intersectionMethodCombo_ = new QComboBox;
    intersectionMethodCombo_->addItem(tr("BVH"), "bvh");
    intersectionMethodCombo_->addItem(tr("Naive"), "naive");

    notesEdit_ = new QLineEdit;
    notesEdit_->setPlaceholderText(tr("notes (optional)"));

    runButton_ = new QPushButton(tr("Run"));
    stopButton_ = new QPushButton(tr("Stop"));
    stopButton_->setEnabled(false);
    auto* loadBtn = new QPushButton(tr("Load…"));
    auto* saveBtn = new QPushButton(tr("Save…"));
    progressBar_ = new QProgressBar;
    progressBar_->setRange(0, 1);
    progressBar_->setValue(0);
    progressBar_->setTextVisible(true);

    // Edit-mode badge — visible only while editing an existing experiment.
    editingBadge_ = new QWidget;
    editingBadgeLabel_ = new QLabel;
    auto* editingClearBtn = new QPushButton(tr("×"));
    editingClearBtn->setFixedWidth(24);
    editingClearBtn->setToolTip(tr("Stop editing (next Run will save as new)"));
    auto* badgeLayout = new QHBoxLayout(editingBadge_);
    badgeLayout->setContentsMargins(6, 2, 4, 2);
    badgeLayout->addWidget(editingBadgeLabel_);
    badgeLayout->addWidget(editingClearBtn);
    editingBadge_->setVisible(false);
    editingBadge_->setStyleSheet(
        "QWidget { background: rgba(245, 166, 35, 30); "
        "border: 1px solid rgba(245, 166, 35, 120); border-radius: 4px; }");

    auto* listColumn = new QWidget;
    auto* listLayout = new QVBoxLayout(listColumn);
    listLayout->addWidget(surfaceList_, /*stretch=*/1);
    auto* listButtons = new QHBoxLayout;
    listButtons->addWidget(addBtn);
    listButtons->addWidget(removeBtn);
    listButtons->addWidget(duplicateBtn);
    listLayout->addLayout(listButtons);

    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(listColumn);
    splitter->addWidget(editor_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    auto* bottom = new QWidget;
    auto* bottomLayout = new QHBoxLayout(bottom);
    bottomLayout->addWidget(new QLabel(tr("Method:")));
    bottomLayout->addWidget(intersectionMethodCombo_);
    bottomLayout->addWidget(notesEdit_, /*stretch=*/1);
    bottomLayout->addWidget(loadBtn);
    bottomLayout->addWidget(saveBtn);
    bottomLayout->addWidget(editingBadge_);
    bottomLayout->addWidget(runButton_);
    bottomLayout->addWidget(stopButton_);

    auto* main = new QVBoxLayout(this);
    auto* bboxBox = new QGroupBox(tr("Bounding box"));
    auto* bboxLayout = new QVBoxLayout(bboxBox);
    bboxLayout->addWidget(bboxWidget_);
    main->addWidget(bboxBox);
    main->addWidget(splitter, /*stretch=*/1);
    main->addWidget(bottom);
    main->addWidget(progressBar_);

    watcher_ = new QFutureWatcher<ExperimentResult>(this);
    connect(watcher_, &QFutureWatcher<ExperimentResult>::finished, this,
            &ExperimentTab::onRunFinished);

    connect(addBtn, &QPushButton::clicked, this, &ExperimentTab::addSurface);
    connect(removeBtn, &QPushButton::clicked, this, &ExperimentTab::removeSelectedSurface);
    connect(duplicateBtn, &QPushButton::clicked, this, &ExperimentTab::duplicateSelectedSurface);
    connect(runButton_, &QPushButton::clicked, this, &ExperimentTab::runAsync);
    connect(stopButton_, &QPushButton::clicked, this, &ExperimentTab::requestStop);
    connect(editingClearBtn, &QPushButton::clicked, this, &ExperimentTab::clearEditingMode);
    connect(loadBtn, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Load experiment config"), QString(),
            tr("JSON files (*.json)"));
        if (path.isEmpty()) return;
        if (!loadConfigFromFile(path)) {
            QMessageBox::warning(this, tr("Load failed"),
                                 tr("Could not parse %1").arg(path));
        }
    });
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getSaveFileName(
            this, tr("Save experiment config"), QString(),
            tr("JSON files (*.json)"));
        if (path.isEmpty()) return;
        if (!saveConfigToFile(path)) {
            QMessageBox::warning(this, tr("Save failed"),
                                 tr("Could not write %1").arg(path));
        }
    });

    connect(surfaceList_, &QListWidget::currentRowChanged, this,
            [this](int) { onSelectionChanged(); });

    connect(editor_, &SurfaceEditorWidget::configChanged, this,
            &ExperimentTab::onEditorConfigChanged);

    // progressFromWorker is emitted from the worker thread; default
    // AutoConnection becomes QueuedConnection across threads.
    connect(this, &ExperimentTab::progressFromWorker, this,
            &ExperimentTab::onProgressUpdate);

    // Seed with one default surface so the editor is non-empty.
    addSurface();
}

ExperimentTab::~ExperimentTab() {
    if (watcher_->isRunning()) {
        cancelToken_.store(true, std::memory_order_relaxed);
        watcher_->waitForFinished();
    }
}

void ExperimentTab::setRepository(ExperimentRepository* repo) { repo_ = repo; }

SurfaceConfig ExperimentTab::defaultSurface() const {
    SurfaceConfig sc;
    sc.type = "ellipsoid";
    sc.params = {1.0, 1.0, 1.0, 1.0};
    sc.transform = Transform::identity();
    sc.triangulationMethod = "parametric";
    sc.uSteps = 30;
    sc.vSteps = 30;
    return sc;
}

void ExperimentTab::addSurface() {
    surfaces_.push_back(defaultSurface());
    surfaceList_->addItem(labelFor(static_cast<int>(surfaces_.size()) - 1));
    surfaceList_->setCurrentRow(static_cast<int>(surfaces_.size()) - 1);
}

void ExperimentTab::removeSelectedSurface() {
    const int row = surfaceList_->currentRow();
    if (row < 0 || row >= static_cast<int>(surfaces_.size())) return;
    surfaces_.erase(surfaces_.begin() + row);
    delete surfaceList_->takeItem(row);
    // Refresh remaining labels (their indices shifted).
    for (int i = row; i < static_cast<int>(surfaces_.size()); ++i) {
        refreshListLabel(i);
    }
}

void ExperimentTab::duplicateSelectedSurface() {
    const int row = surfaceList_->currentRow();
    if (row < 0 || row >= static_cast<int>(surfaces_.size())) return;
    surfaces_.push_back(surfaces_[row]);
    surfaceList_->addItem(labelFor(static_cast<int>(surfaces_.size()) - 1));
    surfaceList_->setCurrentRow(static_cast<int>(surfaces_.size()) - 1);
}

void ExperimentTab::onSelectionChanged() {
    const int row = surfaceList_->currentRow();
    if (row < 0 || row >= static_cast<int>(surfaces_.size())) {
        editor_->setEnabled(false);
        return;
    }
    editor_->setEnabled(true);
    suppressEditorSignals_ = true;
    editor_->setConfig(surfaces_[row]);
    suppressEditorSignals_ = false;
}

void ExperimentTab::onEditorConfigChanged() {
    if (suppressEditorSignals_) return;
    const int row = surfaceList_->currentRow();
    if (row < 0 || row >= static_cast<int>(surfaces_.size())) return;
    surfaces_[row] = editor_->config();
    refreshListLabel(row);
}

QString ExperimentTab::labelFor(int index) const {
    if (index < 0 || index >= static_cast<int>(surfaces_.size())) return {};
    return QString("%1: %2").arg(index).arg(surfaces_[index].type);
}

void ExperimentTab::refreshListLabel(int index) {
    if (index < 0 || index >= surfaceList_->count()) return;
    surfaceList_->item(index)->setText(labelFor(index));
}

ExperimentConfig ExperimentTab::buildConfig() const {
    ExperimentConfig cfg;
    cfg.bbox = bboxWidget_->boundingBox();
    cfg.surfaces = surfaces_;
    cfg.intersectionMethod = intersectionMethodCombo_->currentData().toString();
    cfg.notes = notesEdit_->text();
    return cfg;
}

ExperimentResult ExperimentTab::runSync() {
    ExperimentRunner runner;
    return runner.run(buildConfig(), [this](const QString& stage, int cur, int tot) {
        emit progressFromWorker(stage, cur, tot);
    });
}

void ExperimentTab::runAsync() {
    if (watcher_->isRunning()) return;
    cancelToken_.store(false, std::memory_order_relaxed);
    runButton_->setEnabled(false);
    stopButton_->setEnabled(true);
    progressBar_->setRange(0, 1);
    progressBar_->setValue(0);

    const ExperimentConfig cfg = buildConfig();
    auto future = QtConcurrent::run([this, cfg]() {
        ExperimentRunner runner;
        return runner.run(
            cfg,
            [this](const QString& stage, int cur, int tot) {
                emit progressFromWorker(stage, cur, tot);
            },
            &cancelToken_);
    });
    watcher_->setFuture(future);
}

void ExperimentTab::requestStop() {
    if (!watcher_->isRunning()) return;
    cancelToken_.store(true, std::memory_order_relaxed);
    stopButton_->setEnabled(false);
    progressBar_->setFormat(tr("stopping…"));
}

void ExperimentTab::onProgressUpdate(const QString& stage, int current, int total) {
    if (progressBar_->maximum() != total) {
        progressBar_->setRange(0, std::max(1, total));
    }
    progressBar_->setValue(current);
    progressBar_->setFormat(QString("%1 (%p%)").arg(stage));
}

void ExperimentTab::onRunFinished() {
    runButton_->setEnabled(true);
    stopButton_->setEnabled(false);
    auto result = watcher_->result();
    if (result.cancelled) {
        progressBar_->setRange(0, 1);
        progressBar_->setValue(0);
        progressBar_->setFormat(tr("stopped"));
        emit experimentCancelled();
        return;
    }
    int replacedId = 0;
    if (editingExperimentId_ > 0 && repo_ != nullptr) {
        if (repo_->deleteExperiment(editingExperimentId_)) {
            replacedId = editingExperimentId_;
        }
        editingExperimentId_ = 0;
        updateEditingBadge();
    }
    int savedId = 0;
    if (repo_ != nullptr) {
        savedId = repo_->saveExperiment(result);
    }
    if (replacedId > 0) emit experimentReplaced(replacedId);
    emit experimentFinished(savedId);
}

void ExperimentTab::loadExperimentForEditing(
    const qi::experiment::ExperimentResult& exp) {
    ExperimentConfig cfg;
    cfg.bbox = exp.bbox;
    cfg.notes = exp.notes;
    cfg.intersectionMethod = exp.intersections.empty()
                                 ? QStringLiteral("bvh")
                                 : exp.intersections.front().intersectionMethod;
    cfg.surfaces.reserve(exp.surfaces.size());
    for (const auto& sr : exp.surfaces) {
        SurfaceConfig sc;
        sc.type = sr.type;
        sc.params = sr.params;
        sc.transform = sr.transform;
        sc.triangulationMethod = sr.triangulationMethod;
        sc.mcResolution = sr.mcResolution;
        sc.uSteps = sr.uSteps;
        sc.vSteps = sr.vSteps;
        cfg.surfaces.push_back(std::move(sc));
    }
    applyConfig(cfg);
    editingExperimentId_ = exp.id;
    updateEditingBadge();
}

void ExperimentTab::clearEditingMode() {
    editingExperimentId_ = 0;
    updateEditingBadge();
}

void ExperimentTab::updateEditingBadge() {
    if (editingExperimentId_ > 0) {
        editingBadgeLabel_->setText(tr("editing #%1").arg(editingExperimentId_));
        editingBadge_->setVisible(true);
    } else {
        editingBadge_->setVisible(false);
    }
}

void ExperimentTab::applyConfig(const ExperimentConfig& cfg) {
    // Loading a foreign config (JSON file or programmatic) drops edit-mode:
    // the next Run will save as a new experiment. Callers that want to keep
    // edit-mode (e.g. loadExperimentForEditing) reset editingExperimentId_
    // afterwards.
    editingExperimentId_ = 0;

    bboxWidget_->setBoundingBox(cfg.bbox);

    const int idx = intersectionMethodCombo_->findData(cfg.intersectionMethod);
    if (idx >= 0) intersectionMethodCombo_->setCurrentIndex(idx);
    notesEdit_->setText(cfg.notes);

    surfaces_.clear();
    surfaceList_->clear();
    if (cfg.surfaces.empty()) {
        addSurface();  // keep at least one
    } else {
        surfaces_ = cfg.surfaces;
        for (std::size_t i = 0; i < surfaces_.size(); ++i) {
            surfaceList_->addItem(labelFor(static_cast<int>(i)));
        }
        surfaceList_->setCurrentRow(0);
    }
    updateEditingBadge();
}

bool ExperimentTab::saveConfigToFile(const QString& path) const {
    return qi::experiment::saveConfigToFile(buildConfig(), path);
}

bool ExperimentTab::loadConfigFromFile(const QString& path) {
    auto opt = qi::experiment::loadConfigFromFile(path);
    if (!opt.has_value()) return false;
    applyConfig(*opt);
    return true;
}

}  // namespace qi::ui
