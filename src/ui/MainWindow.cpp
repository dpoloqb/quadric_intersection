#include "MainWindow.hpp"

#include "ui_MainWindow.h"

namespace qi::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui_(std::make_unique<Ui::MainWindow>()) {
    ui_->setupUi(this);
}

MainWindow::~MainWindow() = default;

}  // namespace qi::ui
