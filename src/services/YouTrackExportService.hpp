#pragma once

#include "SettingsService.hpp"
#include "repositories/FactRepository.hpp"
#include "repositories/YouTrackExportLogRepository.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <atomic>

namespace timetracker::services {

struct AggregatedWorkItem {
    std::string issueId;
    std::string activityName;  // Original activity name
    std::string date;          // YYYY-MM-DD format
    int minutes;               // Rounded minutes
    std::vector<int64_t> factIds;  // IDs of facts included in this work item
};

struct ExportResult {
    bool success{false};
    std::string errorMessage;
    int totalMinutes{0};
    int itemsExported{0};
    std::vector<models::Fact> overlappingFacts;  // Populated when overlap detected
};

struct PrepareExportResult {
    std::vector<AggregatedWorkItem> workItems;
    std::vector<std::string> unresolvedActivities;  // Activities that couldn't be mapped to valid issue IDs
    int skippedExportedCount{0};  // Facts skipped because they were already exported
};

class YouTrackExportService {
public:
    YouTrackExportService(
        SettingsService& settingsService,
        repositories::FactRepository& factRepository,
        repositories::YouTrackExportLogRepository& exportLogRepository);
    ~YouTrackExportService() = default;

    // Non-copyable, non-movable
    YouTrackExportService(const YouTrackExportService&) = delete;
    YouTrackExportService& operator=(const YouTrackExportService&) = delete;
    YouTrackExportService(YouTrackExportService&&) = delete;
    YouTrackExportService& operator=(YouTrackExportService&&) = delete;

    /**
     * Check if YouTrack export is configured.
     * @return true if URL and token are set
     */
    bool isConfigured() const;

    /**
     * Get configuration error message if not configured.
     * @return error message or empty string if configured
     */
    std::string getConfigurationError() const;

    /**
     * Check for overlapping facts in the date range.
     * @param startTime Start of date range (Unix timestamp)
     * @param endTime End of date range (Unix timestamp)
     * @return Vector of overlapping facts (empty if no overlaps)
     */
    std::vector<models::Fact> checkForOverlaps(int64_t startTime, int64_t endTime);

    /**
     * Prepare export data for preview (aggregated work items).
     * @param startTime Start of date range (Unix timestamp)
     * @param endTime End of date range (Unix timestamp)
     * @return Result containing work items and any unresolved activity names
     */
    PrepareExportResult prepareExport(int64_t startTime, int64_t endTime);

    /**
     * Export aggregated work items to YouTrack.
     * @param workItems Work items to export (from prepareExport)
     * @param progressCallback Optional callback for progress updates (current, total)
     * @param cancelFlag Optional pointer to bool flag for cancellation check
     * @return Export result with success status and details
     */
    ExportResult exportToYouTrack(
        const std::vector<AggregatedWorkItem>& workItems,
        std::function<void(int, int)> progressCallback = nullptr,
        const std::atomic<bool>* cancelFlag = nullptr);

private:
    SettingsService& settingsService_;
    repositories::FactRepository& factRepository_;
    repositories::YouTrackExportLogRepository& exportLogRepository_;

    /**
     * Resolve activity name to YouTrack issue ID using aliases.
     * @param activityName Activity name from database
     * @return Issue ID (from alias or activity name itself)
     */
    std::string resolveIssueId(const std::string& activityName) const;

    /**
     * Validate issue ID format (PROJECT-NUMBER).
     * @param issueId Issue ID to validate
     * @return true if valid format
     */
    bool validateIssueId(const std::string& issueId) const;

    /**
     * Round minutes up to nearest 10.
     * @param minutes Input minutes
     * @return Rounded minutes (minimum 10)
     */
    int roundUpToNearest10(int minutes) const;

    /**
     * Post a single work item to YouTrack API.
     * @param issueId YouTrack issue ID
     * @param minutes Duration in minutes
     * @param date Date in YYYY-MM-DD format
     * @return Error message or empty string on success
     */
    std::string postWorkItem(const std::string& issueId, int minutes, const std::string& date);

    /**
     * Convert date string (YYYY-MM-DD) to milliseconds since epoch.
     * @param dateStr Date string
     * @return Milliseconds since epoch
     */
    int64_t dateToMilliseconds(const std::string& dateStr) const;
};

} // namespace timetracker::services
