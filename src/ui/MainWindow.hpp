#pragma once

#include <QMainWindow>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace qi::storage {
class DatabaseManager;
class ExperimentRepository;
}

namespace qi::ui {

class ExperimentTab;
class ResultsTab;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    std::unique_ptr<Ui::MainWindow> ui_;
    std::unique_ptr<qi::storage::DatabaseManager> db_;
    std::unique_ptr<qi::storage::ExperimentRepository> repo_;
    ExperimentTab* experimentTab_ = nullptr;
    ResultsTab* resultsTab_ = nullptr;
};

}  // namespace qi::ui
