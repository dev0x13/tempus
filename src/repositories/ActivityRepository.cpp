#include "ActivityRepository.hpp"
#include "utils/Utf8.hpp"
#include <algorithm>
#include <string>

namespace timetracker::repositories {

ActivityRepository::ActivityRepository(database::Database& db) : db_(db) {}

std::string ActivityRepository::toLower(const std::string& str) {
    // Activity names are user text in any language, so this needs real Unicode case folding
    // rather than std::tolower.
    return utils::Utf8::toLower(str);
}

models::Activity ActivityRepository::mapRow(SQLite::Statement& query) {
    return models::Activity{
        query.getColumn(0).getInt64(),       // id
        query.getColumn(1).getString(),      // name
        query.getColumn(2).getString()       // search_name
    };
}

models::Activity ActivityRepository::create(const std::string& name) {
    std::string searchName = toLower(name);

    SQLite::Statement insert(db_.getHandle(),
        "INSERT INTO activities (name, search_name) VALUES (?, ?)");
    insert.bind(1, name);
    insert.bind(2, searchName);
    insert.exec();

    int64_t id = db_.getHandle().getLastInsertRowid();
    return models::Activity{id, name, searchName};
}

std::optional<models::Activity> ActivityRepository::findById(int64_t id) {
    SQLite::Statement query(db_.getHandle(),
        "SELECT id, name, search_name FROM activities WHERE id = ?");
    query.bind(1, id);

    if (query.executeStep()) {
        return mapRow(query);
    }
    return std::nullopt;
}

std::optional<models::Activity> ActivityRepository::findByName(const std::string& name) {
    std::string searchName = toLower(name);

    SQLite::Statement query(db_.getHandle(),
        "SELECT id, name, search_name FROM activities WHERE search_name = ?");
    query.bind(1, searchName);

    if (query.executeStep()) {
        return mapRow(query);
    }
    return std::nullopt;
}

std::vector<models::Activity> ActivityRepository::findAll() {
    std::vector<models::Activity> activities;

    SQLite::Statement query(db_.getHandle(),
        "SELECT id, name, search_name FROM activities ORDER BY name");

    while (query.executeStep()) {
        activities.push_back(mapRow(query));
    }
    return activities;
}

void ActivityRepository::update(const models::Activity& activity) {
    std::string searchName = toLower(activity.name);

    SQLite::Statement update(db_.getHandle(),
        "UPDATE activities SET name = ?, search_name = ? WHERE id = ?");
    update.bind(1, activity.name);
    update.bind(2, searchName);
    update.bind(3, activity.id);
    update.exec();
}

models::Activity ActivityRepository::getOrCreate(const std::string& name) {
    auto existing = findByName(name);
    if (existing.has_value()) {
        return *existing;
    }
    return create(name);
}

void ActivityRepository::deleteIfOrphaned(int64_t activityId) {
    // Count remaining facts for this activity
    SQLite::Statement count(db_.getHandle(),
        "SELECT COUNT(*) FROM facts WHERE activity_id = ?");
    count.bind(1, activityId);

    if (count.executeStep()) {
        int factCount = count.getColumn(0).getInt();
        if (factCount == 0) {
            // No facts remain, delete the activity
            SQLite::Statement del(db_.getHandle(),
                "DELETE FROM activities WHERE id = ?");
            del.bind(1, activityId);
            del.exec();
        }
    }
}

std::vector<models::Activity> ActivityRepository::search(const std::string& query, int limit) {
    std::vector<models::Activity> activities;
    std::string searchQuery = toLower(query) + "%";

    SQLite::Statement stmt(db_.getHandle(),
        "SELECT id, name, search_name FROM activities "
        "WHERE search_name LIKE ? "
        "ORDER BY name LIMIT ?");
    stmt.bind(1, searchQuery);
    stmt.bind(2, limit);

    while (stmt.executeStep()) {
        activities.push_back(mapRow(stmt));
    }
    return activities;
}

} // namespace timetracker::repositories
