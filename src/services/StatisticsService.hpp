#pragma once

#include "repositories/IFactRepository.hpp"
#include "models/Fact.hpp"
#include "models/Statistics.hpp"
#include <memory>
#include <cstdint>

namespace timetracker::services {

class StatisticsService {
public:
    explicit StatisticsService(std::shared_ptr<repositories::IFactRepository> factRepo);
    ~StatisticsService() = default;

    // Get statistics for a date range
    models::Statistics getStatistics(int64_t startTime, int64_t endTime) const;

    // Get statistics for an already-loaded fact collection.
    models::Statistics getStatistics(const std::vector<models::Fact>& facts) const;

    // Get totals grouped by activity
    std::vector<models::ActivityTotal> getTotalsByActivity(int64_t startTime, int64_t endTime) const;

    // Get daily totals
    std::vector<models::DailyTotal> getDailyTotals(int64_t startTime, int64_t endTime) const;

    // Get weekly totals
    std::vector<models::WeeklyTotal> getWeeklyTotals(int64_t startTime, int64_t endTime) const;

    // Get monthly totals
    std::vector<models::MonthlyTotal> getMonthlyTotals(int64_t startTime, int64_t endTime) const;

    // Get expected working seconds for a date range (weekdays × 8 h)
    int64_t getExpectedSeconds(int64_t startTime, int64_t endTime) const;

private:
    std::shared_ptr<repositories::IFactRepository> factRepo_;

    std::vector<models::ActivityTotal> getTotalsByActivity(const std::vector<models::Fact>& facts, int64_t now) const;
    std::vector<models::DailyTotal> getDailyTotals(const std::vector<models::Fact>& facts, int64_t now) const;
    std::vector<models::WeeklyTotal> getWeeklyTotals(const std::vector<models::Fact>& facts, int64_t now) const;
    std::vector<models::MonthlyTotal> getMonthlyTotals(const std::vector<models::Fact>& facts, int64_t now) const;
};

} // namespace timetracker::services
