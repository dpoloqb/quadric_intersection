#pragma once

#include <QFutureWatcher>
#include <QString>
#include <QWidget>
#include <atomic>
#include <vector>

#include "ExperimentConfig.hpp"
#include "ExperimentResult.hpp"

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QPushButton;
class QWidget;
QT_END_NAMESPACE

namespace qi::storage {
class ExperimentRepository;
}

namespace qi::ui {

class BoundingBoxWidget;
class SurfaceEditorWidget;

// Top-level tab that lets a user configure an experiment, run it on a
// background thread, save the result to SQLite, and signal completion.
//
// Layout:
//   - top:    BoundingBoxWidget
//   - left:   QListWidget of surfaces + add/remove/duplicate buttons
//   - right:  SurfaceEditorWidget bound to current selection
//   - bottom: intersection-method combo, notes, Run button, progress bar
class ExperimentTab : public QWidget {
    Q_OBJECT
public:
    explicit ExperimentTab(QWidget* parent = nullptr);
    ~ExperimentTab() override;

    // Optional repository: if set, every successful run is persisted.
    void setRepository(qi::storage::ExperimentRepository* repo);

    // Build an ExperimentConfig from the current UI state (used by Run and
    // tests).
    qi::experiment::ExperimentConfig buildConfig() const;

    // Programmatically add / remove / duplicate surfaces — called by
    // button handlers and tests.
    void addSurface();
    void removeSelectedSurface();
    void duplicateSelectedSurface();
    int surfaceCount() const { return static_cast<int>(surfaces_.size()); }

    // Synchronous run — used by tests that don't want to wait on QtConcurrent.
    qi::experiment::ExperimentResult runSync();

    // Asynchronous run — wired to the Run button.
    void runAsync();

    // Request cancellation of the currently running async experiment.
    // Safe to call when nothing is running (no-op).
    void requestStop();

    // Replace surfaces and metadata in-place from an in-memory config.
    void applyConfig(const qi::experiment::ExperimentConfig& cfg);

    // Load a previously-saved experiment for editing. The form is populated
    // from `exp` (bbox, surfaces, intersection method, notes) and the tab
    // enters "edit mode": the next successful Run will replace experiment
    // `exp.id` in the database (delete + save) instead of appending a new row.
    void loadExperimentForEditing(const qi::experiment::ExperimentResult& exp);

    // Currently-edited experiment id (0 if not editing). Mainly for tests.
    int editingExperimentId() const { return editingExperimentId_; }

    // Drop edit mode without changing the form values. Next Run will save as
    // a new experiment instead of replacing the original.
    void clearEditingMode();

    // Save / load the current config as a JSON file. Returns true on success.
    bool saveConfigToFile(const QString& path) const;
    bool loadConfigFromFile(const QString& path);

signals:
    // Emitted on the UI thread after the runner finishes (and the row is
    // saved if a repository is set). `experimentId` is the DB id, or 0 if
    // no repository was attached.
    void experimentFinished(int experimentId);

    // Emitted on the UI thread when a run was stopped via Stop button.
    // No DB row is saved in that case. Distinct from `experimentFinished`
    // so listeners can suppress the "saved/finished" popup.
    void experimentCancelled();

    // Emitted when an in-place edit replaced an existing experiment.
    // `oldId` was deleted before the new row was inserted; the new id is
    // delivered separately via `experimentFinished`.
    void experimentReplaced(int oldId);

    // Internal cross-thread progress channel — emitted by the worker thread,
    // received on the UI thread to update the progress bar.
    void progressFromWorker(const QString& stage, int current, int total);

private slots:
    void onSelectionChanged();
    void onEditorConfigChanged();
    void onProgressUpdate(const QString& stage, int current, int total);
    void onRunFinished();

private:
    void refreshListLabel(int index);
    QString labelFor(int index) const;
    qi::experiment::SurfaceConfig defaultSurface() const;
    void updateEditingBadge();

    BoundingBoxWidget* bboxWidget_;
    QListWidget* surfaceList_;
    SurfaceEditorWidget* editor_;
    QComboBox* intersectionMethodCombo_;
    QLineEdit* notesEdit_;
    QPushButton* runButton_;
    QPushButton* stopButton_;
    QProgressBar* progressBar_;
    QWidget* editingBadge_;
    QLabel* editingBadgeLabel_;

    std::vector<qi::experiment::SurfaceConfig> surfaces_;
    qi::storage::ExperimentRepository* repo_ = nullptr;
    QFutureWatcher<qi::experiment::ExperimentResult>* watcher_;
    std::atomic<bool> cancelToken_{false};
    bool suppressEditorSignals_ = false;
    int editingExperimentId_ = 0;
};

}  // namespace qi::ui
