#pragma once

#include "models/Activity.hpp"
#include <vector>
#include <optional>
#include <string>

namespace timetracker::repositories {

class IActivityRepository {
public:
    virtual ~IActivityRepository() = default;

    // CRUD operations
    virtual models::Activity create(const std::string& name) = 0;
    virtual std::optional<models::Activity> findById(int64_t id) = 0;
    virtual std::optional<models::Activity> findByName(const std::string& name) = 0;
    virtual std::vector<models::Activity> findAll() = 0;
    virtual void update(const models::Activity& activity) = 0;

    // Get or create - returns existing activity if found, creates new if not
    virtual models::Activity getOrCreate(const std::string& name) = 0;

    // Search activities by name (case-insensitive prefix match)
    virtual std::vector<models::Activity> search(const std::string& query, int limit = 10) = 0;

    // Delete activity if it has no associated facts (orphan cleanup)
    virtual void deleteIfOrphaned(int64_t activityId) = 0;
};

} // namespace timetracker::repositories
