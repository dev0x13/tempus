#include "YouTrackExportService.hpp"
#include "utils/TimeUtils.hpp"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <map>
#include <set>
#include <iostream>

namespace timetracker::services {

YouTrackExportService::YouTrackExportService(
    SettingsService& settingsService,
    repositories::FactRepository& factRepository,
    repositories::YouTrackExportLogRepository& exportLogRepository)
    : settingsService_(settingsService)
    , factRepository_(factRepository)
    , exportLogRepository_(exportLogRepository) {}

bool YouTrackExportService::isConfigured() const {
    std::string url = settingsService_.getYouTrackUrl();
    std::string token = settingsService_.getYouTrackToken();
    return !url.empty() && !token.empty();
}

std::string YouTrackExportService::getConfigurationError() const {
    std::string url = settingsService_.getYouTrackUrl();
    std::string token = settingsService_.getYouTrackToken();

    if (url.empty() && token.empty()) {
        return "YouTrack URL and token not configured. Edit settings.json.";
    } else if (url.empty()) {
        return "YouTrack URL not configured. Edit settings.json.";
    } else if (token.empty()) {
        return "YouTrack token not configured. Edit settings.json.";
    }
    return "";
}

std::vector<models::Fact> YouTrackExportService::checkForOverlaps(int64_t startTime, int64_t endTime) {
    return factRepository_.findOverlappingFacts(startTime, endTime);
}

PrepareExportResult YouTrackExportService::prepareExport(int64_t startTime, int64_t endTime) {
    // Get all facts in date range (not just unexported ones - allow re-export)
    auto facts = factRepository_.findByDateRange(startTime, endTime);

    // Aggregate by (date, issue_id)
    // Key: "YYYY-MM-DD|ISSUE-ID"
    std::map<std::string, AggregatedWorkItem> aggregated;
    std::set<std::string> unresolvedActivities;

    for (const auto& fact : facts) {
        // Skip ongoing facts (no end time)
        if (!fact.endTime.has_value()) {
            continue;
        }

        // Resolve issue ID
        std::string issueId = resolveIssueId(fact.activityName);

        // Collect unresolved activities instead of silently skipping
        if (!validateIssueId(issueId)) {
            unresolvedActivities.insert(fact.activityName);
            continue;
        }

        // Extract date (YYYY-MM-DD)
        std::time_t startTimeT = static_cast<std::time_t>(fact.startTime);
        std::tm* tm = std::localtime(&startTimeT);
        std::ostringstream dateStream;
        dateStream << std::put_time(tm, "%Y-%m-%d");
        std::string date = dateStream.str();

        // Create aggregation key
        std::string key = date + "|" + issueId;

        // Initialize or update aggregated item
        if (aggregated.find(key) == aggregated.end()) {
            AggregatedWorkItem item;
            item.issueId = issueId;
            item.activityName = fact.activityName;
            item.date = date;
            item.minutes = 0;
            aggregated[key] = item;
        }

        // Add duration
        int64_t durationSeconds = *fact.endTime - fact.startTime;
        int durationMinutes = static_cast<int>(durationSeconds / 60);
        aggregated[key].minutes += durationMinutes;
        aggregated[key].factIds.push_back(fact.id);
    }

    PrepareExportResult result;
    result.unresolvedActivities = std::vector<std::string>(unresolvedActivities.begin(), unresolvedActivities.end());

    // Round up durations and convert to vector
    for (auto& pair : aggregated) {
        pair.second.minutes = roundUpToNearest10(pair.second.minutes);
        if (pair.second.minutes > 0) {
            result.workItems.push_back(pair.second);
        }
    }

    return result;
}

ExportResult YouTrackExportService::exportToYouTrack(
    const std::vector<AggregatedWorkItem>& workItems,
    std::function<void(int, int)> progressCallback,
    const std::atomic<bool>* cancelFlag) {

    ExportResult result;

    if (!isConfigured()) {
        result.success = false;
        result.errorMessage = getConfigurationError();
        return result;
    }

    // Export each work item
    int totalMinutes = 0;
    int itemsExported = 0;
    std::vector<int64_t> exportedFactIds;
    int totalItems = static_cast<int>(workItems.size());

    for (size_t i = 0; i < workItems.size(); ++i) {
        // Check for cancellation
        if (cancelFlag != nullptr && cancelFlag->load()) {
            result.success = false;
            result.errorMessage = "Export cancelled by user";
            result.totalMinutes = totalMinutes;
            result.itemsExported = itemsExported;

            // Mark already exported facts before returning
            if (!exportedFactIds.empty()) {
                factRepository_.markFactsAsExported(exportedFactIds);
            }

            return result;
        }

        const auto& item = workItems[i];
        std::string error = postWorkItem(item.issueId, item.minutes, item.date);
        if (!error.empty()) {
            result.success = false;
            result.errorMessage = error;
            result.totalMinutes = totalMinutes;
            result.itemsExported = itemsExported;

            // Mark already exported facts before returning
            if (!exportedFactIds.empty()) {
                factRepository_.markFactsAsExported(exportedFactIds);
            }

            return result;
        }

        // Log successful export
        int64_t currentTimestamp = std::time(nullptr);
        exportLogRepository_.addLogEntry(
            item.activityName,
            item.issueId,
            item.date,
            item.minutes,
            currentTimestamp
        );

        totalMinutes += item.minutes;
        itemsExported++;

        // Collect fact IDs for marking as exported
        exportedFactIds.insert(exportedFactIds.end(), item.factIds.begin(), item.factIds.end());

        // Report progress after each item
        if (progressCallback) {
            progressCallback(itemsExported, totalItems);
        }
    }

    // Mark facts as exported
    factRepository_.markFactsAsExported(exportedFactIds);

    result.success = true;
    result.totalMinutes = totalMinutes;
    result.itemsExported = itemsExported;
    return result;
}

std::string YouTrackExportService::resolveIssueId(const std::string& activityName) const {
    auto aliases = settingsService_.getActivityAliases();
    auto it = aliases.find(activityName);
    if (it != aliases.end()) {
        return it->second;
    }
    return activityName;  // Use activity name as-is if no alias
}

bool YouTrackExportService::validateIssueId(const std::string& issueId) const {
    // Pattern: PROJECT-NUMBER (e.g., "PROJ-1234", "XXX-8606")
    std::regex pattern("^[A-Z]+-\\d+$");
    return std::regex_match(issueId, pattern);
}

int YouTrackExportService::roundUpToNearest10(int minutes) const {
    if (minutes <= 0) {
        return 10;  // Minimum billable unit
    }
    return ((minutes + 9) / 10) * 10;
}

std::string YouTrackExportService::postWorkItem(const std::string& issueId, int minutes, const std::string& date) {
    std::string url = settingsService_.getYouTrackUrl();
    std::string token = settingsService_.getYouTrackToken();

    // Strip trailing slash before building API endpoint URL
    while (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    std::string apiUrl = url + "/api/issues/" + issueId + "/timeTracking/workItems";

    // Build JSON payload
    nlohmann::json payload;
    payload["duration"]["minutes"] = minutes;
    payload["date"] = dateToMilliseconds(date);

    // Make POST request
    try {
        auto response = cpr::Post(
            cpr::Url{apiUrl},
            cpr::Header{
                {"Authorization", "Bearer " + token},
                {"Accept", "application/json"},
                {"Content-Type", "application/json"}
            },
            cpr::Body{payload.dump()}
        );

        // Check for network/connection errors
        if (response.error.code != cpr::ErrorCode::OK) {
            return "Failed to connect to YouTrack. Check URL and network. Error: " + response.error.message;
        }

        // Check HTTP status
        if (response.status_code == 401) {
            return "Invalid YouTrack token. Check settings.json.";
        } else if (response.status_code >= 400) {
            return "YouTrack API error " + std::to_string(response.status_code) + ": " + response.text;
        }

        // Success
        return "";

    } catch (const std::exception& e) {
        return std::string("Exception during export: ") + e.what();
    }
}

int64_t YouTrackExportService::dateToMilliseconds(const std::string& dateStr) const {
    // Parse YYYY-MM-DD to struct tm
    std::tm tm = {};
    std::istringstream ss(dateStr);
    ss >> std::get_time(&tm, "%Y-%m-%d");

    // Convert to time_t (seconds since epoch)
    std::time_t time = std::mktime(&tm);

    // Convert to milliseconds
    return static_cast<int64_t>(time) * 1000;
}

} // namespace timetracker::services
