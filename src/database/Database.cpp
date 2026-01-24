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
            FOREIGN KEY (activity_id) REFERENCES activities(id)
        )
    )");
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
}

} // namespace timetracker::database
