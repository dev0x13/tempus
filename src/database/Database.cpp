#include "Database.hpp"
#include <stdexcept>

namespace timetracker::database {

Database::Database(const std::string& dbPath)
    : db_(std::make_unique<SQLite::Database>(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)) {
    initSchema();
}

SQLite::Database& Database::getHandle() {
    return *db_;
}

void Database::initSchema() {
    createTables();
    migrateSchema();
    createIndexes();
}

void Database::execute(const std::string& sql) {
    db_->exec(sql);
}

void Database::createTables() {
    // Activities table
    db_->exec(R"(
        CREATE TABLE IF NOT EXISTS activities (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            search_name TEXT NOT NULL,
            deleted INTEGER NOT NULL DEFAULT 0
        )
    )");

    // Facts table
    db_->exec(R"(
        CREATE TABLE IF NOT EXISTS facts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            activity_id INTEGER NOT NULL,
            start_time INTEGER NOT NULL,
            end_time INTEGER,
            exported_to_youtrack INTEGER NOT NULL DEFAULT 0,
            FOREIGN KEY (activity_id) REFERENCES activities(id)
        )
    )");

    // Settings table
    db_->exec(R"(
        CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        )
    )");

    // YouTrack export log table
    db_->exec(R"(
        CREATE TABLE IF NOT EXISTS youtrack_export_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            activity_name TEXT NOT NULL,
            issue_id TEXT NOT NULL,
            tracked_date TEXT NOT NULL,
            duration_minutes INTEGER NOT NULL,
            exported_at INTEGER NOT NULL
        )
    )");
}

void Database::migrateSchema() {
    // Check if exported_to_youtrack column exists in facts table
    SQLite::Statement query(*db_, "PRAGMA table_info(facts)");
    bool hasExportedColumn = false;

    while (query.executeStep()) {
        std::string columnName = query.getColumn(1).getString();
        if (columnName == "exported_to_youtrack") {
            hasExportedColumn = true;
            break;
        }
    }

    // Add exported_to_youtrack column if it doesn't exist
    if (!hasExportedColumn) {
        db_->exec("ALTER TABLE facts ADD COLUMN exported_to_youtrack INTEGER NOT NULL DEFAULT 0");
    }
}

void Database::createIndexes() {
    // Index for activity search
    db_->exec(R"(
        CREATE INDEX IF NOT EXISTS idx_activities_search_name
        ON activities(search_name)
    )");

    // Index for fact lookups by activity
    db_->exec(R"(
        CREATE INDEX IF NOT EXISTS idx_facts_activity_id
        ON facts(activity_id)
    )");

    // Index for date range queries
    db_->exec(R"(
        CREATE INDEX IF NOT EXISTS idx_facts_start_time
        ON facts(start_time)
    )");

    // Index for finding ongoing facts
    db_->exec(R"(
        CREATE INDEX IF NOT EXISTS idx_facts_end_time
        ON facts(end_time)
    )");

    // Index for export status filtering
    db_->exec(R"(
        CREATE INDEX IF NOT EXISTS idx_facts_exported
        ON facts(exported_to_youtrack)
    )");

    // Index for export log chronological queries
    db_->exec(R"(
        CREATE INDEX IF NOT EXISTS idx_export_log_timestamp
        ON youtrack_export_log(exported_at DESC)
    )");
}

} // namespace timetracker::database
