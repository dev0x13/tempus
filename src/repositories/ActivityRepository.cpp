#include "ActivityRepository.hpp"
#include <unicode/unistr.h>
#include <unicode/locid.h>
#include <algorithm>
#include <string>

namespace timetracker::repositories {

ActivityRepository::ActivityRepository(database::Database& db) : db_(db) {}

std::string ActivityRepository::toLower(const std::string& str) {
    // Use ICU for proper UTF-8 case folding
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(str);
    ustr.toLower();
    std::string result;
    ustr.toUTF8String(result);
    return result;
}

models::Activity ActivityRepository::mapRow(SQLite::Statement& query) {
    return models::Activity{
        query.getColumn(0).getInt64(),       // id
        query.getColumn(1).getString(),      // name
        query.getColumn(2).getString(),      // search_name
        query.getColumn(3).getString(),      // description
        query.getColumn(4).getInt() != 0     // deleted
    };
}

models::Activity ActivityRepository::create(const std::string& name) {
    std::string searchName = toLower(name);

    SQLite::Statement insert(db_.getHandle(),
        "INSERT INTO activities (name, search_name, description) VALUES (?, ?, ?)");
    insert.bind(1, name);
    insert.bind(2, searchName);
    insert.bind(3, "");  // Empty description by default
    insert.exec();

    int64_t id = db_.getHandle().getLastInsertRowid();
    return models::Activity{id, name, searchName, "", false};
}

std::optional<models::Activity> ActivityRepository::findById(int64_t id) {
    SQLite::Statement query(db_.getHandle(),
        "SELECT id, name, search_name, description, deleted FROM activities WHERE id = ?");
    query.bind(1, id);

    if (query.executeStep()) {
        return mapRow(query);
    }
    return std::nullopt;
}

std::optional<models::Activity> ActivityRepository::findByName(const std::string& name) {
    std::string searchName = toLower(name);

    SQLite::Statement query(db_.getHandle(),
        "SELECT id, name, search_name, description, deleted FROM activities WHERE search_name = ?");
    query.bind(1, searchName);

    if (query.executeStep()) {
        return mapRow(query);
    }
    return std::nullopt;
}

std::vector<models::Activity> ActivityRepository::findAll(bool includeDeleted) {
    std::vector<models::Activity> activities;

    std::string sql = "SELECT id, name, search_name, description, deleted FROM activities";
    if (!includeDeleted) {
        sql += " WHERE deleted = 0";
    }
    sql += " ORDER BY name";

    SQLite::Statement query(db_.getHandle(), sql);
    while (query.executeStep()) {
        activities.push_back(mapRow(query));
    }
    return activities;
}

void ActivityRepository::update(const models::Activity& activity) {
    std::string searchName = toLower(activity.name);

    SQLite::Statement update(db_.getHandle(),
        "UPDATE activities SET name = ?, search_name = ?, description = ?, deleted = ? WHERE id = ?");
    update.bind(1, activity.name);
    update.bind(2, searchName);
    update.bind(3, activity.description);
    update.bind(4, activity.deleted ? 1 : 0);
    update.bind(5, activity.id);
    update.exec();
}

void ActivityRepository::softDelete(int64_t id) {
    SQLite::Statement update(db_.getHandle(),
        "UPDATE activities SET deleted = 1 WHERE id = ?");
    update.bind(1, id);
    update.exec();
}

void ActivityRepository::restore(int64_t id) {
    SQLite::Statement update(db_.getHandle(),
        "UPDATE activities SET deleted = 0 WHERE id = ?");
    update.bind(1, id);
    update.exec();
}

models::Activity ActivityRepository::getOrCreate(const std::string& name) {
    auto existing = findByName(name);
    if (existing.has_value()) {
        // If it was deleted, restore it
        if (existing->deleted) {
            restore(existing->id);
            existing->deleted = false;
        }
        return *existing;
    }
    return create(name);
}

std::vector<models::Activity> ActivityRepository::search(const std::string& query, int limit) {
    std::vector<models::Activity> activities;
    std::string searchQuery = toLower(query) + "%";

    SQLite::Statement stmt(db_.getHandle(),
        "SELECT id, name, search_name, description, deleted FROM activities "
        "WHERE search_name LIKE ? AND deleted = 0 "
        "ORDER BY name LIMIT ?");
    stmt.bind(1, searchQuery);
    stmt.bind(2, limit);

    while (stmt.executeStep()) {
        activities.push_back(mapRow(stmt));
    }
    return activities;
}

} // namespace timetracker::repositories
