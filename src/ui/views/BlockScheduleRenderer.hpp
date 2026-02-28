#pragma once

#include "models/Fact.hpp"
#include "services/SettingsService.hpp"
#include "services/TimeTrackingService.hpp"
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace timetracker::ui {

// A block in the schedule: either a fact or an empty gap
struct ScheduleBlock {
    int64_t startTime{0};
    int64_t endTime{0};
    bool isFact{false};
    const models::Fact* fact{nullptr};  // Non-null if isFact is true
};

// For backward compatibility, alias ScheduleSegment to ScheduleBlock
using ScheduleSegment = ScheduleBlock;

// User action from the schedule renderer
struct ScheduleAction {
    enum Type { None, EditFact, CreateFact };
    Type type{None};
    const models::Fact* factToEdit{nullptr};  // For EditFact
    int64_t slotStart{0};   // For CreateFact
    int64_t slotEnd{0};     // For CreateFact
};

class BlockScheduleRenderer {
public:
    BlockScheduleRenderer(
        std::shared_ptr<services::SettingsService> settingsService,
        std::shared_ptr<services::TimeTrackingService> timeService);
    ~BlockScheduleRenderer() = default;

    // Render the schedule for a single day's facts.
    // Returns an action if the user interacted with a block.
    ScheduleAction renderDay(
        const std::string& dateHeader,
        const std::string& isoDate,
        const std::vector<models::Fact>& dayFacts,
        int64_t dayStart);

private:
    std::shared_ptr<services::SettingsService> settingsService_;
    std::shared_ptr<services::TimeTrackingService> timeService_;

    // Compute activity-driven block list for the workday window
    std::vector<ScheduleBlock> computeBlocks(
        int64_t workdayStart, int64_t workdayEnd,
        const std::vector<models::Fact>& dayFacts) const;
};

} // namespace timetracker::ui
