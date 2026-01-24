#pragma once

#include "models/Fact.hpp"
#include <vector>
#include <optional>
#include <cstdint>

namespace timetracker::repositories {

class IFactRepository {
public:
    virtual ~IFactRepository() = default;

    // CRUD operations
    virtual models::Fact create(int64_t activityId, int64_t startTime, std::optional<int64_t> endTime = std::nullopt) = 0;
    virtual std::optional<models::Fact> findById(int64_t id) = 0;
    virtual std::vector<models::Fact> findAll() = 0;
    virtual void update(const models::Fact& fact) = 0;
    virtual void remove(int64_t id) = 0;

    // Find ongoing fact (fact with no end time)
    virtual std::optional<models::Fact> findOngoing() = 0;

    // Find facts by date range (inclusive)
    virtual std::vector<models::Fact> findByDateRange(int64_t startTime, int64_t endTime) = 0;

    // Find facts by activity
    virtual std::vector<models::Fact> findByActivity(int64_t activityId) = 0;

    // Get recent facts (ordered by start_time descending)
    virtual std::vector<models::Fact> findRecent(int limit = 10) = 0;

    // Stop ongoing fact by setting end time
    virtual void stopOngoing(int64_t endTime) = 0;
};

} // namespace timetracker::repositories
