#pragma once

#include "IActivityRepository.hpp"
#include "database/Database.hpp"

namespace timetracker::repositories {

class ActivityRepository : public IActivityRepository {
public:
    explicit ActivityRepository(database::Database& db);
    ~ActivityRepository() override = default;

    models::Activity create(const std::string& name) override;
    std::optional<models::Activity> findById(int64_t id) override;
    std::optional<models::Activity> findByName(const std::string& name) override;
    std::vector<models::Activity> findAll(bool includeDeleted = false) override;
    void update(const models::Activity& activity) override;
    void softDelete(int64_t id) override;
    void restore(int64_t id) override;

    models::Activity getOrCreate(const std::string& name) override;
    std::vector<models::Activity> search(const std::string& query, int limit = 10) override;

private:
    database::Database& db_;

    static std::string toLower(const std::string& str);
    models::Activity mapRow(SQLite::Statement& query);
};

} // namespace timetracker::repositories
