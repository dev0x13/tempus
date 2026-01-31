#pragma once

#include "IYouTrackExportLogRepository.hpp"
#include "database/Database.hpp"

namespace timetracker::repositories {

class YouTrackExportLogRepository : public IYouTrackExportLogRepository {
public:
    explicit YouTrackExportLogRepository(database::Database& db);
    ~YouTrackExportLogRepository() override = default;

    void addLogEntry(const std::string& activityName, const std::string& issueId,
                    const std::string& trackedDate, int durationMinutes,
                    int64_t exportedAt) override;

    std::vector<models::YouTrackExportLogEntry> getLogEntries(int offset, int limit) override;
    int getTotalCount() override;
    void clearLog() override;

private:
    database::Database& db_;

    models::YouTrackExportLogEntry mapRow(SQLite::Statement& query);
};

} // namespace timetracker::repositories
