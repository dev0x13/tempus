#pragma once

#include "repositories/IActivityRepository.hpp"
#include "repositories/IFactRepository.hpp"
#include "models/Activity.hpp"
#include "models/Fact.hpp"
#include <memory>
#include <optional>
#include <string>

namespace timetracker::services {

class TimeTrackingService {
public:
    TimeTrackingService(
        std::shared_ptr<repositories::IActivityRepository> activityRepo,
        std::shared_ptr<repositories::IFactRepository> factRepo);
    ~TimeTrackingService() = default;

    // Start tracking a new activity (stops any ongoing)
    models::Fact startTracking(const std::string& activityName, const std::string& description = "");

    // Start tracking with a custom start time (stops any ongoing)
    models::Fact startTracking(const std::string& activityName, int64_t startTime, const std::string& description = "");

    // Stop the current tracking
    std::optional<models::Fact> stopTracking();

    // Check if currently tracking
    bool isTracking() const;

    // Get current ongoing fact
    std::optional<models::Fact> getCurrentTracking() const;

    // Manual entry
    models::Fact addManualEntry(const std::string& activityName, int64_t startTime, int64_t endTime, const std::string& description = "");

    // Edit an existing entry
    void updateEntry(const models::Fact& fact, const std::string& activityName, const std::string& description = "");

    // Delete an entry
    void deleteEntry(int64_t factId);

    // Get recent entries
    std::vector<models::Fact> getRecentEntries(int limit = 10, int offset = 0) const;

    // Get entries for a date range
    std::vector<models::Fact> getEntriesForRange(int64_t startTime, int64_t endTime) const;

    // Get all activities
    std::vector<models::Activity> getAllActivities() const;

    // Search activities
    std::vector<models::Activity> searchActivities(const std::string& query, int limit = 10) const;

private:
    std::shared_ptr<repositories::IActivityRepository> activityRepo_;
    std::shared_ptr<repositories::IFactRepository> factRepo_;

    // Cached current tracking state
    mutable std::optional<models::Fact> currentTracking_;
    mutable bool currentTrackingCached_{false};

    void invalidateCache();
    void refreshCurrentTracking() const;
};

} // namespace timetracker::services
