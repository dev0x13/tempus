#pragma once

#include <string>
#include <cstdint>

namespace timetracker {
namespace models {

struct KTalkConference {
    std::string startTime;  // ISO 8601 format: "2026-01-16T09:29:06Z"
    std::string endTime;    // ISO 8601 format: "2026-01-16T10:22:31.144Z"
    std::string title;

    KTalkConference() = default;

    KTalkConference(std::string startTime, std::string endTime, std::string title)
        : startTime(std::move(startTime)), endTime(std::move(endTime)), title(std::move(title)) {}
};

}  // namespace models
}  // namespace timetracker
