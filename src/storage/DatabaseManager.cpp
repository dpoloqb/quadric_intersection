#include "DatabaseManager.hpp"

#include <QSqlError>
#include <QSqlQuery>
#include <atomic>

namespace qi::storage {

namespace {

std::atomic<int> g_dbCounter{0};

const char* const kCreateExperiments = R"sql(
    CREATE TABLE IF NOT EXISTS experiments (
        id INTEGER PRIMARY KEY,
        created_at TEXT NOT NULL,
        bbox_min_x REAL, bbox_min_y REAL, bbox_min_z REAL,
        bbox_max_x REAL, bbox_max_y REAL, bbox_max_z REAL,
        surfaces_count INTEGER,
        notes TEXT
    );
)sql";

const char* const kCreateSurfaces = R"sql(
    CREATE TABLE IF NOT EXISTS surfaces (
        id INTEGER PRIMARY KEY,
        experiment_id INTEGER NOT NULL,
        index_in_experiment INTEGER NOT NULL,
        type TEXT NOT NULL,
        params_json TEXT NOT NULL,
        transform_json TEXT NOT NULL,
        triangulation_method TEXT NOT NULL,
        triangulation_params_json TEXT NOT NULL,
        triangles_count INTEGER,
        time_triangulation_ms REAL,
        FOREIGN KEY (experiment_id) REFERENCES experiments(id) ON DELETE CASCADE
    );
)sql";

const char* const kCreateIntersections = R"sql(
    CREATE TABLE IF NOT EXISTS intersections (
        id INTEGER PRIMARY KEY,
        experiment_id INTEGER NOT NULL,
        surface1_id INTEGER NOT NULL,
        surface2_id INTEGER NOT NULL,
        intersection_method TEXT NOT NULL,
        time_intersection_ms REAL,
        segments_count INTEGER,
        polylines_count INTEGER,
        FOREIGN KEY (experiment_id) REFERENCES experiments(id) ON DELETE CASCADE,
        FOREIGN KEY (surface1_id) REFERENCES surfaces(id) ON DELETE CASCADE,
        FOREIGN KEY (surface2_id) REFERENCES surfaces(id) ON DELETE CASCADE
    );
)sql";

const char* const kCreateSchemaVersion = R"sql(
    CREATE TABLE IF NOT EXISTS schema_version (
        version INTEGER PRIMARY KEY
    );
)sql";

}  // namespace

DatabaseManager::DatabaseManager(const QString& path, QString connectionName) {
    if (connectionName.isEmpty()) {
        connectionName = QString("qi_db_%1").arg(g_dbCounter.fetch_add(1));
    }
    connectionName_ = connectionName;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName_);
    db.setDatabaseName(path);
    if (!db.open()) {
        lastError_ = db.lastError().text();
        return;
    }
    ensureForeignKeysOn();
}

DatabaseManager::~DatabaseManager() {
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName_, false);
        if (db.isValid() && db.isOpen()) {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connectionName_);
}

bool DatabaseManager::isOpen() const {
    return database().isOpen();
}

QSqlDatabase DatabaseManager::database() const {
    return QSqlDatabase::database(connectionName_, false);
}

bool DatabaseManager::initialize() {
    if (!isOpen()) {
        lastError_ = "database not open";
        return false;
    }

    QSqlDatabase db = database();
    if (!db.transaction()) {
        lastError_ = db.lastError().text();
        return false;
    }

    if (!execNoBindings(kCreateExperiments) ||
        !execNoBindings(kCreateSurfaces) ||
        !execNoBindings(kCreateIntersections) ||
        !execNoBindings(kCreateSchemaVersion)) {
        db.rollback();
        return false;
    }

    QSqlQuery q(db);
    q.prepare("INSERT OR IGNORE INTO schema_version (version) VALUES (?)");
    q.addBindValue(kCurrentSchemaVersion);
    if (!q.exec()) {
        lastError_ = q.lastError().text();
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        lastError_ = db.lastError().text();
        db.rollback();
        return false;
    }
    return true;
}

int DatabaseManager::schemaVersion() const {
    QSqlQuery q(database());
    if (!q.exec("SELECT version FROM schema_version ORDER BY version DESC LIMIT 1")) {
        // Table likely missing — that's an expected state before initialize().
        return 0;
    }
    if (q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

bool DatabaseManager::execNoBindings(const QString& sql) {
    QSqlQuery q(database());
    if (!q.exec(sql)) {
        lastError_ = q.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::ensureForeignKeysOn() {
    QSqlQuery q(database());
    if (!q.exec("PRAGMA foreign_keys = ON")) {
        lastError_ = q.lastError().text();
        return false;
    }
    return true;
}

}  // namespace qi::storage
