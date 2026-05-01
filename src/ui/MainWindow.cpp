#include "MainWindow.hpp"

#include <QMessageBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include "DatabaseManager.hpp"
#include "ExperimentRepository.hpp"
#include "ExperimentTab.hpp"
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
    if (repo_) {
        experimentTab_->setRepository(repo_.get());
    }

    auto* page = ui_->experimentTab;
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(experimentTab_);
    layout->setContentsMargins(0, 0, 0, 0);

    connect(experimentTab_, &ExperimentTab::experimentFinished, this,
            [this](int id) {
                if (id > 0) {
                    QMessageBox::information(
                        this, tr("Experiment saved"),
                        tr("Saved as id %1").arg(id));
                } else {
                    QMessageBox::information(this, tr("Experiment finished"),
                                             tr("Run completed (not persisted)"));
                }
            });
}

MainWindow::~MainWindow() = default;

}  // namespace qi::ui
