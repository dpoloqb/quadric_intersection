#include <QApplication>

#include "MainWindow.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    qi::ui::MainWindow window;
    window.show();
    return app.exec();
}
