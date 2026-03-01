#pragma once

#include "TimeTrackingService.hpp"
#include "SettingsService.hpp"
#include "models/KTalkConference.hpp"
#include <string>
#include <vector>
#include <optional>
#include <map>

namespace timetracker {
namespace services {

// Maximum number of days allowed for a single KTalk import
constexpr int KTALK_MAX_IMPORT_DAYS = 10;

// Estimated maximum number of meetings per day (used for API pagination)
constexpr int KTALK_MAX_MEETINGS_PER_DAY = 10;

struct FetchPayload {
    std::string url;
    std::map<std::string, std::string> headers;
};

struct ImportResult {
    bool success{false};
    std::string errorMessage;
    int conferencesImported{0};
};

class KTalkImportService {
public:
    explicit KTalkImportService(TimeTrackingService& timeTrackingService, SettingsService& settingsService);
    ~KTalkImportService() = default;

    // Non-copyable, non-movable
    KTalkImportService(const KTalkImportService&) = delete;
    KTalkImportService& operator=(const KTalkImportService&) = delete;
    KTalkImportService(KTalkImportService&&) = delete;
    KTalkImportService& operator=(KTalkImportService&&) = delete;

    /**
     * Parse JavaScript fetch() payload to extract URL and headers.
     * @param payload JavaScript fetch() code as string
     * @return FetchPayload with URL and headers, or std::nullopt if parsing fails
     */
    std::optional<FetchPayload> parseFetchPayload(const std::string& payload);

    /**
     * Build KTalk API URL with date range parameters.
     * @param baseUrl Base URL (without query parameters)
     * @param fromDate Start date in YYYY-MM-DD format
     * @param toDate End date in YYYY-MM-DD format
     * @return Complete API URL with query parameters
     */
    std::string buildApiUrl(const std::string& baseUrl, const std::string& fromDate, const std::string& toDate);

    /**
     * Fetch conferences from KTalk API.
     * @param url API URL with date range parameters
     * @param headers HTTP headers for authentication
     * @return Vector of conferences or error message
     */
    std::pair<std::vector<models::KTalkConference>, std::string> fetchConferences(
        const std::string& url,
        const std::map<std::string, std::string>& headers);

    /**
     * Import conferences as time tracking entries.
     * @param fetchPayload JavaScript fetch() payload
     * @param fromDate Start date in YYYY-MM-DD format
     * @param toDate End date in YYYY-MM-DD format
     * @return Import result with success status and details
     */
    ImportResult importConferences(const std::string& fetchPayload, const std::string& fromDate, const std::string& toDate);

private:
    TimeTrackingService& timeTrackingService_;
    SettingsService& settingsService_;

    /**
     * Parse ISO 8601 timestamp to Unix timestamp.
     * @param iso8601 ISO 8601 timestamp string (e.g., "2026-01-16T09:29:06Z")
     * @return Unix timestamp in seconds, or 0 if parsing fails
     */
    int64_t parseIso8601(const std::string& iso8601);

    /**
     * Extract base URL (part before '?') from full URL.
     * @param url Full URL
     * @return Base URL without query parameters
     */
    std::string extractBaseUrl(const std::string& url);

    /**
     * Convert local datetime to UTC ISO 8601 format.
     * @param localDateTime Local datetime string in "YYYY-MM-DD HH:MM:SS" format
     * @return UTC ISO 8601 string in "YYYY-MM-DDTHH:mm:ss.sssZ" format, or empty string on error
     */
    std::string convertToUtcIso8601(const std::string& localDateTime);

    /**
     * Snap a Unix timestamp to the nearest multiple of intervalMinutes in local time.
     * Seconds are truncated. Ties (exact midpoint) round up.
     * @param t Unix timestamp in seconds
     * @param intervalMinutes Grid spacing in minutes
     * @return Snapped Unix timestamp
     */
    int64_t snapGridTime(int64_t t, int intervalMinutes);

    /**
     * Compute snapped end time for an imported conference.
     * Snaps to the start of the next nearby fact (DB + current batch) if one exists
     * within intervalMinutes; otherwise snaps by the grid algorithm.
     * Pushes end forward if it would equal or precede snappedStart.
     * @param rawEndTime Raw (unsnapped) end time in seconds
     * @param snappedStart Already-snapped start time for this conference
     * @param intervalMinutes Snap interval in minutes
     * @param batchSnappedStarts Snapped start times of already-processed batch conferences
     * @return Snapped end time in seconds
     */
    int64_t computeSnappedEndTime(int64_t rawEndTime, int64_t snappedStart, int intervalMinutes,
                                   const std::vector<int64_t>& batchSnappedStarts);
};

}  // namespace services
}  // namespace timetracker
