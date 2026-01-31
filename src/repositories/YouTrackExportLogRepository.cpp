#include "YouTrackExportLogRepository.hpp"

namespace timetracker::repositories {

YouTrackExportLogRepository::YouTrackExportLogRepository(database::Database& db) : db_(db) {}

models::YouTrackExportLogEntry YouTrackExportLogRepository::mapRow(SQLite::Statement& query) {
    models::YouTrackExportLogEntry entry;
    entry.id = query.getColumn(0).getInt64();
    entry.activityName = query.getColumn(1).getString();
    entry.issueId = query.getColumn(2).getString();
    entry.trackedDate = query.getColumn(3).getString();
    entry.durationMinutes = query.getColumn(4).getInt();
    entry.exportedAt = query.getColumn(5).getInt64();
    return entry;
}

void YouTrackExportLogRepository::addLogEntry(const std::string& activityName,
                                             const std::string& issueId,
                                             const std::string& trackedDate,
                                             int durationMinutes,
                                             int64_t exportedAt) {
    SQLite::Statement insert(db_.getHandle(),
        "INSERT INTO youtrack_export_log (activity_name, issue_id, tracked_date, duration_minutes, exported_at) "
        "VALUES (?, ?, ?, ?, ?)");
    insert.bind(1, activityName);
    insert.bind(2, issueId);
    insert.bind(3, trackedDate);
    insert.bind(4, durationMinutes);
    insert.bind(5, exportedAt);
    insert.exec();
}

std::vector<models::YouTrackExportLogEntry> YouTrackExportLogRepository::getLogEntries(int offset, int limit) {
    SQLite::Statement query(db_.getHandle(),
        "SELECT id, activity_name, issue_id, tracked_date, duration_minutes, exported_at "
        "FROM youtrack_export_log "
        "ORDER BY exported_at DESC "
        "LIMIT ? OFFSET ?");
    query.bind(1, limit);
    query.bind(2, offset);

    std::vector<models::YouTrackExportLogEntry> entries;
    while (query.executeStep()) {
        entries.push_back(mapRow(query));
    }
    return entries;
}

int YouTrackExportLogRepository::getTotalCount() {
    SQLite::Statement query(db_.getHandle(),
        "SELECT COUNT(*) FROM youtrack_export_log");
    if (query.executeStep()) {
        return query.getColumn(0).getInt();
    }
    return 0;
}

void YouTrackExportLogRepository::clearLog() {
    db_.getHandle().exec("DELETE FROM youtrack_export_log");
}

} // namespace timetracker::repositories
