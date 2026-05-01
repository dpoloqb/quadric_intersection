#include <gtest/gtest.h>

#include <QCoreApplication>

// Tests that touch QSqlDatabase need a Qt application instance for plugin
// loading. We use QCoreApplication (no GUI) which is enough for QtSql.
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
