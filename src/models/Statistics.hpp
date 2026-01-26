#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <map>

namespace timetracker::models {

struct ActivityTotal {
    int64_t activityId{0};
    std::string activityName;
    int64_t totalSeconds{0};
    int count{0};
    int exportedCount{0};  // Number of facts exported to YouTrack

    [[nodiscard]] double totalHours() const {
        return static_cast<double>(totalSeconds) / 3600.0;
    }

    [[nodiscard]] bool isFullyExported() const {
        return count > 0 && exportedCount == count;
    }

    [[nodiscard]] bool isPartiallyExported() const {
        return exportedCount > 0 && exportedCount < count;
    }

    [[nodiscard]] bool isNotExported() const {
        return exportedCount == 0;
    }
};

struct DailyTotal {
    int year{0};
    int month{0};
    int day{0};
    int64_t totalSeconds{0};
    int count{0};

    [[nodiscard]] double totalHours() const {
        return static_cast<double>(totalSeconds) / 3600.0;
    }

    [[nodiscard]] std::string dateString() const {
        char buf[16];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, day);
        return buf;
    }
};

struct WeeklyTotal {
    int year{0};
    int week{0};  // ISO week number
    int64_t totalSeconds{0};
    int count{0};

    [[nodiscard]] double totalHours() const {
        return static_cast<double>(totalSeconds) / 3600.0;
    }

    [[nodiscard]] std::string weekString() const {
        char buf[16];
        snprintf(buf, sizeof(buf), "%04d-W%02d", year, week);
        return buf;
    }
};

struct MonthlyTotal {
    int year{0};
    int month{0};
    int64_t totalSeconds{0};
    int count{0};

    [[nodiscard]] double totalHours() const {
        return static_cast<double>(totalSeconds) / 3600.0;
    }

    [[nodiscard]] std::string monthString() const {
        char buf[16];
        snprintf(buf, sizeof(buf), "%04d-%02d", year, month);
        return buf;
    }
};

struct Statistics {
    std::vector<ActivityTotal> byActivity;
    std::vector<DailyTotal> daily;
    std::vector<WeeklyTotal> weekly;
    std::vector<MonthlyTotal> monthly;

    int64_t totalSeconds{0};
    int totalFacts{0};

    [[nodiscard]] double totalHours() const {
        return static_cast<double>(totalSeconds) / 3600.0;
    }
};

} // namespace timetracker::models
