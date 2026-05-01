#pragma once

#include <QFutureWatcher>
#include <QString>
#include <QWidget>
#include <vector>

#include "ExperimentConfig.hpp"
#include "ExperimentResult.hpp"

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QPushButton;
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

    // Replace surfaces and metadata in-place from an in-memory config.
    void applyConfig(const qi::experiment::ExperimentConfig& cfg);

    // Save / load the current config as a JSON file. Returns true on success.
    bool saveConfigToFile(const QString& path) const;
    bool loadConfigFromFile(const QString& path);

signals:
    // Emitted on the UI thread after the runner finishes (and the row is
    // saved if a repository is set). `experimentId` is the DB id, or 0 if
    // no repository was attached.
    void experimentFinished(int experimentId);

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

    BoundingBoxWidget* bboxWidget_;
    QListWidget* surfaceList_;
    SurfaceEditorWidget* editor_;
    QComboBox* intersectionMethodCombo_;
    QLineEdit* notesEdit_;
    QPushButton* runButton_;
    QProgressBar* progressBar_;

    std::vector<qi::experiment::SurfaceConfig> surfaces_;
    qi::storage::ExperimentRepository* repo_ = nullptr;
    QFutureWatcher<qi::experiment::ExperimentResult>* watcher_;
    bool suppressEditorSignals_ = false;
};

}  // namespace qi::ui
