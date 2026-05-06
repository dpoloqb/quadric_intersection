#include "MainWindow.hpp"

#include <QMessageBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include "DatabaseManager.hpp"
#include "ExperimentRepository.hpp"
#include "ExperimentTab.hpp"
#include "ResultsTab.hpp"
#include "ViewTab.hpp"
#include "ui_MainWindow.h"

namespace qi::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui_(std::make_unique<Ui::MainWindow>()) {
    ui_->setupUi(this);

    db_ = std::make_unique<qi::storage::DatabaseManager>(QStringLiteral("./experiments.db"));
    if (db_->isOpen()) {
        db_->initialize();
        repo_ = std::make_unique<qi::storage::ExperimentRepository>(*db_);
    }

    experimentTab_ = new ExperimentTab(this);
    resultsTab_ = new ResultsTab(this);
    viewTab_ = new ViewTab(this);
    if (repo_) {
        experimentTab_->setRepository(repo_.get());
        resultsTab_->setRepository(repo_.get());
        resultsTab_->setDatabase(db_->database());
        viewTab_->setRepository(repo_.get());
    }

    auto installInto = [](QWidget* page, QWidget* w) {
        auto* layout = new QVBoxLayout(page);
        layout->addWidget(w);
        layout->setContentsMargins(0, 0, 0, 0);
    };
    installInto(ui_->experimentTab, experimentTab_);
    installInto(ui_->resultsTab, resultsTab_);
    installInto(ui_->viewTab, viewTab_);

    connect(experimentTab_, &ExperimentTab::experimentFinished, this,
            [this](int id) {
                if (id > 0) {
                    resultsTab_->refresh();
                    viewTab_->refreshExperimentList();
                    QMessageBox::information(
                        this, tr("Experiment saved"),
                        tr("Saved as id %1").arg(id));
                } else {
                    QMessageBox::information(this, tr("Experiment finished"),
                                             tr("Run completed (not persisted)"));
                }
            });
    connect(experimentTab_, &ExperimentTab::experimentCancelled, this,
            [this]() {
                QMessageBox::information(this, tr("Experiment stopped"),
                                         tr("The experiment was stopped before completion. Nothing was saved."));
            });
    connect(experimentTab_, &ExperimentTab::experimentReplaced, this,
            [this](int /*oldId*/) {
                resultsTab_->refresh();
                viewTab_->refreshExperimentList();
            });
    connect(resultsTab_, &ResultsTab::experimentDeleted, this,
            [this](int id) {
                if (experimentTab_->editingExperimentId() == id) {
                    experimentTab_->clearEditingMode();
                }
                viewTab_->refreshExperimentList();
            });
    connect(resultsTab_, &ResultsTab::experimentEditRequested, this,
            [this](int id) {
                if (!repo_) return;
                auto exp = repo_->loadExperiment(id);
                if (!exp.has_value()) {
                    QMessageBox::warning(this, tr("Open for editing"),
                                         tr("Could not load experiment %1").arg(id));
                    return;
                }
                experimentTab_->loadExperimentForEditing(*exp);
                ui_->tabWidget->setCurrentWidget(ui_->experimentTab);
            });
}

MainWindow::~MainWindow() = default;

}  // namespace qi::ui
