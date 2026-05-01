#include <gtest/gtest.h>

#include <QApplication>

// Tests that touch QSqlDatabase or any QWidget need a Qt application
// instance. QApplication is a QCoreApplication subclass, so it covers both.
// `offscreen` platform plugin allows widget tests to construct widgets
// without a running display server (needed in CI / headless envs).
int main(int argc, char** argv) {
    if (qgetenv("QT_QPA_PLATFORM").isEmpty()) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
