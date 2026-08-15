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

// Per-day cap sent as the API's `top` parameter. The range is walked one day at a time
// because the API stops responding when a single reply would carry more than roughly a
// dozen conferences — see fetchConferences.
constexpr int KTALK_MAX_MEETINGS_PER_DAY = 100;

// API path appended to the configured space URL. Matched case-insensitively by the server.
constexpr const char* KTALK_API_PATH = "/api/conferencesHistory";

// Run in the browser's DevTools console on a logged-in KTalk tab: copies the session token
// to the clipboard. localStorage is where the web app keeps it; the cookie branch covers
// spaces that authenticate the other documented way.
constexpr const char* KTALK_TOKEN_SNIPPET =
    R"JS((()=>{try{const t=JSON.parse(localStorage.session).data.token;copy(t);return t}catch(e){}const m=document.cookie.match(/(?:^|;\s*)sessionToken=([^;]+)/);if(m){copy(m[1]);return m[1]}return 'TOKEN NOT FOUND'})())JS";

struct FetchResult {
    std::vector<models::KTalkConference> conferences;
    std::string errorMessage;
    bool authFailed{false};
};

struct ImportResult {
    bool success{false};
    std::string errorMessage;
    int conferencesImported{0};
    int duplicatesSkipped{0};
    // Set when KTalk rejected the credentials, so the UI can point at the settings
    // instead of showing a generic error.
    bool authFailed{false};
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
     * Build the API URL for a single day.
     * @param spaceUrl Configured space URL
     * @param fromDateTime Start of the window, "YYYY-MM-DD HH:MM:SS" local time
     * @param toDateTime End of the window, "YYYY-MM-DD HH:MM:SS" local time
     * @return Complete API URL with query parameters
     */
    std::string buildApiUrl(const std::string& spaceUrl, const std::string& fromDateTime, const std::string& toDateTime);

    /**
     * Fetch conferences for a date range, one request per day.
     * @param fromDate Range start, "YYYY-MM-DD HH:MM:SS" local time
     * @param toDate Range end, "YYYY-MM-DD HH:MM:SS" local time
     * @return Conferences, or an error message with the authentication flag set
     */
    FetchResult fetchConferences(const std::string& fromDate, const std::string& toDate);

    /**
     * Import conferences as time tracking entries using the configured connection.
     * @param fromDate Start date in "YYYY-MM-DD HH:MM:SS" format
     * @param toDate End date in "YYYY-MM-DD HH:MM:SS" format
     * @return Import result with success status and details
     */
    ImportResult importConferences(const std::string& fromDate, const std::string& toDate);

    /**
     * Whether both a space URL and a token are configured.
     */
    bool hasConnection() const;

    /**
     * Host of the configured space, for display purposes.
     * @return Host part of the space URL, or an empty string if unconfigured
     */
    std::string getSpaceHost() const;

    /**
     * When the session token was last saved.
     * @return Unix timestamp in seconds, or 0 if no token is stored
     */
    int64_t getTokenSavedAt() const;

    /**
     * Store the connection. The space URL is normalised to scheme and host.
     * @param spaceUrl Space URL, e.g. "https://example.ktalk.ru"
     * @param token Session token; pass an empty string to leave the stored one untouched
     */
    void setConnection(const std::string& spaceUrl, const std::string& token);

    /**
     * Issue a cheap request to confirm the stored credentials work.
     * @return Empty string on success, otherwise a human-readable error
     */
    std::string testConnection();

    /**
     * Reduce a user-entered URL to scheme and host, dropping any path and trailing slash.
     */
    static std::string normalizeSpaceUrl(const std::string& url);

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
     * Convert local datetime to UTC ISO 8601 format.
     * @param localDateTime Local datetime string in "YYYY-MM-DD HH:MM:SS" format
     * @return UTC ISO 8601 string in "YYYY-MM-DDTHH:mm:ss.sssZ" format, or empty string on error
     */
    std::string convertToUtcIso8601(const std::string& localDateTime);

    /**
     * Authorization headers for the configured token.
     */
    std::map<std::string, std::string> buildHeaders() const;

    /**
     * Fetch a single day's conferences.
     * @param day Date in "YYYY-MM-DD" format
     */
    FetchResult fetchDay(const std::string& day);

    /**
     * List the days a range covers, inclusive, capped at KTALK_MAX_IMPORT_DAYS.
     * @return Dates in "YYYY-MM-DD" format
     */
    std::vector<std::string> enumerateDays(const std::string& fromDate, const std::string& toDate);

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

    /**
     * Whether this meeting is already present as a fact.
     * Matches on activity id plus title over any fact overlapping the interval rather than on
     * exact timestamps, because a stored interval drifts (snapping, auto-fill clipping, manual
     * edits) from what a later import would compute. The id is what getOrCreate resolves the
     * import activity to; comparing names instead misses a stored name that only differs in case.
     * @param activityId Activity imported meetings belong to, or a negative value if none exists
     * @param title Meeting title, stored as the fact description (empty for unplanned meetings)
     * @param startTime Interval start in seconds
     * @param endTime Interval end in seconds; widened to a minute if it is not after startTime
     * @return true if a completed fact with the same activity and title overlaps the interval
     */
    bool isAlreadyImported(int64_t activityId, const std::string& title,
                           int64_t startTime, int64_t endTime) const;
};

}  // namespace services
}  // namespace timetracker
