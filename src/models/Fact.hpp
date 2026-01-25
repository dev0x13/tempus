#pragma once

#include <string>
#include <cstdint>
#include <optional>

namespace timetracker::models {

struct Fact {
    int64_t id{0};
    int64_t activityId{0};
    int64_t startTime{0};                // Unix timestamp
    std::optional<int64_t> endTime;      // Null for ongoing
    bool exportedToYoutrack{false};      // Whether exported to YouTrack

    // Cached activity name (not stored in DB, populated by joins)
    std::string activityName;

    Fact() = default;

    Fact(int64_t id, int64_t activityId, int64_t startTime, std::optional<int64_t> endTime = std::nullopt)
        : id(id), activityId(activityId), startTime(startTime), endTime(endTime) {}

    [[nodiscard]] bool isOngoing() const {
        return !endTime.has_value();
    }

    [[nodiscard]] int64_t getDuration() const {
        if (endTime.has_value()) {
            return *endTime - startTime;
        }
        return 0;  // Ongoing facts need current time to calculate duration
    }

    [[nodiscard]] int64_t getDuration(int64_t currentTime) const {
        if (endTime.has_value()) {
            return *endTime - startTime;
        }
        return currentTime - startTime;
    }

    bool operator==(const Fact& other) const {
        return id == other.id;
    }
};

} // namespace timetracker::models
