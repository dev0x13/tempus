#include "StatisticsService.hpp"
#include "utils/TimeUtils.hpp"
#include <map>
#include <algorithm>

namespace timetracker::services {

StatisticsService::StatisticsService(std::shared_ptr<repositories::IFactRepository> factRepo)
    : factRepo_(std::move(factRepo)) {}

models::Statistics StatisticsService::getStatistics(int64_t startTime, int64_t endTime) const {
    return getStatistics(factRepo_->findByDateRange(startTime, endTime));
}

models::Statistics StatisticsService::getStatistics(const std::vector<models::Fact>& facts) const {
    models::Statistics stats;
    int64_t now = utils::TimeUtils::now();

    stats.byActivity = getTotalsByActivity(facts, now);
    stats.daily = getDailyTotals(facts, now);
    stats.weekly = getWeeklyTotals(facts, now);
    stats.monthly = getMonthlyTotals(facts, now);

    // Calculate overall totals
    for (const auto& activity : stats.byActivity) {
        stats.totalSeconds += activity.totalSeconds;
        stats.totalFacts += activity.count;
    }

    return stats;
}

std::vector<models::ActivityTotal> StatisticsService::getTotalsByActivity(int64_t startTime, int64_t endTime) const {
    return getTotalsByActivity(factRepo_->findByDateRange(startTime, endTime), utils::TimeUtils::now());
}

std::vector<models::ActivityTotal> StatisticsService::getTotalsByActivity(const std::vector<models::Fact>& facts, int64_t now) const {
    // Group by activity
    std::map<int64_t, models::ActivityTotal> totals;

    for (const auto& fact : facts) {
        auto& total = totals[fact.activityId];
        total.activityId = fact.activityId;
        total.activityName = fact.activityName;
        total.totalSeconds += fact.getDuration(now);
        total.count++;
        if (fact.exportedToYoutrack) {
            total.exportedCount++;
        }
    }

    // Convert to vector and sort by total time descending
    std::vector<models::ActivityTotal> result;
    result.reserve(totals.size());
    for (auto& [id, total] : totals) {
        result.push_back(std::move(total));
    }

    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) { return a.totalSeconds > b.totalSeconds; });

    return result;
}

std::vector<models::DailyTotal> StatisticsService::getDailyTotals(int64_t startTime, int64_t endTime) const {
    return getDailyTotals(factRepo_->findByDateRange(startTime, endTime), utils::TimeUtils::now());
}

std::vector<models::DailyTotal> StatisticsService::getDailyTotals(const std::vector<models::Fact>& facts, int64_t now) const {
    // Group by day
    std::map<std::string, models::DailyTotal> totals;

    for (const auto& fact : facts) {
        auto tm = utils::TimeUtils::toLocalTime(fact.startTime);
        char key[16];
        snprintf(key, sizeof(key), "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

        auto& total = totals[key];
        total.year = tm.tm_year + 1900;
        total.month = tm.tm_mon + 1;
        total.day = tm.tm_mday;
        total.totalSeconds += fact.getDuration(now);
        total.count++;
    }

    // Convert to vector and sort by date
    std::vector<models::DailyTotal> result;
    result.reserve(totals.size());
    for (auto& [key, total] : totals) {
        result.push_back(std::move(total));
    }

    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) {
            if (a.year != b.year) return a.year > b.year;
            if (a.month != b.month) return a.month > b.month;
            return a.day > b.day;
        });

    return result;
}

std::vector<models::WeeklyTotal> StatisticsService::getWeeklyTotals(int64_t startTime, int64_t endTime) const {
    return getWeeklyTotals(factRepo_->findByDateRange(startTime, endTime), utils::TimeUtils::now());
}

std::vector<models::WeeklyTotal> StatisticsService::getWeeklyTotals(const std::vector<models::Fact>& facts, int64_t now) const {
    // Group by week
    std::map<std::string, models::WeeklyTotal> totals;

    for (const auto& fact : facts) {
        auto tm = utils::TimeUtils::toLocalTime(fact.startTime);
        int week = utils::TimeUtils::getISOWeek(fact.startTime);
        char key[16];
        snprintf(key, sizeof(key), "%04d-W%02d", tm.tm_year + 1900, week);

        auto& total = totals[key];
        total.year = tm.tm_year + 1900;
        total.week = week;
        total.totalSeconds += fact.getDuration(now);
        total.count++;
    }

    // Convert to vector and sort by year/week
    std::vector<models::WeeklyTotal> result;
    result.reserve(totals.size());
    for (auto& [key, total] : totals) {
        result.push_back(std::move(total));
    }

    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) {
            if (a.year != b.year) return a.year > b.year;
            return a.week > b.week;
        });

    return result;
}

std::vector<models::MonthlyTotal> StatisticsService::getMonthlyTotals(int64_t startTime, int64_t endTime) const {
    return getMonthlyTotals(factRepo_->findByDateRange(startTime, endTime), utils::TimeUtils::now());
}

std::vector<models::MonthlyTotal> StatisticsService::getMonthlyTotals(const std::vector<models::Fact>& facts, int64_t now) const {
    // Group by month
    std::map<std::string, models::MonthlyTotal> totals;

    for (const auto& fact : facts) {
        auto tm = utils::TimeUtils::toLocalTime(fact.startTime);
        char key[16];
        snprintf(key, sizeof(key), "%04d-%02d", tm.tm_year + 1900, tm.tm_mon + 1);

        auto& total = totals[key];
        total.year = tm.tm_year + 1900;
        total.month = tm.tm_mon + 1;
        total.totalSeconds += fact.getDuration(now);
        total.count++;
    }

    // Convert to vector and sort by year/month
    std::vector<models::MonthlyTotal> result;
    result.reserve(totals.size());
    for (auto& [key, total] : totals) {
        result.push_back(std::move(total));
    }

    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) {
            if (a.year != b.year) return a.year > b.year;
            return a.month > b.month;
        });

    return result;
}

int64_t StatisticsService::getExpectedSeconds(int64_t startTime, int64_t endTime) const {
    return static_cast<int64_t>(utils::TimeUtils::countWeekdays(startTime, endTime)) * 8 * 3600;
}

} // namespace timetracker::services
