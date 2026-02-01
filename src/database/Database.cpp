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
            search_name TEXT NOT NULL
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
    SQLite::Statement factsQuery(*db_, "PRAGMA table_info(facts)");
    bool hasExportedColumn = false;

    while (factsQuery.executeStep()) {
        std::string columnName = factsQuery.getColumn(1).getString();
        if (columnName == "exported_to_youtrack") {
            hasExportedColumn = true;
            break;
        }
    }

    // Add exported_to_youtrack column if it doesn't exist
    if (!hasExportedColumn) {
        db_->exec("ALTER TABLE facts ADD COLUMN exported_to_youtrack INTEGER NOT NULL DEFAULT 0");
    }

    // Check if description or deleted columns exist in activities table (legacy)
    // If either exists, we need to drop them
    SQLite::Statement activitiesQuery(*db_, "PRAGMA table_info(activities)");
    bool hasActivityDescriptionColumn = false;
    bool hasActivityDeletedColumn = false;

    while (activitiesQuery.executeStep()) {
        std::string columnName = activitiesQuery.getColumn(1).getString();
        if (columnName == "description") {
            hasActivityDescriptionColumn = true;
        } else if (columnName == "deleted") {
            hasActivityDeletedColumn = true;
        }
    }

    // Drop description and deleted columns from activities if they exist
    // SQLite doesn't support DROP COLUMN directly, so we need to recreate the table
    if (hasActivityDescriptionColumn || hasActivityDeletedColumn) {
        db_->exec(R"(
            BEGIN TRANSACTION;

            -- Create new activities table without description or deleted
            CREATE TABLE activities_new (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL UNIQUE,
                search_name TEXT NOT NULL
            );

            -- Copy data from old table (excluding description and deleted)
            INSERT INTO activities_new (id, name, search_name)
            SELECT id, name, search_name FROM activities;

            -- Drop old table
            DROP TABLE activities;

            -- Rename new table
            ALTER TABLE activities_new RENAME TO activities;

            COMMIT;
        )");
    }

    // Clean up orphaned activities (activities with no facts)
    db_->exec(R"(
        DELETE FROM activities
        WHERE id NOT IN (SELECT DISTINCT activity_id FROM facts)
    )");

    // Check if description column exists in facts table
    SQLite::Statement factsDescQuery(*db_, "PRAGMA table_info(facts)");
    bool factsHasDescriptionColumn = false;

    while (factsDescQuery.executeStep()) {
        std::string columnName = factsDescQuery.getColumn(1).getString();
        if (columnName == "description") {
            factsHasDescriptionColumn = true;
            break;
        }
    }

    // Add description column to facts if it doesn't exist
    if (!factsHasDescriptionColumn) {
        db_->exec("ALTER TABLE facts ADD COLUMN description TEXT NOT NULL DEFAULT ''");
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
