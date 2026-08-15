#pragma once

#include <memory>
#include <string>
#include <SQLiteCpp/SQLiteCpp.h>

namespace timetracker::database {

class Database {
public:
    explicit Database(const std::string& dbPath);
    ~Database() = default;

    // Non-copyable
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Movable
    Database(Database&&) = default;
    Database& operator=(Database&&) = default;

    // Get raw database handle for repositories
    SQLite::Database& getHandle();

    // Initialize schema
    void initSchema();

    // Execute raw SQL (for testing/debugging)
    void execute(const std::string& sql);

private:
    std::unique_ptr<SQLite::Database> db_;

    void createTables();
    void migrateSchema();
    void createIndexes();

    // Check whether a column already exists on a table (for guarded ALTER TABLE)
    bool hasColumn(const std::string& table, const std::string& column);
};

} // namespace timetracker::database
