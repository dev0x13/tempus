#pragma once

#include "models/YouTrackExportLogEntry.hpp"
#include <vector>
#include <string>
#include <cstdint>

namespace timetracker::repositories {

class IYouTrackExportLogRepository {
public:
    virtual ~IYouTrackExportLogRepository() = default;

    // Add a new log entry
    virtual void addLogEntry(const std::string& activityName, const std::string& issueId,
                            const std::string& trackedDate, int durationMinutes,
                            int64_t exportedAt) = 0;

    // Get paginated log entries (ordered by exported_at DESC)
    virtual std::vector<models::YouTrackExportLogEntry> getLogEntries(int offset, int limit) = 0;

    // Get total count of log entries for pagination
    virtual int getTotalCount() = 0;

    // Clear all log entries
    virtual void clearLog() = 0;
};

} // namespace timetracker::repositories
