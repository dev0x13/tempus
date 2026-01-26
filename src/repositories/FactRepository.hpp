#pragma once

#include "IFactRepository.hpp"
#include "database/Database.hpp"

namespace timetracker::repositories {

class FactRepository : public IFactRepository {
public:
    explicit FactRepository(database::Database& db);
    ~FactRepository() override = default;

    models::Fact create(int64_t activityId, int64_t startTime, std::optional<int64_t> endTime = std::nullopt) override;
    std::optional<models::Fact> findById(int64_t id) override;
    std::vector<models::Fact> findAll() override;
    void update(const models::Fact& fact) override;
    void remove(int64_t id) override;

    std::optional<models::Fact> findOngoing() override;
    std::vector<models::Fact> findByDateRange(int64_t startTime, int64_t endTime) override;
    std::vector<models::Fact> findByActivity(int64_t activityId) override;
    std::vector<models::Fact> findRecent(int limit = 10) override;
    void stopOngoing(int64_t endTime) override;

    // YouTrack export status management
    void markFactsAsExported(const std::vector<int64_t>& factIds);

private:
    database::Database& db_;

    models::Fact mapRow(SQLite::Statement& query);
    models::Fact mapRowWithActivity(SQLite::Statement& query);
};

} // namespace timetracker::repositories
