#pragma once

#include <QSqlDatabase>
#include <QString>

namespace qi::storage {

inline constexpr int kCurrentSchemaVersion = 1;

// Owns one named SQLite connection. The connection is closed and
// unregistered on destruction.
//
// Default file path is "./experiments.db" — when running the GUI from the
// build directory the database lives next to the binary as PLAN.md prescribes.
// Tests should pass an explicit path via TempDatabase.
class DatabaseManager {
public:
    explicit DatabaseManager(const QString& path,
                             QString connectionName = QString());
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool isOpen() const;
    QString lastError() const { return lastError_; }
    QString connectionName() const { return connectionName_; }
    QSqlDatabase database() const;

    // Creates the schema (experiments / surfaces / intersections /
    // schema_version) if missing. Idempotent. Enables PRAGMA foreign_keys = ON.
    bool initialize();

    // Reads schema_version.version. Returns 0 if the table does not exist
    // (i.e. database has not been initialized yet).
    int schemaVersion() const;

private:
    bool execNoBindings(const QString& sql);
    bool ensureForeignKeysOn();

    QString connectionName_;
    mutable QString lastError_;
};

}  // namespace qi::storage
