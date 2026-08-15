// cpr pulls in <windows.h>, whose min/max macros would break std::min/std::max below.
#define NOMINMAX

#include "KTalkImportService.hpp"
#include "localization/LocalizationManager.hpp"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <algorithm>

namespace timetracker {
namespace services {

namespace {

std::string trim(const std::string& value) {
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

}  // namespace

KTalkImportService::KTalkImportService(TimeTrackingService& timeTrackingService, SettingsService& settingsService)
    : timeTrackingService_(timeTrackingService), settingsService_(settingsService) {}

std::string KTalkImportService::normalizeSpaceUrl(const std::string& url) {
    std::string value = trim(url);
    if (value.empty()) {
        return "";
    }

    // A space is identified by scheme and host; anything the user pasted beyond that
    // (an API path, a room link) is dropped because the path is ours to build.
    if (value.find("://") == std::string::npos) {
        value = "https://" + value;
    }

    const size_t hostStart = value.find("://") + 3;
    const size_t hostEnd = value.find('/', hostStart);

    return hostEnd == std::string::npos ? value : value.substr(0, hostEnd);
}

std::map<std::string, std::string> KTalkImportService::buildHeaders() const {
    return {
        {"Authorization", "Session " + settingsService_.getKTalkToken()},
        {"Accept", "application/json"},
    };
}

bool KTalkImportService::hasConnection() const {
    return !settingsService_.getKTalkSpaceUrl().empty() && !settingsService_.getKTalkToken().empty();
}

std::string KTalkImportService::getSpaceHost() const {
    const std::string url = settingsService_.getKTalkSpaceUrl();
    const size_t schemeEnd = url.find("://");
    return schemeEnd == std::string::npos ? url : url.substr(schemeEnd + 3);
}

int64_t KTalkImportService::getTokenSavedAt() const {
    return settingsService_.getKTalkTokenSavedAt();
}

void KTalkImportService::setConnection(const std::string& spaceUrl, const std::string& token) {
    settingsService_.setKTalkSpaceUrl(normalizeSpaceUrl(spaceUrl));

    // An untouched password field comes back empty; that must not wipe a working token.
    const std::string trimmedToken = trim(token);
    if (!trimmedToken.empty() && trimmedToken != settingsService_.getKTalkToken()) {
        settingsService_.setKTalkToken(trimmedToken);
        settingsService_.setKTalkTokenSavedAt(static_cast<int64_t>(std::time(nullptr)));
    }
}

std::string KTalkImportService::testConnection() {
    auto& L = localization::L10n();

    if (settingsService_.getKTalkSpaceUrl().empty()) {
        return L.get("Enter the KTalk space address first");
    }
    if (settingsService_.getKTalkToken().empty()) {
        return L.get("Enter the KTalk token first");
    }

    // A single past day keeps the reply small enough to come back immediately.
    std::tm yesterdayTm = {};
    const std::time_t yesterday = std::time(nullptr) - 24 * 60 * 60;
#ifdef _WIN32
    localtime_s(&yesterdayTm, &yesterday);
#else
    localtime_r(&yesterday, &yesterdayTm);
#endif
    char day[16];
    std::strftime(day, sizeof(day), "%Y-%m-%d", &yesterdayTm);

    const FetchResult probe = fetchDay(day);
    return probe.errorMessage;
}

std::string KTalkImportService::convertToUtcIso8601(const std::string& localDateTime) {
    // Parse local datetime format: "YYYY-MM-DD HH:MM:SS"
    std::tm tm = {};
    std::istringstream ss(localDateTime);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    if (ss.fail()) {
        std::cerr << "Failed to parse local datetime: " << localDateTime << std::endl;
        return "";
    }

    // Convert local time to UTC timestamp
    std::time_t localTime = std::mktime(&tm);
    if (localTime == -1) {
        std::cerr << "Failed to convert local datetime to timestamp: " << localDateTime << std::endl;
        return "";
    }

    // Convert to UTC
    std::tm* utcTm = std::gmtime(&localTime);
    if (!utcTm) {
        std::cerr << "Failed to convert timestamp to UTC: " << localDateTime << std::endl;
        return "";
    }

    // Format as ISO 8601 with milliseconds: "YYYY-MM-DDTHH:mm:ss.000Z"
    std::ostringstream oss;
    oss << std::put_time(utcTm, "%Y-%m-%dT%H:%M:%S") << ".000Z";
    return oss.str();
}

std::string KTalkImportService::buildApiUrl(const std::string& spaceUrl, const std::string& fromDateTime, const std::string& toDateTime) {
    // Convert local datetime strings to UTC ISO 8601 format
    std::string fromDateUtc = convertToUtcIso8601(fromDateTime);
    std::string toDateUtc = convertToUtcIso8601(toDateTime);

    const std::string endpoint = spaceUrl + KTALK_API_PATH;

    if (fromDateUtc.empty() || toDateUtc.empty()) {
        std::cerr << "Failed to convert dates to UTC" << std::endl;
        return endpoint;  // Return endpoint without parameters on error
    }

    return endpoint + "?fromDate=" + fromDateUtc + "&toDate=" + toDateUtc +
           "&top=" + std::to_string(KTALK_MAX_MEETINGS_PER_DAY);
}

std::vector<std::string> KTalkImportService::enumerateDays(const std::string& fromDate, const std::string& toDate) {
    std::vector<std::string> days;

    if (fromDate.size() < 10 || toDate.size() < 10) {
        return days;
    }
    const std::string lastDay = toDate.substr(0, 10);

    std::tm cursor = {};
    std::istringstream ss(fromDate.substr(0, 10));
    ss >> std::get_time(&cursor, "%Y-%m-%d");
    if (ss.fail()) {
        return days;
    }
    cursor.tm_isdst = -1;

    for (int i = 0; i < KTALK_MAX_IMPORT_DAYS; ++i) {
        char day[16];
        std::strftime(day, sizeof(day), "%Y-%m-%d", &cursor);
        days.emplace_back(day);

        if (std::string(day) >= lastDay) {
            break;
        }

        // mktime normalises the month/year rollover that ++tm_mday can produce.
        cursor.tm_mday += 1;
        cursor.tm_isdst = -1;
        if (std::mktime(&cursor) == -1) {
            break;
        }
    }

    return days;
}

FetchResult KTalkImportService::fetchConferences(const std::string& fromDate, const std::string& toDate) {
    auto& L = localization::L10n();

    FetchResult result;

    const std::vector<std::string> days = enumerateDays(fromDate, toDate);
    if (days.empty()) {
        result.errorMessage = L.get("Please select both from and to dates");
        return result;
    }

    // One request per day. The API stops responding altogether once a single reply would
    // carry more than roughly a dozen conferences, and a day comfortably stays under that
    // while answering in well under a second.
    for (const std::string& day : days) {
        FetchResult dayResult = fetchDay(day);
        if (!dayResult.errorMessage.empty()) {
            dayResult.conferences.clear();
            return dayResult;
        }
        result.conferences.insert(result.conferences.end(),
                                  dayResult.conferences.begin(), dayResult.conferences.end());
    }

    return result;
}

FetchResult KTalkImportService::fetchDay(const std::string& day) {
    auto& L = localization::L10n();

    FetchResult result;

    const std::string url = buildApiUrl(settingsService_.getKTalkSpaceUrl(),
                                        day + " 00:00:00", day + " 23:59:59");

    try {
        // Build CPR headers
        cpr::Header cprHeaders;
        for (const auto& [key, value] : buildHeaders()) {
            cprHeaders[key] = value;
        }

        // Make GET request. The call blocks the render thread, so cap how long a stalled
        // connection can freeze the UI.
        auto response = cpr::Get(
            cpr::Url{url},
            cprHeaders,
            cpr::Timeout{30000}
        );

        // Check for network/connection errors
        if (response.error.code != cpr::ErrorCode::OK) {
            result.errorMessage = std::string(L.get("Failed to connect to KTalk API: ")) + response.error.message;
            return result;
        }

        // Check HTTP status
        if (response.status_code == 401 || response.status_code == 403) {
            result.errorMessage = L.get("KTalk rejected the saved credentials. The session token has most likely expired.");
            result.authFailed = true;
            return result;
        } else if (response.status_code >= 500) {
            result.errorMessage = L.get("KTalk API error. Please try again later.");
            return result;
        } else if (response.status_code >= 400) {
            result.errorMessage = std::string(L.get("KTalk API error ")) + std::to_string(response.status_code) + ": " + response.text;
            return result;
        }

        // Parse JSON response
        nlohmann::json jsonResponse = nlohmann::json::parse(response.text);

        // Extract conferences array
        if (!jsonResponse.contains("conferences") || !jsonResponse["conferences"].is_array()) {
            result.errorMessage = L.get("Unexpected response format from KTalk API");
            return result;
        }

        for (const auto& conf : jsonResponse["conferences"]) {
            if (conf.contains("startTime") && conf.contains("endTime")) {
                models::KTalkConference conference;
                conference.startTime = conf["startTime"].get<std::string>();
                conference.endTime = conf["endTime"].get<std::string>();
                // Title is optional - use empty string if missing
                conference.title = conf.contains("title") ? conf["title"].get<std::string>() : "";
                result.conferences.push_back(conference);
            }
        }

    } catch (const nlohmann::json::exception& e) {
        result.conferences.clear();
        result.errorMessage = "Unexpected response format from KTalk API";
    } catch (const std::exception& e) {
        result.conferences.clear();
        result.errorMessage = std::string(L.get("Exception during import: ")) + e.what();
    }

    return result;
}

int64_t KTalkImportService::parseIso8601(const std::string& iso8601) {
    // Parse ISO 8601 format: "2026-01-16T09:29:06Z" or "2026-01-16T10:22:31.144Z"
    std::tm tm = {};
    std::istringstream ss(iso8601);

    // Try parsing with seconds
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

    if (ss.fail()) {
        std::cerr << "Failed to parse ISO 8601 timestamp: " << iso8601 << std::endl;
        return 0;
    }

    // Convert UTC time to Unix timestamp
    // ISO 8601 with 'Z' suffix indicates UTC
#ifdef _WIN32
    // Windows: use _mkgmtime which interprets tm as UTC
    std::time_t time = _mkgmtime(&tm);
#else
    // POSIX: use timegm which interprets tm as UTC
    std::time_t time = timegm(&tm);
#endif

    if (time == -1) {
        std::cerr << "Failed to convert ISO 8601 timestamp to Unix time: " << iso8601 << std::endl;
        return 0;
    }

    return static_cast<int64_t>(time);
}

int64_t KTalkImportService::snapGridTime(int64_t t, int N) {
    time_t raw = static_cast<time_t>(t);
    struct tm localTm = {};
#ifdef _WIN32
    localtime_s(&localTm, &raw);
#else
    localtime_r(&raw, &localTm);
#endif
    int M = localTm.tm_min;
    int S = localTm.tm_sec;

    int64_t tNoSec = t - S;  // truncate to minute boundary

    int remainder = M % N;
    if (remainder * 2 < N) {
        // Round down
        return tNoSec - static_cast<int64_t>(remainder) * 60;
    } else {
        // Round up
        return tNoSec + static_cast<int64_t>(N - remainder) * 60;
    }
}

int64_t KTalkImportService::computeSnappedEndTime(int64_t rawEndTime, int64_t snappedStart,
                                                   int N, const std::vector<int64_t>& batchSnappedStarts) {
    int64_t intervalSecs = static_cast<int64_t>(N) * 60;

    // Find the earliest next-fact start within N minutes after rawEndTime.
    // Check already-processed meetings in the current batch.
    int64_t nextStart = 0;
    for (int64_t s : batchSnappedStarts) {
        if (s > rawEndTime && s <= rawEndTime + intervalSecs) {
            if (nextStart == 0 || s < nextStart) {
                nextStart = s;
            }
        }
    }

    // Check existing DB facts.
    auto dbFacts = timeTrackingService_.getEntriesForRange(rawEndTime + 1, rawEndTime + intervalSecs);
    for (const auto& f : dbFacts) {
        if (f.startTime > rawEndTime && (nextStart == 0 || f.startTime < nextStart)) {
            nextStart = f.startTime;
        }
    }

    int64_t snappedEnd = (nextStart > 0) ? nextStart : snapGridTime(rawEndTime, N);

    // Prevent collapsed or zero-duration interval.
    if (snappedEnd <= snappedStart) {
        snappedEnd = snappedStart + intervalSecs;
    }

    return snappedEnd;
}

bool KTalkImportService::isAlreadyImported(int64_t activityId, const std::string& title,
                                          int64_t startTime, int64_t endTime) const {
    // Nothing has ever been imported under this name, so nothing can be a duplicate.
    if (activityId < 0) {
        return false;
    }

    // An imported meeting is identified by its activity plus its title, not by its exact
    // timestamps. The stored interval drifts away from what a later import computes: end-time
    // snapping consults neighbouring facts (which only exist from the second import on), and
    // auto-fill, clipFactsOverlappingStart or a manual edit can clip an imported fact
    // afterwards. Matching on overlap survives all of that and keeps the import idempotent.
    // Clipping can only shrink a fact from within, so a survivor still overlaps the original
    // interval; a fact clipped away entirely is deleted, and re-importing it is correct.
    const int64_t windowEnd = std::max(endTime, startTime + 60);

    for (const auto& fact : timeTrackingService_.getEntriesOverlappingRange(startTime, windowEnd)) {
        // The ongoing fact has no end and is never an import.
        if (!fact.endTime.has_value()) {
            continue;
        }
        if (fact.activityId == activityId && fact.description == title) {
            return true;
        }
    }
    return false;
}

ImportResult KTalkImportService::importConferences(
    const std::string& fromDate,
    const std::string& toDate) {

    auto& L = localization::L10n();

    ImportResult result;

    // Validate date range
    if (fromDate.empty() || toDate.empty()) {
        result.errorMessage = L.get("Please select both from and to dates");
        return result;
    }

    if (fromDate > toDate) {
        result.errorMessage = L.get("End date must be after start date");
        return result;
    }

    // Validate date range does not exceed maximum allowed days
    {
        std::tm fromTm = {}, toTm = {};
        std::istringstream fromSs(fromDate), toSs(toDate);
        fromSs >> std::get_time(&fromTm, "%Y-%m-%d %H:%M:%S");
        toSs >> std::get_time(&toTm, "%Y-%m-%d %H:%M:%S");
        if (!fromSs.fail() && !toSs.fail()) {
            std::time_t fromTime = std::mktime(&fromTm);
            std::time_t toTime = std::mktime(&toTm);
            int days = static_cast<int>((toTime - fromTime) / (60 * 60 * 24)) + 1;
            if (days > KTALK_MAX_IMPORT_DAYS) {
                char buf[128];
                snprintf(buf, sizeof(buf), L.get("Date range exceeds maximum of %d days"), KTALK_MAX_IMPORT_DAYS);
                result.errorMessage = buf;
                return result;
            }
        }
    }

    if (!hasConnection()) {
        result.errorMessage = L.get("KTalk is not configured yet. Set the space address and token in Settings.");
        result.authFailed = true;
        return result;
    }

    FetchResult fetched = fetchConferences(fromDate, toDate);
    if (!fetched.errorMessage.empty()) {
        result.errorMessage = fetched.errorMessage;
        result.authFailed = fetched.authFailed;
        return result;
    }

    const std::vector<models::KTalkConference>& conferences = fetched.conferences;

    if (conferences.empty()) {
        result.errorMessage = L.get("No meetings found in selected date range");
        return result;
    }

    // Get the "Include unplanned meetings" setting
    bool includeUnplanned = settingsService_.getKTalkIncludeUnplanned();

    // Filter conferences based on setting
    std::vector<models::KTalkConference> filteredConferences;
    for (const auto& conf : conferences) {
        // If includeUnplanned is false, skip conferences without a title
        if (!includeUnplanned && conf.title.empty()) {
            continue;
        }
        filteredConferences.push_back(conf);
    }

    if (filteredConferences.empty()) {
        result.errorMessage = L.get("No meetings found matching filter criteria");
        return result;
    }

    // Get snap interval; sort by start time so end-time snapping sees prior meetings
    int snapInterval = settingsService_.getKTalkSnapInterval();
    if (snapInterval > 0) {
        std::sort(filteredConferences.begin(), filteredConferences.end(),
            [](const models::KTalkConference& a, const models::KTalkConference& b) {
                return a.startTime < b.startTime;
            });
    }

    // Import each conference
    int imported = 0;
    int duplicatesSkipped = 0;
    std::string activityName = L.get("KTalk meeting");

    // Resolve the activity up front for the duplicate check. The lookup is case-insensitive, so
    // this is the row addManualEntry will reuse even when its stored name differs in case from
    // the localized one (older builds wrote "KTalk Meeting"). Facts must be matched against this
    // id, not against activityName. Absent means no meeting has ever been imported under it.
    const auto importActivity = timeTrackingService_.findActivity(activityName);
    int64_t importActivityId = importActivity.has_value() ? importActivity->id : -1;

    std::vector<int64_t> batchSnappedStarts;  // snapped starts of already-processed batch meetings
    for (const auto& conf : filteredConferences) {
        // Parse timestamps
        int64_t startTime = parseIso8601(conf.startTime);
        int64_t endTime = parseIso8601(conf.endTime);

        if (startTime == 0 || endTime == 0) {
            std::string displayTitle = conf.title.empty() ? "Untitled meeting" : conf.title;
            std::cerr << "Warning: Skipping meetings with invalid timestamps: " << displayTitle << std::endl;
            continue;
        }

        const int64_t rawStartTime = startTime;
        const int64_t rawEndTime = endTime;

        // Apply snapping if enabled
        if (snapInterval > 0) {
            int64_t snappedStart = snapGridTime(startTime, snapInterval);
            int64_t snappedEnd = computeSnappedEndTime(endTime, snappedStart, snapInterval, batchSnappedStarts);
            batchSnappedStarts.push_back(snappedStart);
            startTime = snappedStart;
            endTime = snappedEnd;
        }

        // Create description: "{title}", or empty for an unplanned meeting
        std::string description;
        if (!conf.title.empty()) {
            description = conf.title;
        }

        // Check for duplicate before inserting. Snapping can move the interval in either
        // direction, so look for an existing import anywhere across the raw and the snapped
        // interval.
        if (isAlreadyImported(importActivityId, description,
                              std::min(rawStartTime, startTime), std::max(rawEndTime, endTime))) {
            duplicatesSkipped++;
            continue;
        }

        try {
            // The activity holds the meeting; the conference title becomes the fact description.
            const models::Fact fact =
                timeTrackingService_.addManualEntry(activityName, startTime, endTime, description);
            // The first insert is what creates the activity on a first-ever import; pick the id up
            // so the rest of this batch is still deduplicated (a meeting spanning midnight comes
            // back in both days' responses).
            importActivityId = fact.activityId;
            imported++;
        } catch (const std::exception& e) {
            std::string displayTitle = conf.title.empty() ? "Untitled meeting" : conf.title;
            std::cerr << "Error importing meeting '" << displayTitle << "': " << e.what() << std::endl;
        }
    }

    if (imported == 0 && duplicatesSkipped > 0) {
        result.errorMessage = L.get("All meetings in this range have already been imported");
        return result;
    }

    result.success = true;
    result.conferencesImported = imported;
    result.duplicatesSkipped = duplicatesSkipped;
    return result;
}

}  // namespace services
}  // namespace timetracker
