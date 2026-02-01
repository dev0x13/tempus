#include "FactRepository.hpp"

namespace timetracker::repositories {

FactRepository::FactRepository(database::Database& db) : db_(db) {}

models::Fact FactRepository::mapRow(SQLite::Statement& query) {
    models::Fact fact;
    fact.id = query.getColumn(0).getInt64();
    fact.activityId = query.getColumn(1).getInt64();
    fact.startTime = query.getColumn(2).getInt64();
    if (!query.getColumn(3).isNull()) {
        fact.endTime = query.getColumn(3).getInt64();
    }
    // Column 4 is exported_to_youtrack (0 or 1)
    if (query.getColumnCount() > 4) {
        fact.exportedToYoutrack = (query.getColumn(4).getInt() != 0);
    }
    // Column 5 is description
    if (query.getColumnCount() > 5) {
        fact.description = query.getColumn(5).getString();
    }
    return fact;
}

models::Fact FactRepository::mapRowWithActivity(SQLite::Statement& query) {
    models::Fact fact;
    fact.id = query.getColumn(0).getInt64();
    fact.activityId = query.getColumn(1).getInt64();
    fact.startTime = query.getColumn(2).getInt64();
    if (!query.getColumn(3).isNull()) {
        fact.endTime = query.getColumn(3).getInt64();
    }
    fact.exportedToYoutrack = (query.getColumn(4).getInt() != 0);
    // Column 5 is fact description
    if (query.getColumnCount() > 5) {
        fact.description = query.getColumn(5).getString();
    }
    // Column 6 is activity name
    if (query.getColumnCount() > 6) {
        fact.activityName = query.getColumn(6).getString();
    }
    return fact;
}

models::Fact FactRepository::create(int64_t activityId, int64_t startTime, std::optional<int64_t> endTime, const std::string& description) {
    SQLite::Statement insert(db_.getHandle(),
        "INSERT INTO facts (activity_id, start_time, end_time, description) VALUES (?, ?, ?, ?)");
    insert.bind(1, activityId);
    insert.bind(2, startTime);
    if (endTime.has_value()) {
        insert.bind(3, *endTime);
    } else {
        insert.bind(3);  // Bind NULL
    }
    insert.bind(4, description);
    insert.exec();

    int64_t id = db_.getHandle().getLastInsertRowid();
    models::Fact fact{id, activityId, startTime, endTime};
    fact.description = description;
    return fact;
}

std::optional<models::Fact> FactRepository::findById(int64_t id) {
    SQLite::Statement query(db_.getHandle(),
        "SELECT f.id, f.activity_id, f.start_time, f.end_time, f.exported_to_youtrack, f.description, a.name "
        "FROM facts f "
        "JOIN activities a ON f.activity_id = a.id "
        "WHERE f.id = ?");
    query.bind(1, id);

    if (query.executeStep()) {
        return mapRowWithActivity(query);
    }
    return std::nullopt;
}

std::vector<models::Fact> FactRepository::findAll() {
    std::vector<models::Fact> facts;

    SQLite::Statement query(db_.getHandle(),
        "SELECT f.id, f.activity_id, f.start_time, f.end_time, f.exported_to_youtrack, f.description, a.name "
        "FROM facts f "
        "JOIN activities a ON f.activity_id = a.id "
        "ORDER BY f.start_time DESC");

    while (query.executeStep()) {
        facts.push_back(mapRowWithActivity(query));
    }
    return facts;
}

void FactRepository::update(const models::Fact& fact) {
    SQLite::Statement update(db_.getHandle(),
        "UPDATE facts SET activity_id = ?, start_time = ?, end_time = ?, description = ? WHERE id = ?");
    update.bind(1, fact.activityId);
    update.bind(2, fact.startTime);
    if (fact.endTime.has_value()) {
        update.bind(3, *fact.endTime);
    } else {
        update.bind(3);  // Bind NULL
    }
    update.bind(4, fact.description);
    update.bind(5, fact.id);
    update.exec();
}

void FactRepository::remove(int64_t id) {
    SQLite::Statement del(db_.getHandle(), "DELETE FROM facts WHERE id = ?");
    del.bind(1, id);
    del.exec();
}

std::optional<models::Fact> FactRepository::findOngoing() {
    SQLite::Statement query(db_.getHandle(),
        "SELECT f.id, f.activity_id, f.start_time, f.end_time, f.exported_to_youtrack, f.description, a.name "
        "FROM facts f "
        "JOIN activities a ON f.activity_id = a.id "
        "WHERE f.end_time IS NULL "
        "ORDER BY f.start_time DESC LIMIT 1");

    if (query.executeStep()) {
        return mapRowWithActivity(query);
    }
    return std::nullopt;
}

std::vector<models::Fact> FactRepository::findByDateRange(int64_t startTime, int64_t endTime) {
    std::vector<models::Fact> facts;

    SQLite::Statement query(db_.getHandle(),
        "SELECT f.id, f.activity_id, f.start_time, f.end_time, f.exported_to_youtrack, f.description, a.name "
        "FROM facts f "
        "JOIN activities a ON f.activity_id = a.id "
        "WHERE f.start_time >= ? AND f.start_time <= ? "
        "ORDER BY f.start_time ASC");
    query.bind(1, startTime);
    query.bind(2, endTime);

    while (query.executeStep()) {
        facts.push_back(mapRowWithActivity(query));
    }
    return facts;
}

std::vector<models::Fact> FactRepository::findByActivity(int64_t activityId) {
    std::vector<models::Fact> facts;

    SQLite::Statement query(db_.getHandle(),
        "SELECT f.id, f.activity_id, f.start_time, f.end_time, f.exported_to_youtrack, f.description, a.name "
        "FROM facts f "
        "JOIN activities a ON f.activity_id = a.id "
        "WHERE f.activity_id = ? "
        "ORDER BY f.start_time DESC");
    query.bind(1, activityId);

    while (query.executeStep()) {
        facts.push_back(mapRowWithActivity(query));
    }
    return facts;
}

std::vector<models::Fact> FactRepository::findRecent(int limit) {
    std::vector<models::Fact> facts;

    SQLite::Statement query(db_.getHandle(),
        "SELECT f.id, f.activity_id, f.start_time, f.end_time, f.exported_to_youtrack, f.description, a.name "
        "FROM facts f "
        "JOIN activities a ON f.activity_id = a.id "
        "ORDER BY f.start_time DESC LIMIT ?");
    query.bind(1, limit);

    while (query.executeStep()) {
        facts.push_back(mapRowWithActivity(query));
    }
    return facts;
}

void FactRepository::stopOngoing(int64_t endTime) {
    SQLite::Statement update(db_.getHandle(),
        "UPDATE facts SET end_time = ? WHERE end_time IS NULL");
    update.bind(1, endTime);
    update.exec();
}

void FactRepository::markFactsAsExported(const std::vector<int64_t>& factIds) {
    if (factIds.empty()) {
        return;
    }

    // Build IN clause with placeholders
    std::string placeholders;
    for (size_t i = 0; i < factIds.size(); ++i) {
        if (i > 0) placeholders += ",";
        placeholders += "?";
    }

    std::string sql = "UPDATE facts SET exported_to_youtrack = 1 WHERE id IN (" + placeholders + ")";
    SQLite::Statement update(db_.getHandle(), sql);

    for (size_t i = 0; i < factIds.size(); ++i) {
        update.bind(static_cast<int>(i + 1), factIds[i]);
    }

    update.exec();
}

} // namespace timetracker::repositories
