#pragma once

#include <string>
#include <vector>

namespace timetracker::models {

// One line of the auto-fill profile: how much of the free time this activity should take.
// Percentages are relative to the sum of all entries, not necessarily to 100.
struct AutoFillAllocation {
    std::string activityName;
    int percent{0};
};

using AutoFillProfile = std::vector<AutoFillAllocation>;

} // namespace timetracker::models
