#include "KTalkImportService.hpp"
#include "localization/LocalizationManager.hpp"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <algorithm>

namespace timetracker {
namespace services {

KTalkImportService::KTalkImportService(TimeTrackingService& timeTrackingService, SettingsService& settingsService)
    : timeTrackingService_(timeTrackingService), settingsService_(settingsService) {}

std::optional<FetchPayload> KTalkImportService::parseFetchPayload(const std::string& payload) {
    FetchPayload result;

    // Extract URL using regex: fetch("URL", ...)
    std::regex urlRegex(R"(fetch\s*\(\s*"([^"]+))");
    std::smatch urlMatch;
    if (!std::regex_search(payload, urlMatch, urlRegex)) {
        return std::nullopt;  // Could not extract URL
    }
    result.url = urlMatch[1].str();

    // Extract the options object (everything between the outer braces after the URL)
    // Pattern: fetch("url", { ... })
    size_t firstBrace = payload.find('{');
    if (firstBrace == std::string::npos) {
        return std::nullopt;  // No options object found
    }

    // Find matching closing brace
    int braceCount = 0;
    size_t lastBrace = firstBrace;
    for (size_t i = firstBrace; i < payload.size(); ++i) {
        if (payload[i] == '{') {
            braceCount++;
        } else if (payload[i] == '}') {
            braceCount--;
            if (braceCount == 0) {
                lastBrace = i;
                break;
            }
        }
    }

    if (braceCount != 0) {
        return std::nullopt;  // Unmatched braces
    }

    std::string optionsJson = payload.substr(firstBrace, lastBrace - firstBrace + 1);

    // Parse JSON options
    try {
        nlohmann::json options = nlohmann::json::parse(optionsJson);

        // Extract headers
        if (options.contains("headers") && options["headers"].is_object()) {
            for (auto& [key, value] : options["headers"].items()) {
                if (value.is_string()) {
                    result.headers[key] = value.get<std::string>();
                }
            }
        } else {
            return std::nullopt;  // No headers found
        }

    } catch (const nlohmann::json::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return std::nullopt;
    }

    return result;
}

std::string KTalkImportService::extractBaseUrl(const std::string& url) {
    size_t queryPos = url.find('?');
    if (queryPos != std::string::npos) {
        return url.substr(0, queryPos);
    }
    return url;
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

std::string KTalkImportService::buildApiUrl(const std::string& baseUrl, const std::string& fromDate, const std::string& toDate) {
    // Convert local datetime strings to UTC ISO 8601 format
    std::string fromDateUtc = convertToUtcIso8601(fromDate);
    std::string toDateUtc = convertToUtcIso8601(toDate);

    if (fromDateUtc.empty() || toDateUtc.empty()) {
        std::cerr << "Failed to convert dates to UTC" << std::endl;
        return baseUrl;  // Return base URL without parameters on error
    }

    int top = KTALK_MAX_IMPORT_DAYS * KTALK_MAX_MEETINGS_PER_DAY;
    return baseUrl + "?fromDate=" + fromDateUtc + "&toDate=" + toDateUtc + "&top=" + std::to_string(top);
}

std::pair<std::vector<models::KTalkConference>, std::string> KTalkImportService::fetchConferences(
    const std::string& url,
    const std::map<std::string, std::string>& headers) {

    std::vector<models::KTalkConference> conferences;

    try {
        // Build CPR headers
        cpr::Header cprHeaders;
        for (const auto& [key, value] : headers) {
            cprHeaders[key] = value;
        }

        // Make GET request
        auto response = cpr::Get(
            cpr::Url{url},
            cprHeaders
        );

        // Check for network/connection errors
        if (response.error.code != cpr::ErrorCode::OK) {
            return {conferences, "Failed to connect to KTalk API: " + response.error.message};
        }

        // Check HTTP status
        if (response.status_code == 401 || response.status_code == 403) {
            return {conferences, "Authentication failed. Please copy a fresh fetch payload from your browser."};
        } else if (response.status_code >= 500) {
            return {conferences, "KTalk API error. Please try again later."};
        } else if (response.status_code >= 400) {
            return {conferences, "KTalk API error " + std::to_string(response.status_code) + ": " + response.text};
        }

        // Parse JSON response
        nlohmann::json jsonResponse = nlohmann::json::parse(response.text);

        // Extract conferences array
        if (!jsonResponse.contains("conferences") || !jsonResponse["conferences"].is_array()) {
            return {conferences, "Unexpected response format from KTalk API"};
        }

        for (const auto& conf : jsonResponse["conferences"]) {
            if (conf.contains("startTime") && conf.contains("endTime")) {
                models::KTalkConference conference;
                conference.startTime = conf["startTime"].get<std::string>();
                conference.endTime = conf["endTime"].get<std::string>();
                // Title is optional - use empty string if missing
                conference.title = conf.contains("title") ? conf["title"].get<std::string>() : "";
                conferences.push_back(conference);
            }
        }

    } catch (const nlohmann::json::exception& e) {
        return {conferences, "Unexpected response format from KTalk API"};
    } catch (const std::exception& e) {
        return {conferences, std::string("Exception during import: ") + e.what()};
    }

    return {conferences, ""};  // Success
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

ImportResult KTalkImportService::importConferences(
    const std::string& fetchPayload,
    const std::string& fromDate,
    const std::string& toDate) {

    auto& L = localization::L10n();

    ImportResult result;

    // Validate date range
    if (fromDate.empty() || toDate.empty()) {
        result.errorMessage = "Please select both from and to dates";
        return result;
    }

    if (fromDate > toDate) {
        result.errorMessage = "End date must be after start date";
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

    // Parse fetch payload
    auto payloadOpt = parseFetchPayload(fetchPayload);
    if (!payloadOpt.has_value()) {
        result.errorMessage = "Invalid fetch payload. Expected JavaScript fetch() code.";
        return result;
    }

    FetchPayload payload = *payloadOpt;

    // Build API URL
    std::string baseUrl = extractBaseUrl(payload.url);
    std::string apiUrl = buildApiUrl(baseUrl, fromDate, toDate);

    // Fetch conferences
    auto [conferences, error] = fetchConferences(apiUrl, payload.headers);
    if (!error.empty()) {
        result.errorMessage = error;
        return result;
    }

    if (conferences.empty()) {
        result.errorMessage = "No conferences found in selected date range";
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
        result.errorMessage = "No conferences found matching filter criteria";
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
    std::vector<int64_t> batchSnappedStarts;  // snapped starts of already-processed batch meetings
    for (const auto& conf : filteredConferences) {
        // Parse timestamps
        int64_t startTime = parseIso8601(conf.startTime);
        int64_t endTime = parseIso8601(conf.endTime);

        if (startTime == 0 || endTime == 0) {
            std::string displayTitle = conf.title.empty() ? "Untitled meeting" : conf.title;
            std::cerr << "Warning: Skipping conference with invalid timestamps: " << displayTitle << std::endl;
            continue;
        }

        // Apply snapping if enabled
        if (snapInterval > 0) {
            int64_t snappedStart = snapGridTime(startTime, snapInterval);
            int64_t snappedEnd = computeSnappedEndTime(endTime, snappedStart, snapInterval, batchSnappedStarts);
            batchSnappedStarts.push_back(snappedStart);
            startTime = snappedStart;
            endTime = snappedEnd;
        }

        // Create description: "{title} (KTalk)" or "Unplanned meeting (KTalk)" if no title
        std::string description;
        if (!conf.title.empty()) {
            description = conf.title;
        }

        try {
            // Create time entry with activity name "KTalk Meeting" and conference title in description
            timeTrackingService_.addManualEntry(L.get("KTalk Meeting"), startTime, endTime, description);
            imported++;
        } catch (const std::exception& e) {
            std::string displayTitle = conf.title.empty() ? "Untitled meeting" : conf.title;
            std::cerr << "Error importing conference '" << displayTitle << "': " << e.what() << std::endl;
        }
    }

    result.success = true;
    result.conferencesImported = imported;
    return result;
}

}  // namespace services
}  // namespace timetracker
