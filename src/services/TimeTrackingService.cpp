#include "TimeTrackingService.hpp"
#include "utils/TimeUtils.hpp"

namespace timetracker::services {

TimeTrackingService::TimeTrackingService(
    std::shared_ptr<repositories::IActivityRepository> activityRepo,
    std::shared_ptr<repositories::IFactRepository> factRepo)
    : activityRepo_(std::move(activityRepo))
    , factRepo_(std::move(factRepo)) {}

models::Fact TimeTrackingService::startTracking(const std::string& activityName, const std::string& description) {
    // Stop any ongoing tracking first
    stopTracking();

    // Get or create the activity
    auto activity = activityRepo_->getOrCreate(activityName);

    // Update description if different
    if (activity.description != description) {
        activity.description = description;
        activityRepo_->update(activity);
    }

    // Create new fact with current time
    int64_t now = utils::TimeUtils::now();
    auto fact = factRepo_->create(activity.id, now);
    fact.activityName = activity.name;

    // Update cache
    currentTracking_ = fact;
    currentTrackingCached_ = true;

    return fact;
}

models::Fact TimeTrackingService::startTracking(const std::string& activityName, int64_t startTime, const std::string& description) {
    // Stop any ongoing tracking first
    stopTracking();

    // Get or create the activity
    auto activity = activityRepo_->getOrCreate(activityName);

    // Update description if different
    if (activity.description != description) {
        activity.description = description;
        activityRepo_->update(activity);
    }

    // Create new fact with custom start time
    auto fact = factRepo_->create(activity.id, startTime);
    fact.activityName = activity.name;

    // Update cache
    currentTracking_ = fact;
    currentTrackingCached_ = true;

    return fact;
}

std::optional<models::Fact> TimeTrackingService::stopTracking() {
    refreshCurrentTracking();

    if (!currentTracking_.has_value()) {
        return std::nullopt;
    }

    int64_t now = utils::TimeUtils::now();
    factRepo_->stopOngoing(now);

    auto stoppedFact = *currentTracking_;
    stoppedFact.endTime = now;

    invalidateCache();

    return stoppedFact;
}

bool TimeTrackingService::isTracking() const {
    refreshCurrentTracking();
    return currentTracking_.has_value();
}

std::optional<models::Fact> TimeTrackingService::getCurrentTracking() const {
    refreshCurrentTracking();
    return currentTracking_;
}

models::Fact TimeTrackingService::addManualEntry(const std::string& activityName, int64_t startTime, int64_t endTime, const std::string& description) {
    auto activity = activityRepo_->getOrCreate(activityName);

    // Update description if different
    if (activity.description != description) {
        activity.description = description;
        activityRepo_->update(activity);
    }

    auto fact = factRepo_->create(activity.id, startTime, endTime);
    fact.activityName = activity.name;
    return fact;
}

void TimeTrackingService::updateEntry(const models::Fact& fact, const std::string& activityName, const std::string& description) {
    auto activity = activityRepo_->getOrCreate(activityName);

    // Update description if different
    if (activity.description != description) {
        activity.description = description;
        activityRepo_->update(activity);
    }

    models::Fact updatedFact = fact;
    updatedFact.activityId = activity.id;

    factRepo_->update(updatedFact);
    invalidateCache();
}

void TimeTrackingService::deleteEntry(int64_t factId) {
    factRepo_->remove(factId);
    invalidateCache();
}

std::vector<models::Fact> TimeTrackingService::getRecentEntries(int limit) const {
    return factRepo_->findRecent(limit);
}

std::vector<models::Fact> TimeTrackingService::getEntriesForRange(int64_t startTime, int64_t endTime) const {
    return factRepo_->findByDateRange(startTime, endTime);
}

std::vector<models::Activity> TimeTrackingService::getAllActivities() const {
    return activityRepo_->findAll();
}

std::vector<models::Activity> TimeTrackingService::searchActivities(const std::string& query, int limit) const {
    return activityRepo_->search(query, limit);
}

void TimeTrackingService::invalidateCache() {
    currentTrackingCached_ = false;
}

void TimeTrackingService::refreshCurrentTracking() const {
    if (!currentTrackingCached_) {
        currentTracking_ = factRepo_->findOngoing();
        currentTrackingCached_ = true;
    }
}

} // namespace timetracker::services
