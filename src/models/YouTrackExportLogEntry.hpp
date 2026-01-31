#pragma once

#include <string>
#include <cstdint>

namespace timetracker::models {

struct YouTrackExportLogEntry {
    int64_t id{0};
    std::string activityName;
    std::string issueId;
    std::string trackedDate;        // ISO8601 format YYYY-MM-DD
    int durationMinutes{0};
    int64_t exportedAt{0};          // Unix timestamp

    YouTrackExportLogEntry() = default;

    YouTrackExportLogEntry(int64_t id, std::string activityName, std::string issueId,
                          std::string trackedDate, int durationMinutes, int64_t exportedAt)
        : id(id), activityName(std::move(activityName)), issueId(std::move(issueId)),
          trackedDate(std::move(trackedDate)), durationMinutes(durationMinutes),
          exportedAt(exportedAt) {}
};

} // namespace timetracker::models
